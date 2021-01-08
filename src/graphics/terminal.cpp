#include "graphics.h"

#include "os/platform.h"
#include "utils/log.h"

static constexpr auto backend = nngn::Graphics::Backend::TERMINAL_BACKEND;

#ifndef HAVE_TERMIOS_H

namespace nngn {

template<>
std::unique_ptr<Graphics> graphics_create_backend<backend>(const void*) {
    NNGN_LOG_CONTEXT_F();
    nngn::Log::l() << "compiled without terminal support\n";
    return {};
}

}

#else

#include <algorithm>
#include <list>
#include <span>
#include <string_view>
#include <thread>
#include <vector>

#include <sys/ioctl.h>
#include <unistd.h>

#include "math/math.h"
#include "timing/timing.h"
#include "utils/ranges.h"

#include "pseudo.h"

using namespace std::literals;

using nngn::u32, nngn::u64;
using Mode = nngn::Graphics::TerminalMode;

namespace {

/** Character sequences to control a VT100 terminal. */
struct VT100EscapeCodes {
    static constexpr auto clear = "\x1b[2J"sv;
    static constexpr auto pos = "\x1b[H"sv;
};

/** Character sequences to control a VT520 terminal. */
struct VT520EscapeCodes {
    static constexpr auto show_cursor = "\x1b[?25h"sv;
    static constexpr auto hide_cursor = "\x1b[?25l"sv;
};

/** ANSI escape code sequences. */
struct ANSIEscapeCode {
    static constexpr auto reset_color = "\x1b[39;49m"sv;
    static constexpr auto bg_color_24bit = "\x1b[48;2;"sv;
};

/** Handles interactions with the output terminal. */
class Terminal {
    /** OS file descriptor. */
    int fd;
    /** `fopen`ed version of \ref fd. */
    std::FILE *f = {};
    nngn::uvec2 m_size = {}, pixel = {};
public:
    NNGN_MOVE_ONLY(Terminal)
    /**
     * Creates an object for a given TTY.
     * \param fd
     *     OS file descriptor, whose lifetime is not managed and must remain
     *     valid until the object is destructed.
     */
    Terminal(int fd);
    ~Terminal(void);
    /** Size of the terminal in characters. */
    auto size() const { return this->m_size; }
    /** Size of the terminal in pixels. */
    auto pixel_size() const { return this->pixel; }
    bool init();
    /** Asks the OS for the terminal size.  Returns {changed, ok}. */
    std::tuple<bool, bool> update_size();
    /** Outputs the entire contents of a buffer. */
    bool write(std::size_t n, const char *p) const;
    /** Outputs the entire contents of a container. */
    template<typename T>
    bool write(const T &v) const { return this->write(v.size(), v.data()); }
    /** Synchronizes the output file descriptor. */
    bool flush();
};

/** Simulates a v-sync pause using `sleep`. */
class FrameLimiter {
    int m_interval = 1;
    nngn::Timing::time_point last = {};
public:
    int interval() const { return this->m_interval; }
    void set_interval(int i) { this->m_interval = i; }
    /** `sleep`s for as long as necessary to maintain a constant frame rate. */
    void limit();
};

/** Buffer holding texture image data. */
struct Texture {
    using texel3 = nngn::vec3_base<std::uint8_t>;
    using texel4 = nngn::vec4_base<std::uint8_t>;
    void copy(const std::uint8_t *p);
    texel4 sample(nngn::vec2 uv) const;
private:
    std::vector<texel4> data = {};
};

class FrameBuffer {
public:
    explicit FrameBuffer(Mode m);
    /** Pointer to the content. */
    std::span<char> span() { return this->v; }
    /** Changes the size and clears the content according to the mode. */
    void resize_and_clear(nngn::uvec2 s);
    /** Write pixel at <tt>{x, y}</tt> with \c color. */
    void write(std::size_t x, std::size_t y, Texture::texel4 color)
        { (this->*wf)(x, y, color); }
    /** Inverts the Y coord. of all pixels, must be called before \ref dedup. */
    void flip();
    /**
     * Eliminates redundant information, possibly reducing the size.
     * Unique elements will be in <tt>span(0, dedup())</tt>.
     */
    std::size_t dedup();
private:
    using write_f = void (FrameBuffer::*)(
        std::size_t, std::size_t, Texture::texel4);
    struct ColoredPixel {
        static constexpr auto CMD = ANSIEscapeCode::bg_color_24bit;
        std::array<char, CMD.size()> cmd = nngn::to_array<CMD.size()>(CMD);
        std::array<char, 3> r = nngn::to_array("000");
        char semicolon0 = ';';
        std::array<char, 3> g = nngn::to_array("000");
        char semicolon1 = ';';
        std::array<char, 3> b = nngn::to_array("000");
        char m = 'm', space = ' ';
        ColoredPixel() = default;
        explicit ColoredPixel(Texture::texel3 color);
        auto operator<=>(const ColoredPixel&) const = default;
        static bool cmp_rgb(const ColoredPixel &lhs, const ColoredPixel &rhs);
    };
    static_assert(std::has_unique_object_representations_v<ColoredPixel>);
    std::size_t pixel_size() const;
    std::size_t size_bytes() const;
    void write_ascii(std::size_t x, std::size_t y, Texture::texel4 color);
    void write_colored(std::size_t x, std::size_t y, Texture::texel4 color);
    Mode mode;
    // TODO separate render targets, bulk writes
    write_f wf;
    std::vector<char> v = {};
    nngn::uvec2 size = {};
};

/** Axis-aligned sprite rasterizer with texture sampling. */
class Rasterizer {
    nngn::vec3 size = {};
    nngn::mat4 proj = {};
    /**
     * Transforms world-space vertices into clip space.
     * \return
     *     {bl, tr, bt, tt}: bottom-left/top-right vertices / texture
     *     coordinates
     */
    std::array<nngn::vec3, 4> to_clip(nngn::Vertex v0, nngn::Vertex v1) const;
    /** Transforms clip-space vertices into screen space. */
    nngn::vec2 to_screen(nngn::vec3 v) const;
    /** Checks whether a sprite lies entirely outside of the clip volume. */
    static bool clip(nngn::vec3 bl, nngn::vec3 tr);
    /** Clamps value between \c 0 and \c max, rounds toward \c -infinity. */
    static std::size_t clamp_floor(float max, float v);
    /** Clamps value between \c 0 and \c max, rounds toward \c +infinity. */
    static std::size_t clamp_ceil(float max, float v);
    /** Calculates texture coordinates for a given position in a sprite. */
    static float uv_coord(
        float p0, std::size_t p, float p1, float t0, float t1);
public:
    void update_projection(
        nngn::uvec2 term_size, nngn::uvec2 window_size,
        const nngn::Graphics::Camera &c);
    void rasterize(
        std::size_t vbo_n, const nngn::Vertex *vbo,
        std::size_t ebo_n, const std::uint32_t *ebo,
        std::size_t n_textures, const Texture *textures,
        FrameBuffer *framebuffer) const;
};

class TerminalBackend final : public nngn::Pseudograph {
    Terminal term;
    FrameLimiter frame_limiter = {};
    Camera camera = {};
    Rasterizer rasterizer = {};
    struct {
        size_callback_f size_cb;
        void *size_p;
    } callback_data = {};
    std::vector<Texture> textures = {};
    std::vector<std::vector<std::byte>> buffers = {{}};
    std::vector<std::pair<u32, u32>> rendered_buffers = {};
    FrameBuffer framebuffer;
    auto version() const -> Version final { return {0, 0, 0, "terminal"}; }
    bool init() final { return this->term.init(); }
    int swap_interval() const final { return this->frame_limiter.interval(); }
    nngn::uvec2 window_size() const final { return this->term.size(); }
    void set_swap_interval(int i) final { this->frame_limiter.set_interval(i); }
    void set_size_callback(void *data, size_callback_f f) final
        { this->callback_data = {f, data}; }
    void set_camera(const Camera &c) final;
    void set_camera_updated() final;
    u32 create_pipeline(const PipelineConfiguration &conf) override;
    u32 create_buffer(const BufferConfiguration &conf) final;
    bool set_buffer_capacity(u32 b, u64 size) final;
    bool set_buffer_size(u32 b, u64 size) final;
    bool write_to_buffer(
        u32 b, u64 offset, u64 n, u64 size,
        void *data, void f(void*, void*, u64, u64)) final;
    bool resize_textures(u32 s) final
        { this->textures.resize(s); return true; }
    bool load_textures(u32 i, u32 n, const std::byte *v) final;
    bool set_render_list(const RenderList &l) final;
    bool render() final;
public:
    NNGN_MOVE_ONLY(TerminalBackend)
    TerminalBackend(int fd, Mode mode) : term(fd), framebuffer(mode) {}
    ~TerminalBackend(void) final = default;
};

Terminal::Terminal(int fd_) : fd(fd_) {}

Terminal::~Terminal() {
    this->write(ANSIEscapeCode::reset_color);
    this->write(VT520EscapeCodes::show_cursor);
    this->write(1, "\n");
    if(fclose(this->f))
        nngn::Log::perror("fclose");
}

bool Terminal::init() {
    NNGN_LOG_CONTEXT_CF(Terminal);
    if(!(this->f = fdopen(this->fd, "w")))
        return nngn::Log::perror("fdopen"), false;
    return true;
}

std::tuple<bool, bool> Terminal::update_size() {
    NNGN_LOG_CONTEXT_CF(Terminal);
    winsize ws = {};
    std::tuple<bool, bool> ret = {};
    if(ioctl(this->fd, TIOCGWINSZ, &ws) == -1)
        return nngn::Log::perror("ioctl(TIOCGWINSZ)"), ret;
    if(!(ws.ws_xpixel || ws.ws_ypixel))
        return nngn::Log::l() << "ws_xpixel or ws_ypixel is zero\n", ret;
    auto old = std::exchange(this->m_size, {ws.ws_col, ws.ws_row});
    auto old_px = std::exchange(this->pixel, {ws.ws_xpixel, ws.ws_ypixel});
    ret = {this->m_size != old || this->pixel != old_px, true};
    return ret;
}

bool Terminal::write(std::size_t n, const char *p) const {
    while(n) {
        const auto w = fwrite(p, 1, n, this->f);
        if(ferror(this->f) && errno != EAGAIN)
            return nngn::Log::perror("fwrite"), false;
        p += static_cast<std::ptrdiff_t>(w);
        n -= w;
    }
    return true;
}

bool Terminal::flush() {
    NNGN_LOG_CONTEXT_CF(Terminal);
    while(fflush(this->f))
        if(errno != EAGAIN)
            return nngn::Log::perror("fflush"), false;
    return true;
}

void FrameLimiter::limit() {
    if(this->m_interval < 1)
        return;
    using clock = nngn::Timing::clock;
    using duration = nngn::Timing::duration;
    constexpr duration::rep frame_rate_hz = 60;
    constexpr auto min_dt =
        std::chrono::duration_cast<duration>(std::chrono::seconds{1})
        / frame_rate_hz;
    const auto dt = clock::now() - this->last;
    if(const auto s = this->m_interval * min_dt - dt; s > duration{})
        std::this_thread::sleep_for(s);
    this->last = clock::now();
}

void Texture::copy(const std::uint8_t *p) {
    constexpr auto n = nngn::Graphics::TEXTURE_SIZE;
    if(this->data.empty())
        this->data.resize(n);
    std::copy(p, p + n, &this->data[0][0]);
}

auto Texture::sample(nngn::vec2 uv) const -> texel4 {
    if(this->data.empty() || uv.x < 0 || 1 <= uv.x || uv.y < 0 || 1 <= uv.y)
        return {};
    constexpr auto size = nngn::Graphics::TEXTURE_EXTENT;
    constexpr auto fsize = static_cast<float>(size);
    const auto i = static_cast<nngn::uvec2>(uv * fsize);
    return this->data[size * i.y + i.x];
}

FrameBuffer::ColoredPixel::ColoredPixel(Texture::texel3 color) {
    std::snprintf(
        this->r.data(), 12, "%03u;%03u;%03u",
        color[0], color[1], color[2]);
}

bool FrameBuffer::ColoredPixel::cmp_rgb(
    const ColoredPixel &lhs, const ColoredPixel &rhs
) {
    constexpr auto off0 = offsetof(ColoredPixel, r);
    constexpr auto off1 = offsetof(ColoredPixel, m);
    static_assert(off0 < off1);
    constexpr auto n = off1 - off0;
    return std::memcmp(lhs.r.data(), rhs.r.data(), n) == 0;
}

FrameBuffer::FrameBuffer(Mode m) :
    mode(m),
    wf(
        m == Mode::ASCII ? &FrameBuffer::write_ascii
        : m == Mode::COLORED ? &FrameBuffer::write_colored
        : nullptr) {}

std::size_t FrameBuffer::pixel_size() const {
    switch(this->mode) {
    case Mode::COLORED: return sizeof(ColoredPixel);
    case Mode::ASCII:
    default: return 1;
    }
}

std::size_t FrameBuffer::size_bytes() const
    { return this->pixel_size() * nngn::Math::product(this->size); }

void FrameBuffer::resize_and_clear(nngn::uvec2 s) {
    this->size = s;
    const auto n = this->size_bytes();
    const auto old = this->v.size();
    switch(this->mode) {
    case Mode::ASCII: {
        constexpr auto fill = ' ';
        if(n != old)
            this->v.resize(n, fill);
        const auto b = begin(this->v);
        std::fill(b, b + static_cast<std::ptrdiff_t>(old), fill);
        break;
    }
    case Mode::COLORED:
        if(n != old)
            this->v.resize(n);
        ColoredPixel f = {};
        const auto *const b = f.cmd.data();
        assert(!(this->v.size() % sizeof(f)));
        nngn::fill_with_pattern(b, b + sizeof(f), begin(this->v), end(this->v));
        break;
    }
}

void FrameBuffer::write_ascii(
    std::size_t x, std::size_t y, Texture::texel4 color
) {
    constexpr auto lum =
        " `^\",:;Il!i~+_-?][}{1)(|\\/"
        "tfjrxnuvczXYUJCLQ0OZmwqpdbkhao*#MW&8%B@$"sv;
    constexpr auto max = static_cast<float>(lum.size() - 1);
    const auto fc = static_cast<nngn::vec4>(color) / 255.0f;
    const auto c = nngn::Math::avg(fc.xyz()) * fc[3];
    assert(0 <= c || c <= 1);
    const auto ci = static_cast<std::size_t>(c * max);
    if(!ci)
        return;
    const auto w = static_cast<std::size_t>(this->size.x);
    const auto i = w * y + x;
    assert(i < this->size_bytes());
    this->v[i] = lum[ci];
}

void FrameBuffer::write_colored(
    std::size_t x, std::size_t y, Texture::texel4 color
) {
    if(!color[3])
        return;
    const auto fc = static_cast<nngn::vec4>(color);
    const auto rgb = static_cast<Texture::texel3>(fc.xyz() * (fc[3] / 255.0f));
    const auto w = static_cast<std::size_t>(this->size.x);
    const auto i = sizeof(ColoredPixel) * (w * y + x);
    assert(i < this->size_bytes());
    const auto px = ColoredPixel{rgb};
    std::memcpy(&this->v[i], &px, sizeof(px));
}

void FrameBuffer::flip() {
    auto [w, h] = this->size;
    w *= static_cast<unsigned>(this->pixel_size());
    std::vector<char> tmp_v(w);
    auto *const tmp = tmp_v.data();
    for(std::size_t y = 0, e = h / 2; y < e; ++y) {
        auto *const p0 = &this->v[w * y];
        auto *const p1 = &this->v[w * (h - y - 1)];
        std::copy(p0, p0 + w, tmp);
        std::copy(p1, p1 + w, p0);
        std::copy(tmp, tmp + w, p1);
    }
}

std::size_t FrameBuffer::dedup() {
    if(this->mode != Mode::COLORED)
        return this->v.size();
    constexpr auto bytes = sizeof(ColoredPixel);
    assert(!(this->v.size() % bytes));
    ColoredPixel last = {};
    const auto pred = [&last](const auto &x)
        { return !ColoredPixel::cmp_rgb(last, x); };
    auto *w = this->v.data();
    auto *p = reinterpret_cast<const ColoredPixel*>(w);
    const auto *const e = p + this->v.size() / bytes;
    for(;; last = *p, w += bytes, ++p) {
        const auto next = std::find_if(p, e, pred);
        if(const auto n = static_cast<std::size_t>(next - p)) {
            w = std::fill_n(w, n, ' ');
            p = next;
        }
        if(p == e)
            break;
        if(!nngn::ptr_cmp(w, p))
            std::memcpy(w, p, bytes);
    }
    return static_cast<std::size_t>(w - this->v.data());
}

void Rasterizer::update_projection(
    nngn::uvec2 term_size, nngn::uvec2 window_size,
    const nngn::Graphics::Camera &c
) {
    const auto fsize = static_cast<nngn::vec2>(term_size);
    const auto scale = fsize / static_cast<nngn::vec2>(window_size);
    const auto scale_mat = nngn::mat4{
        scale.x, 0, 0, 0,
        0, scale.y, 0, 0,
        0, 0, 1, 0,
        0, 0, 0, 1};
    this->proj = nngn::Math::transpose(*c.proj * scale_mat * *c.view);
    this->size = nngn::vec3{fsize, 1};
}

std::array<nngn::vec3, 4> Rasterizer::to_clip(
    nngn::Vertex v0, nngn::Vertex v1
) const {
    const auto f = [this](auto p)
        { return (this->proj * nngn::vec4{p, 1}).persp_div(); };
    auto bl = f(v0.pos), tr = f(v1.pos);
    auto bt = v0.color, tt = v1.color;
    if(tr.x < bl.x)
        std::swap(bl.x, tr.x), std::swap(bt[0], tt[0]);
    if(tr.y < bl.y)
        std::swap(bl.y, tr.y), std::swap(bt[1], tt[1]);
    return {bl, tr, bt, tt};
}

nngn::vec2 Rasterizer::to_screen(nngn::vec3 v) const
    { return (this->size * (v / 2.0f + nngn::vec3{0.5f, 0.5f, 0})).xy(); }

bool Rasterizer::clip(nngn::vec3 bl, nngn::vec3 tr)
    { return tr.x < -1 || tr.y < -1 || 1 <= bl.x || 1 <= bl.y; }

std::size_t Rasterizer::clamp_floor(float max, float v) {
    return static_cast<std::size_t>(std::floor(std::clamp(v, 0.0f, max)));
}

std::size_t Rasterizer::clamp_ceil(float max, float v) {
    return static_cast<std::size_t>(std::ceil(std::clamp(v, 0.0f, max)));
}

float Rasterizer::uv_coord(
    float p0, std::size_t p, float p1, float t0, float t1
) {
    const auto fp = static_cast<float>(p);
    assert(p0 <= fp && fp <= p1);
    const auto t = std::clamp((fp - p0) / (p1 - p0), 0.0f, 1.0f);
    return t0 + (t1 - t0) * t;
}

void Rasterizer::rasterize(
    [[maybe_unused]] std::size_t vbo_n, const nngn::Vertex *vbo,
    std::size_t ebo_n, const std::uint32_t *ebo,
    [[maybe_unused]] std::size_t n_textures, const Texture *textures,
    FrameBuffer *framebuffer
) const {
    const auto screen_size = this->size.xy() - nngn::vec2{1};
    // TODO use entire EBO when proper rasterization is implemented
    for(const auto *const e = ebo + ebo_n; ebo != e; ebo += 6) {
        const auto vbo_idx0 = ebo[0], vbo_idx1 = ebo[5];
        assert(vbo_idx0 < vbo_n && vbo_idx1 < vbo_n);
        const auto [bl_clip, tr_clip, bl_t, tr_t] =
            this->to_clip(vbo[vbo_idx0], vbo[vbo_idx1]);
        if(Rasterizer::clip(bl_clip, tr_clip))
            continue;
        const auto tex_i = static_cast<std::size_t>(bl_t[2]);
        assert(tex_i < n_textures);
        const auto &tex = textures[tex_i];
        const auto bl_screen = this->to_screen(bl_clip);
        if(screen_size.x < bl_screen.x || screen_size.y < bl_screen.y)
            continue;
        const auto tr_screen = this->to_screen(tr_clip);
        const auto xb = Rasterizer::clamp_ceil(screen_size.x, bl_screen.x);
        const auto yb = Rasterizer::clamp_ceil(screen_size.y, bl_screen.y);
        const auto xe = Rasterizer::clamp_floor(screen_size.x, tr_screen.x);
        const auto ye = Rasterizer::clamp_floor(screen_size.y, tr_screen.y);
        for(auto y = yb; y <= ye; ++y) {
            const auto v = Rasterizer::uv_coord(
                bl_screen.y, y, tr_screen.y, bl_t.y, tr_t.y);
            for(auto x = xb; x <= xe; ++x) {
                const auto u = Rasterizer::uv_coord(
                    bl_screen.x, x, tr_screen.x, bl_t.x, tr_t.x);
                framebuffer->write(x, y, tex.sample({u, v}));
            }
        }
    }
}

void TerminalBackend::set_camera(const Camera &c)
    { this->camera = c; this->set_camera_updated(); }

void TerminalBackend::set_camera_updated() {
    this->rasterizer.update_projection(
        this->term.size(), this->term.pixel_size(), this->camera);
}

u32 TerminalBackend::create_pipeline(const PipelineConfiguration &conf) {
    return conf.type == PipelineConfiguration::Type::SPRITE ? 1 : 2;
}

u32 TerminalBackend::create_buffer(const BufferConfiguration &conf) {
    const auto ret = static_cast<u32>(this->buffers.size());
    auto &b = this->buffers.emplace_back();
    if(conf.size)
        b.resize(static_cast<std::size_t>(conf.size));
    return ret;
}

bool TerminalBackend::set_buffer_capacity(u32 b, u64 size) {
    auto *const v = &this->buffers[static_cast<std::size_t>(b)];
    v->resize(static_cast<std::size_t>(size));
    v->shrink_to_fit();
    return true;
}

bool TerminalBackend::set_buffer_size(u32 b, u64 s) {
    const auto sz = static_cast<std::size_t>(s);
    auto &v = this->buffers[static_cast<std::size_t>(b)];
    assert(sz <= v.capacity());
    v.resize(sz);
    return true;
}

bool TerminalBackend::load_textures(
    std::uint32_t i, std::uint32_t n, const std::byte *v
) {
    constexpr auto size = Graphics::TEXTURE_SIZE;
    for(const auto e = i + n; i < e; ++i)
        this->textures[i].copy(
            static_cast<const std::uint8_t*>(
                static_cast<const void*>(std::exchange(v, v + size))));
    return true;
}

bool TerminalBackend::write_to_buffer(
    u32 b, u64 offset, u64 n, [[maybe_unused]] u64 size,
    void *data, void f(void*, void*, u64, u64)
) {
    assert(b < this->buffers.size());
    auto &v = this->buffers[b];
    assert(offset + n * size <= v.capacity());
    f(data, v.data() + offset, 0, n);
    return true;
}

bool TerminalBackend::set_render_list(const RenderList &l) {
    const auto f = [this](const auto &l_) {
        for(const auto &s : l_) {
            if(s.pipeline != 1)
                continue;
            for(const auto &x : s.buffers)
                this->rendered_buffers.push_back(x);
        }
    };
    this->rendered_buffers.clear();
    f(l.depth);
    f(l.map_ortho);
    f(l.map_persp);
    f(l.normal);
    f(l.no_light);
    f(l.overlay);
    f(l.hud);
    f(l.shadow_maps);
    f(l.shadow_cubes);
    this->rendered_buffers.shrink_to_fit();
    return true;
}

bool TerminalBackend::render() {
    NNGN_LOG_CONTEXT_CF(TerminalBackend);
    const auto rasterize = [this](u32 vbo_i, u32 ebo_i) {
        const auto &vbo = this->buffers[vbo_i];
        const auto &ebo = this->buffers[ebo_i];
        this->rasterizer.rasterize(
            vbo.size() / sizeof(nngn::Vertex),
            static_cast<const nngn::Vertex*>(
                static_cast<const void*>(vbo.data())),
            ebo.size() / sizeof(std::uint32_t),
            static_cast<const std::uint32_t*>(
                static_cast<const void*>(ebo.data())),
            this->textures.size(), this->textures.data(),
            &this->framebuffer);
    };
    const auto write = [this] {
        const auto nw = this->framebuffer.dedup();
        return this->term.write(VT520EscapeCodes::hide_cursor)
            && this->term.write(VT100EscapeCodes::clear)
            && this->term.write(VT100EscapeCodes::pos)
            && this->term.write(this->framebuffer.span())
            && this->term.write(this->framebuffer.span().subspan(0, nw))
            && this->term.write(ANSIEscapeCode::reset_color)
            && this->term.write(VT520EscapeCodes::show_cursor)
            && this->term.flush();
    };
    if(const auto [changed, ok] = this->term.update_size(); !ok)
        return false;
    else if(changed) {
        this->callback_data.size_cb(
            this->callback_data.size_p, this->term.size());
        this->set_camera_updated();
    }
    const auto size = this->term.size();
    this->framebuffer.resize_and_clear(size);
    for(const auto &[vbo, ebo] : this->rendered_buffers)
        rasterize(vbo, ebo);
    this->framebuffer.flip();
    if(!write())
        return false;
    this->frame_limiter.limit();
    return true;
}

}

namespace nngn {

template<>
std::unique_ptr<Graphics> graphics_create_backend<backend>(const void *params) {
    using P = Graphics::TerminalParameters;
    auto p = params ? *static_cast<const P*>(params) : P{};
    if(p.fd == -1)
        p.fd = STDOUT_FILENO;
    return std::make_unique<TerminalBackend>(p.fd, p.mode);
}

}

#endif

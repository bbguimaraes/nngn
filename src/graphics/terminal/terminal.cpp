#include "graphics/graphics.h"
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

#include "font/font.h"
#include "graphics/pseudo.h"
#include "os/terminal.h"
#include "timing/limit.h"
#include "utils/flags.h"

#include "frame_buffer.h"
#include "rasterizer.h"

using namespace nngn::literals;
using nngn::u8, nngn::u32, nngn::u64, nngn::uvec2, nngn::mat4;
using Flag = nngn::Graphics::TerminalFlag;

namespace {

/**
  * Graphics back end for character terminals.
  * Implemented as a software rasterizer, either monochrome or colored.
  * Currently only supports axis-aligned sprites (which includes text).
  */
class TerminalBackend final : public nngn::Pseudograph {
public:
    NNGN_MOVE_ONLY(TerminalBackend)
    /**
      * Constructor.
      * \param fd
      *     Non-owning reference to the output file.  Must remain valid while
      *     the object exists.
      */
    TerminalBackend(int fd, Flag f);
    /** Resets the terminal to its previous state and deallocates all data. */
    ~TerminalBackend(void) final;
private:
    struct RenderList {
        struct Stage {
            PipelineConfiguration::Type type;
            u32 vbo, ebo;
        };
        static void set_stage(
            std::vector<Stage> *dst,
            std::span<const Graphics::RenderList::Stage> src,
            std::span<const PipelineConfiguration::Type> pipelines);
        std::vector<Stage>
            depth = {}, map_ortho = {}, map_persp = {},
            normal = {}, no_light = {}, overlay = {}, hud = {},
            shadow_maps = {}, shadow_cubes = {};
    };
    // Graphics overrides
    auto version(void) const -> Version final { return {0, 0, 0, "terminal"}; }
    bool init(void) final;
    int swap_interval(void) const final;
    uvec2 window_size(void) const final { return this->term.size(); }
    void set_swap_interval(int i) final { this->frame_limiter.set_interval(i); }
    void set_size_callback(void *data, size_callback_f f) final;
    void set_camera(const Camera &c) final;
    void set_camera_updated(void) final;
    u32 create_pipeline(const PipelineConfiguration &conf) final;
    u32 create_buffer(const BufferConfiguration &conf) final;
    bool set_buffer_capacity(u32 b, u64 size) final;
    bool set_buffer_size(u32 b, u64 size) final;
    bool write_to_buffer(
        u32 b, u64 offset, u64 n, u64 size,
        void *data, void f(void*, void*, u64, u64)) final;
    bool resize_textures(u32 n) final;
    bool load_textures(u32 i, u32 n, const std::byte *v) final;
    bool resize_font(u32 n) final;
    bool load_font(
        unsigned char c, u32 n, const nngn::uvec2 *size,
        const std::byte *v) final;
    bool set_render_list(const Graphics::RenderList &l) final;
    bool render(void) final;
    // Buffer/texture helpers
    std::vector<std::byte> &buffer(u32 i);
    const std::vector<std::byte> &buffer(u32 i) const;
    std::span<const nngn::Vertex> vbo(u32 i) const;
    std::span<const u32> ebo(u32 i) const;
    nngn::term::Texture &texture(u32 i);
    // Data
    std::vector<PipelineConfiguration::Type> pipelines = {{}};
    std::vector<nngn::term::Texture> textures = {}, fonts = {};
    std::vector<std::vector<std::byte>> buffers = {{}};
    RenderList render_list = {};
    nngn::Terminal term = {};
    nngn::FrameLimiter frame_limiter = {};
    nngn::term::FrameBuffer frame_buffer;
    nngn::term::Rasterizer rasterizer = {};
    int fd;
    Camera camera = {};
    nngn::Flags<Flag> flags;
    struct {
        size_callback_f size_cb;
        void *size_p;
    } callback_data = {};
};

void TerminalBackend::RenderList::set_stage(
    std::vector<Stage> *dst, std::span<const Graphics::RenderList::Stage> src,
    std::span<const PipelineConfiguration::Type> pipelines)
{
    dst->resize(std::transform_reduce(
        begin(src), end(src), 0_z, std::plus<>{},
        [](const auto &x) { return x.buffers.size(); }));
    for(std::size_t i = 0, n = src.size(); i != n; ++i) {
        const auto type = pipelines[src[i].pipeline];
        for(const auto &[vbo, ebo] : src[i].buffers)
            (*dst)[i] = {type, vbo, ebo};
    }
}

TerminalBackend::TerminalBackend(int fd_, Flag f)
    : frame_buffer{f}, fd{fd_}, flags{f} {}

TerminalBackend::~TerminalBackend(void) {
    if(this->flags.is_set(Flag::HIDE_CURSOR))
        this->term.show_cursor();
}

bool TerminalBackend::init(void) {
    return this->term.init(this->fd)
        && (!this->flags.is_set(Flag::HIDE_CURSOR)
            || this->term.hide_cursor());
}

int TerminalBackend::swap_interval(void) const {
    return this->frame_limiter.interval();
}

void TerminalBackend::set_size_callback(void *data, size_callback_f f) {
    this->callback_data = {.size_cb = f, .size_p = data};
}

void TerminalBackend::set_camera(const Camera &c) {
    this->camera = c;
    this->set_camera_updated();
}

void TerminalBackend::set_camera_updated(void) {
    this->rasterizer.update_camera(
        this->term.size(), this->term.pixel_size(),
        *this->camera.proj, *this->camera.screen_proj, *this->camera.view);
}

u32 TerminalBackend::create_pipeline(const PipelineConfiguration &conf) {
    const auto ret = nngn::narrow<u32>(this->pipelines.size());
    this->pipelines.push_back(conf.type);
    return ret;
}

u32 TerminalBackend::create_buffer(const BufferConfiguration &conf) {
    const auto ret = nngn::narrow<u32>(this->buffers.size());
    auto &b = this->buffers.emplace_back();
    if(conf.size)
        nngn::set_capacity(&b, nngn::narrow<std::size_t>(conf.size));
    return ret;
}

bool TerminalBackend::set_buffer_capacity(u32 b, u64 n) {
    auto &v = this->buffer(b);
    v.resize(nngn::narrow<std::size_t>(n));
    v.shrink_to_fit();
    return true;
}

bool TerminalBackend::set_buffer_size(u32 b, u64 n) {
    auto &v = this->buffer(b);
    assert(nngn::narrow<std::size_t>(n) <= v.capacity());
    v.resize(nngn::narrow<std::size_t>(n));
    return true;
}

bool TerminalBackend::resize_textures(u32 n) {
    nngn::resize_and_init(&this->textures, n, [](auto *x) {
        constexpr auto size = nngn::Graphics::TEXTURE_EXTENT;
        x->resize({size, size});
    });
    return true;
}

bool TerminalBackend::load_textures(u32 i, u32 n, const std::byte *v) {
    constexpr auto size = Graphics::TEXTURE_SIZE;
    for(const auto e = i + n; i < e; ++i, v += size)
        this->texture(i).copy(nngn::byte_cast<const u8*>(v));
    return true;
}

bool TerminalBackend::resize_font(u32 n) {
    if(this->fonts.empty())
        this->fonts.resize(nngn::Font::N);
    for(auto &x : this->fonts)
        x.resize({n, n});
    return true;
}

bool TerminalBackend::load_font(
    unsigned char c, u32 n, const nngn::uvec2 *size, const std::byte *v)
{
    for(const auto e = c + n; c < e; ++c) {
        this->fonts[c].copy(*size, nngn::byte_cast<const u8*>(v));
        v += nngn::Math::product(*size++);
    }
    return true;
}

bool TerminalBackend::write_to_buffer(
    u32 b, u64 offset, u64 n, [[maybe_unused]] u64 size,
    void *data, void f(void*, void*, u64, u64))
{
    auto &v = this->buffer(b);
    if(const auto vn = offset + n * size; v.size() < vn)
        v.resize(nngn::narrow<std::size_t>(vn));
    f(data, v.data() + offset, 0, n);
    return true;
}

bool TerminalBackend::set_render_list(const Graphics::RenderList &l) {
    const auto f = [this](auto &dst, auto &src) {
        RenderList::set_stage(&dst, src, this->pipelines);
    };
    f(this->render_list.depth, l.depth);
    f(this->render_list.map_ortho, l.map_ortho);
    f(this->render_list.map_persp, l.map_persp);
    f(this->render_list.normal, l.normal);
    f(this->render_list.no_light, l.no_light);
    f(this->render_list.overlay, l.overlay);
    f(this->render_list.hud, l.screen);
    f(this->render_list.shadow_maps, l.shadow_maps);
    f(this->render_list.shadow_cubes, l.shadow_cubes);
    return true;
}

bool TerminalBackend::render(void) {
    NNGN_LOG_CONTEXT_CF(TerminalBackend);
    const auto rasterize = [this](mat4 proj, const auto &l) {
        using enum PipelineConfiguration::Type;
        for(const auto &s : l)
            switch(s.type) {
            case SPRITE:
                nngn::term::Rasterizer::sprite(
                    this->vbo(s.vbo), this->ebo(s.ebo), proj,
                    this->textures, &this->frame_buffer);
                break;
            case FONT:
                nngn::term::Rasterizer::font(
                    this->vbo(s.vbo), this->ebo(s.ebo), proj,
                    this->fonts, &this->frame_buffer);
                 break;
            case TRIANGLE:
            case VOXEL:
            case TRIANGLE_DEPTH:
            case SPRITE_DEPTH:
                 break; // TODO
            case MAX:
            default:
                 assert(!"invalid pipeline type");
            }
    };
    if(const auto [changed, ok] = this->term.update_size(); !ok)
        return false;
    else if(changed) {
        this->callback_data.size_cb(
            this->callback_data.size_p, this->term.size());
        this->set_camera_updated();
    }
    this->frame_buffer.resize_and_clear(this->term.size());
    const auto proj = this->rasterizer.proj();
    const auto hud_proj = this->rasterizer.hud_proj();
    rasterize(proj, this->render_list.map_ortho);
    rasterize(proj, this->render_list.map_persp);
    rasterize(proj, this->render_list.normal);
    rasterize(proj, this->render_list.no_light);
    rasterize(proj, this->render_list.overlay);
    rasterize(hud_proj, this->render_list.hud);
    this->frame_buffer.flip();
    return this->term.drain()
        && this->term.write(this->frame_buffer.span())
        && this->term.flush()
        && (this->frame_limiter.limit(), true);
}

std::vector<std::byte> &TerminalBackend::buffer(u32 i) {
    assert(nngn::narrow<std::size_t>(i) < this->buffers.size());
    return this->buffers[nngn::narrow<std::size_t>(i)];
}

const std::vector<std::byte> &TerminalBackend::buffer(u32 i) const {
    return const_cast<TerminalBackend&>(*this).buffer(i);
}

std::span<const nngn::Vertex> TerminalBackend::vbo(u32 i) const {
    return nngn::byte_cast<const nngn::Vertex>(std::span{this->buffer(i)});
}

std::span<const u32> TerminalBackend::ebo(u32 i) const {
    return nngn::byte_cast<const u32>(std::span{this->buffer(i)});
}

nngn::term::Texture &TerminalBackend::texture(u32 i) {
    assert(nngn::narrow<std::size_t>(i) < this->textures.size());
    return this->textures[nngn::narrow<std::size_t>(i)];
}

}

namespace nngn {

template<>
std::unique_ptr<Graphics> graphics_create_backend<backend>(const void *params) {
    using P = Graphics::TerminalParameters;
    const auto p = params ? *static_cast<const P*>(params) : P{};
    const int fd = p.fd == -1 ? STDOUT_FILENO : p.fd;
    return std::make_unique<TerminalBackend>(fd, p.flags);
}

}

#endif

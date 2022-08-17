#ifndef NNGN_GRAPHICS_TERMINAL_FRAME_BUFFER_H
#define NNGN_GRAPHICS_TERMINAL_FRAME_BUFFER_H

#include "graphics/graphics.h"
#include "os/os.h"
#include "utils/ranges.h"

#include "texture.h"

namespace nngn::term {

class FrameBuffer {
public:
    using Flag = nngn::Graphics::TerminalFlag;
    using Mode = nngn::Graphics::TerminalMode;
    using texel4 = Texture::texel4;
    FrameBuffer(Flag f, Mode m);
    /** Size of the frame buffer in pixels. */
    uvec2 size(void) const { return this->m_size; }
    /** Pointer to the content. */
    std::span<char> span(void) { return this->v; }
    std::span<char> prefix(void);
    std::span<char> pixels(void);
    std::span<char> suffix(void);
    /** Changes the size and clears the content according to the mode. */
    void resize_and_clear(uvec2 s);
    /** Write pixel at `{x, y}` with \p color, ASCII output. */
    void write_ascii(std::size_t x, std::size_t y, Texture::texel4 color);
    /** Write pixel at `{x, y}` with \p color, colored output. */
    void write_colored(std::size_t x, std::size_t y, Texture::texel4 color);
    /** Inverts the Y coord. of all pixels, must be called before \ref dedup. */
    void flip(void);
    /**
     * Eliminates redundant information, possibly reducing the size.
     * Unique elements and the prefix/suffix will be in `span(0, dedup())`.
     */
    std::size_t dedup(void);
private:
    struct ColoredPixel {
        /** Constructs a black pixel. */
        ColoredPixel(void) = default;
        /** Constructs an opaque pixel with a given color. */
        explicit ColoredPixel(Texture::texel3 color);
        /** Equality comparison for the RGB portion of the data. */
        static bool cmp_rgb(const ColoredPixel &lhs, const ColoredPixel &rhs);
        static constexpr auto CMD = ANSIEscapeCode::bg_color_24bit;
        std::array<char, CMD.size()> cmd = nngn::to_array<CMD.size()>(CMD);
        std::array<char, 3> r = nngn::to_array("000");
        char semicolon0 = ';';
        std::array<char, 3> g = nngn::to_array("000");
        char semicolon1 = ';';
        std::array<char, 3> b = nngn::to_array("000");
        char m = 'm', space = ' ';
    };
    static_assert(std::has_unique_object_representations_v<ColoredPixel>);
    static constexpr std::size_t prefix_size_from_flags(Flag f);
    std::size_t pixel_size(void) const;
    Flags<Flag> flags;
    Mode mode;
    std::vector<char> v = {}, flip_tmp = {};
    std::size_t prefix_size, suffix_size;
    uvec2 m_size = {};
};

inline FrameBuffer::ColoredPixel::ColoredPixel(Texture::texel3 color) {
    constexpr auto f = [](std::span<char> s, u8 x) {
        using namespace std::string_view_literals;
        static_assert(nngn::is_sequence("0123456789"sv));
        constexpr auto z = static_cast<unsigned>('0');
        const auto i = static_cast<unsigned>(x);
        s[2] = static_cast<char>(z + i       % 10);
        s[1] = static_cast<char>(z + i /  10 % 10);
        s[0] = static_cast<char>(z + i / 100 % 10);
    };
    f(this->r, color[0]);
    f(this->g, color[1]);
    f(this->b, color[2]);
}

inline bool FrameBuffer::ColoredPixel::cmp_rgb(
    const ColoredPixel &lhs, const ColoredPixel &rhs)
{
    constexpr auto off0 = offsetof(ColoredPixel, r);
    constexpr auto off1 = offsetof(ColoredPixel, m);
    static_assert(off0 < off1);
    constexpr auto n = off1 - off0;
    return std::memcmp(lhs.r.data(), rhs.r.data(), n) == 0;
}

inline FrameBuffer::FrameBuffer(Flag f, Mode m) :
    flags{f},
    mode{m},
    prefix_size{FrameBuffer::prefix_size_from_flags(f)},
    suffix_size{
        nngn::Flags<Flag>{f}.is_set(Flag::RESET_COLOR)
            ? sizeof(ANSIEscapeCode::reset_color) : 0
    }
{}

inline constexpr std::size_t FrameBuffer::prefix_size_from_flags(Flag f) {
    const auto u = to_underlying(f);
    std::size_t ret = 0;
    if(u & to_underlying(Flag::CLEAR))
        ret += sizeof(VT100EscapeCode::clear);
    if(u & to_underlying(Flag::REPOSITION))
        ret += sizeof(VT100EscapeCode::pos);
    return ret;
}

inline std::size_t FrameBuffer::pixel_size(void) const {
    switch(this->mode) {
    case Mode::COLORED:
        return sizeof(unsigned); // sizeof(ColoredPixel);
    case Mode::ASCII:
    default:
        return 1;
    }
}

inline std::span<char> FrameBuffer::prefix(void) {
    return std::span{this->v}.subspan(0, this->prefix_size);
}

inline std::span<char> FrameBuffer::pixels(void) {
    return {end(this->prefix()), begin(this->suffix())};
}

inline std::span<char> FrameBuffer::suffix(void) {
    return std::span{this->v}.subspan(this->v.size() - this->suffix_size);
}

inline void FrameBuffer::write_ascii(
    std::size_t x, std::size_t y, Texture::texel4 color)
{
    using namespace std::string_view_literals;
    constexpr auto lum =
        " `^\",:;Il!i~+_-?][}{1)(|\\/"
        "tfjrxnuvczXYUJCLQ0OZmwqpdbkhao*#MW&8%B@$"sv;
    constexpr auto max = static_cast<float>(lum.size() - 1);
    const auto fc = static_cast<vec4>(color) / 255.0f;
    const auto c = nngn::Math::avg(fc.xyz()) * fc[3];
    assert(0 <= c && c <= 1);
    const auto ci = static_cast<std::size_t>(c * max);
    if(!ci)
        return;
    const auto w = static_cast<std::size_t>(this->m_size.x);
    const auto i = w * y + x;
    assert(i < this->pixels().size());
    this->pixels()[i] = lum[ci];
}

inline void FrameBuffer::write_colored(
    std::size_t x, std::size_t y, Texture::texel4 color)
{
    // TODO blend
    if(!color[3])
        return;
    const auto fc = static_cast<vec4>(color);
    const auto rgb = static_cast<Texture::texel3>(fc.xyz() * (fc[3] / 255.0f));
    const auto w = static_cast<std::size_t>(this->m_size.x);
    const auto i = sizeof(unsigned) * (w * y + x);
    const auto c =
        (static_cast<unsigned>(rgb[0]) << 16)
        | (static_cast<unsigned>(rgb[1]) << 8)
        | static_cast<unsigned>(rgb[2]);
    std::memcpy(&this->pixels()[i], &c, sizeof(c));
}

}

#endif

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
    using texel4 = Texture::texel4;
    explicit FrameBuffer(Flag f = {});
    /** Size of the frame buffer in pixels. */
    uvec2 size(void) const { return this->m_size; }
    /** Pointer to the content. */
    std::span<char> span(void) { return this->v; }
    std::span<char> prefix(void);
    std::span<char> pixels(void);
    std::span<char> suffix(void);
    /** Changes the size and clears the contents. */
    void resize_and_clear(uvec2 s);
    /** Write pixel at `{x, y}` with \p color. */
    void write(std::size_t x, std::size_t y, Texture::texel4 color);
    /** Inverts the Y coordinate of all pixels. */
    void flip(void);
private:
    static constexpr std::size_t prefix_size_from_flags(Flag f);
    Flag flags;
    std::vector<char> v = {}, flip_tmp = {};
    std::size_t prefix_size;
    uvec2 m_size = {};
};

inline FrameBuffer::FrameBuffer(Flag f) :
    flags{f},
    prefix_size{FrameBuffer::prefix_size_from_flags(f)}
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

inline std::span<char> FrameBuffer::prefix(void) {
    return std::span{this->v}.subspan(0, this->prefix_size);
}

inline std::span<char> FrameBuffer::pixels(void) {
    return {end(this->prefix()), begin(this->suffix())};
}

inline std::span<char> FrameBuffer::suffix(void) {
    return std::span{this->v}.subspan(this->v.size());
}

inline void FrameBuffer::write(
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

}

#endif

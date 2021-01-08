#include "rasterizer.h"

#include "math/vec3.h"
#include "math/vec4.h"

#include "frame_buffer.h"

using namespace nngn::term;
using nngn::u32, nngn::uvec2, nngn::vec2, nngn::vec3, nngn::vec4, nngn::mat4;
using texel3 = Texture::texel3;
using texel4 = Texture::texel4;
using TerminalMode = nngn::Graphics::TerminalMode;

namespace {

/**
 * Transforms world-space vertices into clip space.
 * \return {bl, tr, bt, tt}: vertices / texture coordinates
 */
inline auto to_clip(
    mat4 proj, nngn::Vertex v0, nngn::Vertex v1, auto &&f)
{
    auto bl = (proj * vec4{f(v0.pos), 1}).persp_div();
    auto tr = (proj * vec4{f(v1.pos), 1}).persp_div();
    auto bl_c = v0.color, tr_c = v1.color;
    if(tr.x < bl.x)
        std::swap(bl.x, tr.x), std::swap(bl_c[0], tr_c[0]);
    if(tr.y < bl.y)
        std::swap(bl.y, tr.y), std::swap(bl_c[1], tr_c[1]);
    return std::array{bl, tr, bl_c, tr_c};
}

/** Checks whether a sprite lies entirely outside of the clip volume. */
bool clip(vec3 bl, vec3 tr) {
    return tr.x < -1 || tr.y < -1 || 1 <= bl.x || 1 <= bl.y;
}

/** Clamps value between \c 0 and \p max, rounds toward \c -infinity. */
std::size_t clamp_floor(float max, float v) {
    return static_cast<std::size_t>(std::floor(std::clamp(v, 0.0f, max)));
}

/** Clamps value between \c 0 and \p max, rounds toward \c +infinity. */
std::size_t clamp_ceil(float max, float v) {
    return static_cast<std::size_t>(std::ceil(std::clamp(v, 0.0f, max)));
}

/** Calculates texture coordinates for a given position in a sprite. */
float uv_coord(float p0, std::size_t p, float p1, float t0, float t1) {
    const auto fp = static_cast<float>(p);
    assert(p0 <= fp && fp <= p1);
    const auto t = std::clamp((fp - p0) / (p1 - p0), 0.0f, 1.0f);
    return t0 + (t1 - t0) * t;
}

/** Transforms clip-space vertices into screen space. */
vec2 to_screen(uvec2 size, vec3 v) {
    const auto fs = static_cast<vec2>(size);
    const auto centered = v / 2.0f + vec3{0.5f, 0.5f, 0};
    const auto scaled = vec3{fs, 1} * centered;
    return scaled.xy();
}

using write_f = void(FrameBuffer::*)(std::size_t, std::size_t, texel4);

write_f write_fn_for_mode(nngn::Graphics::TerminalMode m) {
    using enum nngn::Graphics::TerminalMode;
    switch(m) {
    case ASCII: return &FrameBuffer::write_ascii;
    case COLORED: return &FrameBuffer::write_colored;
    default: assert(!"invalid mode"); return nullptr;
    }
}

void sprite(
    std::span<const nngn::Vertex> vbo, std::span<const u32> ebo, mat4 proj,
    std::span<const Texture> textures, FrameBuffer *fb, TerminalMode mode,
    auto &&pos_f)
{
    constexpr std::ptrdiff_t n_verts = 6;
    const auto wf = write_fn_for_mode(mode);
    const uvec2 size = fb->size();
    const auto screen_size = static_cast<vec2>(size.xy() - 1u);
    const auto e = end(ebo);
    // TODO use entire EBO when proper rasterization is implemented
    assert(!(ebo.size() % n_verts));
    for(auto b = begin(ebo); b != e; b += n_verts) {
        const auto vbo_idx0 = b[0];
        const auto vbo_idx1 = b[n_verts - 1];
        assert(vbo_idx0 < vbo.size() && vbo_idx1 < vbo.size());
        const auto [clip_bl, clip_tr, bl_uv, tr_uv] =
            to_clip(proj, vbo[vbo_idx0], vbo[vbo_idx1], pos_f);
        if(clip(clip_bl, clip_tr))
            continue;
        const auto screen_bl = to_screen(size, clip_bl);
        const auto screen_tr = to_screen(size, clip_tr);
        if(screen_size.x < screen_bl.x || screen_size.y < screen_bl.y)
            continue;
        const auto xb = clamp_ceil(screen_size.x, screen_bl.x);
        const auto yb = clamp_ceil(screen_size.y, screen_bl.y);
        const auto xe = clamp_floor(screen_size.x, screen_tr.x);
        const auto ye = clamp_floor(screen_size.y, screen_tr.y);
        const auto tex_i = static_cast<std::size_t>(bl_uv[2]);
        assert(tex_i < textures.size());
        const auto &tex = textures[tex_i];
        for(auto y = yb; y <= ye; ++y) {
            const auto v =
                uv_coord(screen_bl.y, y, screen_tr.y, bl_uv.y, tr_uv.y);
            for(auto x = xb; x <= xe; ++x) {
                const auto u =
                    uv_coord(screen_bl.x, x, screen_tr.x, bl_uv.x, tr_uv.x);
                (fb->*wf)(x, y, tex.sample({u, v}));
            }
        }
    }
}

}

namespace nngn::term {

void Rasterizer::update_camera(
    uvec2 term_size, uvec2 window_size,
    mat4 proj, mat4 hud_proj, mat4 view)
{
    const auto fsize = static_cast<vec2>(term_size);
    const auto s = fsize / static_cast<vec2>(window_size);
    const auto scale = nngn::mat4{
        s.x,   0, 0, 0,
          0, s.y, 0, 0,
          0,   0, 1, 0,
          0,   0, 0, 1,
    };
    this->m_proj = proj * scale * view;
    this->m_hud_proj = hud_proj * scale;
}

void Rasterizer::sprite(
    std::span<const nngn::Vertex> vbo, std::span<const u32> ebo, mat4 proj,
    std::span<const Texture> textures, FrameBuffer *fb) const
{
    return ::sprite(vbo, ebo, proj, textures, fb, this->mode, std::identity{});
}

void Rasterizer::font(
    std::span<const nngn::Vertex> vbo, std::span<const u32> ebo, mat4 proj,
    std::span<const Texture> font, FrameBuffer *fb) const
{
    return ::sprite(
        vbo, ebo, proj, font, fb, this->mode,
        [](auto x) { return vec3{x.xy()}; });
}

}

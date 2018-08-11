#ifndef NNGN_RENDER_GEN_H
#define NNGN_RENDER_GEN_H

#include "collision/colliders.h"
#include "font/font.h"
#include "font/text.h"
#include "font/textbox.h"
#include "graphics/graphics.h"

#include "light.h"
#include "renderers.h"

namespace nngn {

struct Gen {
    // Primitives
    static void quad_indices(u64 i, u64 n, u32 *p);
    static void quad_vertices(
        Vertex **p, vec2 bl, vec2 tr, float z, vec3 norm,
        u32 tex, vec2 uv0, vec2 uv1);
    static void quad_vertices(
        Vertex **p, vec2 bl, vec2 tr, float z, vec3 norm, vec3 color);
    static void quad_vertices_persp(
        Vertex **p, vec2 bl, vec2 tr, float y, vec3 norm,
        u32 tex, vec2 uv0, vec2 uv1);
    static inline void quad_vertices_zsprite(
        Vertex **p, vec2 bl, vec2 tr, float z, vec3 norm,
        u32 tex, vec2 uv0, vec2 uv1);
    static void cube_vertices(Vertex **p, vec3 pos, vec3 size, vec3 color);
    static void cube_vertices(
        Vertex **p, vec3 pos, vec3 size,
        u32 tex, const std::array<vec4, 6> &uv);
    // Utilities
    static float text_color(u8 r, u8 g, u8 b);
    static float text_color(u32 c);
    // Renderers
    static void sprite_ortho(Vertex **p, SpriteRenderer *x);
    static void sprite_orthoz(Vertex **p, SpriteRenderer *x);
    static void sprite_persp(Vertex **p, SpriteRenderer *x);
    static void cube_ortho(Vertex **p, CubeRenderer *x);
    static void cube_persp(Vertex **p, CubeRenderer *x);
    static void voxel_ortho(Vertex **p, VoxelRenderer *x);
    static void voxel_persp(Vertex **p, VoxelRenderer *x);
    static void sprite_debug(Vertex **p, SpriteRenderer *x);
    static void cube_debug(Vertex **p, CubeRenderer *x);
    static void voxel_debug(Vertex **p, VoxelRenderer *x);
    static void text(
        Vertex **p, u64 n,
        const Font &font, const Text &txt, bool mono, float left,
        vec2 *pos_p, float *color_p, u64 *i_p, u64 *n_visible_p);
    static void textbox(Vertex **p, const Textbox &x);
    static void selection(Vertex **p, const SpriteRenderer &x);
    static void aabb(Vertex **p, const AABBCollider &x, vec3 color);
    static void aabb_circle(Vertex **p, const AABBCollider &x);
    static void bb(Vertex **p, const BBCollider &x, vec3 color);
    static void coll_sphere(Vertex **p, const SphereCollider &x);
    static void light(Vertex **p, const Light &x, vec3 pos);
    static void light_range(Vertex **p, const Light &x);
private:
    static constexpr std::array<vec2, 2>
        CIRCLE_UV_32 = {{{ 32/512.0f, 1}, { 64/512.0f, 1 -  32/512.0f}}},
        CIRCLE_UV_64 = {{{128/512.0f, 1}, {256/512.0f, 1 - 128/512.0f}}};
};

inline void Gen::quad_indices(u64 i_64, u64 n, u32 *p) {
    constexpr std::size_t n_verts = 4, log2 = std::countr_zero(n_verts);
    auto i = static_cast<u32>(i_64) << log2;
    for(const auto e = i + (n << log2); i != e; i += n_verts) {
        *p++ = i    ; *(p++) = i + 1; *(p++) = i + 2;
        *p++ = i + 2; *(p++) = i + 1; *(p++) = i + 3;
    }
}

inline void Gen::quad_vertices(
    Vertex **pp, vec2 bl, vec2 tr, float z, vec3 norm, vec3 color
) {
    auto *p = *pp;
    *p++ = {{bl,         z}, norm, color};
    *p++ = {{tr.x, bl.y, z}, norm, color};
    *p++ = {{bl.x, tr.y, z}, norm, color};
    *p++ = {{tr,         z}, norm, color};
    *pp = p;
}

inline void Gen::quad_vertices(
    Vertex **pp, vec2 bl, vec2 tr, float z, vec3 norm,
    u32 tex, vec2 uv0, vec2 uv1
) {
    const auto tex_f = static_cast<float>(tex);
    auto *p = *pp;
    *p++ = {{bl,         z}, norm, {uv0.x, uv0.y, tex_f}};
    *p++ = {{tr.x, bl.y, z}, norm, {uv1.x, uv0.y, tex_f}};
    *p++ = {{bl.x, tr.y, z}, norm, {uv0.x, uv1.y, tex_f}};
    *p++ = {{tr,         z}, norm, {uv1.x, uv1.y, tex_f}};
    *pp = p;
}

inline void Gen::quad_vertices_persp(
    Vertex **pp, vec2 bl, vec2 tr, float y, vec3 norm,
    u32 tex, vec2 uv0, vec2 uv1
) {
    const auto tex_f = static_cast<float>(tex);
    auto *p = *pp;
    *p++ = {{bl.x, y, bl.y}, norm, {uv0.x, uv0.y, tex_f}};
    *p++ = {{tr.x, y, bl.y}, norm, {uv1.x, uv0.y, tex_f}};
    *p++ = {{bl.x, y, tr.y}, norm, {uv0.x, uv1.y, tex_f}};
    *p++ = {{tr.x, y, tr.y}, norm, {uv1.x, uv1.y, tex_f}};
    *pp = p;
}

inline void Gen::quad_vertices_zsprite(
    Vertex **pp, vec2 bl, vec2 tr, float z, vec3 norm,
    u32 tex, vec2 uv0, vec2 uv1
) {
    const auto tex_f = static_cast<float>(tex);
    auto *p = *pp;
    *p++ = {{bl.x, bl.y, z}, norm, {uv0.x, uv0.y, tex_f}};
    *p++ = {{tr.x, bl.y, z}, norm, {uv1.x, uv0.y, tex_f}};
    *p++ = {{bl.x, tr.y, z + tr.y - bl.y}, norm, {uv0.x, uv1.y, tex_f}};
    *p++ = {{tr.x, tr.y, z + tr.y - bl.y}, norm, {uv1.x, uv1.y, tex_f}};
    *pp = p;
}

inline void Gen::cube_vertices(Vertex **pp, vec3 pos, vec3 size, vec3 color) {
    const auto s = size / 2.0f;
    const auto bl = pos - s, tr = pos + s;
    auto *p = *pp;
    *p++ = { bl               , { 0,  0, -1}, color};
    *p++ = {{bl.x, tr.y, bl.z}, { 0,  0, -1}, color};
    *p++ = {{tr.x, bl.y, bl.z}, { 0,  0, -1}, color};
    *p++ = {{tr.x, tr.y, bl.z}, { 0,  0, -1}, color};
    *p++ = {{bl.x, bl.y, tr.z}, { 0,  0,  1}, color};
    *p++ = {{tr.x, bl.y, tr.z}, { 0,  0,  1}, color};
    *p++ = {{bl.x, tr.y, tr.z}, { 0,  0,  1}, color};
    *p++ = { tr               , { 0,  0,  1}, color};
    *p++ = { bl               , {-1,  0,  0}, color};
    *p++ = {{bl.x, bl.y, tr.z}, {-1,  0,  0}, color};
    *p++ = {{bl.x, tr.y, bl.z}, {-1,  0,  0}, color};
    *p++ = {{bl.x, tr.y, tr.z}, {-1,  0,  0}, color};
    *p++ = {{tr.x, bl.y, bl.z}, { 1,  0,  0}, color};
    *p++ = {{tr.x, tr.y, bl.z}, { 1,  0,  0}, color};
    *p++ = {{tr.x, bl.y, tr.z}, { 1,  0,  0}, color};
    *p++ = { tr               , { 1,  0,  0}, color};
    *p++ = { bl               , { 0, -1,  0}, color};
    *p++ = {{tr.x, bl.y, bl.z}, { 0, -1,  0}, color};
    *p++ = {{bl.x, bl.y, tr.z}, { 0, -1,  0}, color};
    *p++ = {{tr.x, bl.y, tr.z}, { 0, -1,  0}, color};
    *p++ = {{bl.x, tr.y, bl.z}, { 0,  1,  0}, color};
    *p++ = {{bl.x, tr.y, tr.z}, { 0,  1,  0}, color};
    *p++ = {{tr.x, tr.y, bl.z}, { 0,  1,  0}, color};
    *p++ = { tr               , { 0,  1,  0}, color};
    *pp = p;
}

inline void Gen::cube_vertices(
    Vertex **p, vec3 pos, vec3 size,
    u32 tex, const std::array<vec4, 6> &uv
) {
    const auto ftex = static_cast<float>(tex);
    const auto s = size / 2.0f;
    const auto bl = pos - s, tr = pos + s;
    auto uv_ = uv[5];
    *((*p)++) = { bl               , { 0,  0, -1}, {uv_.xy(), ftex}};
    *((*p)++) = {{bl.x, tr.y, bl.z}, { 0,  0, -1}, {uv_.xw(), ftex}};
    *((*p)++) = {{tr.x, bl.y, bl.z}, { 0,  0, -1}, {uv_.zy(), ftex}};
    *((*p)++) = {{tr.x, tr.y, bl.z}, { 0,  0, -1}, {uv_.zw(), ftex}};
    uv_ = uv[4];
    *((*p)++) = {{bl.x, bl.y, tr.z}, { 0,  0,  1}, {uv_.xy(), ftex}};
    *((*p)++) = {{tr.x, bl.y, tr.z}, { 0,  0,  1}, {uv_.zy(), ftex}};
    *((*p)++) = {{bl.x, tr.y, tr.z}, { 0,  0,  1}, {uv_.xw(), ftex}};
    *((*p)++) = { tr               , { 0,  0,  1}, {uv_.zw(), ftex}};
    uv_ = uv[1];
    *((*p)++) = { bl               , {-1,  0,  0}, {uv_.xy(), ftex}};
    *((*p)++) = {{bl.x, bl.y, tr.z}, {-1,  0,  0}, {uv_.xw(), ftex}};
    *((*p)++) = {{bl.x, tr.y, bl.z}, {-1,  0,  0}, {uv_.zy(), ftex}};
    *((*p)++) = {{bl.x, tr.y, tr.z}, {-1,  0,  0}, {uv_.zw(), ftex}};
    uv_ = uv[0];
    *((*p)++) = {{tr.x, bl.y, bl.z}, { 1,  0,  0}, {uv_.xy(), ftex}};
    *((*p)++) = {{tr.x, tr.y, bl.z}, { 1,  0,  0}, {uv_.zy(), ftex}};
    *((*p)++) = {{tr.x, bl.y, tr.z}, { 1,  0,  0}, {uv_.xw(), ftex}};
    *((*p)++) = { tr               , { 1,  0,  0}, {uv_.zw(), ftex}};
    uv_ = uv[3];
    *((*p)++) = { bl               , { 0, -1,  0}, {uv_.xy(), ftex}};
    *((*p)++) = {{tr.x, bl.y, bl.z}, { 0, -1,  0}, {uv_.zy(), ftex}};
    *((*p)++) = {{bl.x, bl.y, tr.z}, { 0, -1,  0}, {uv_.xw(), ftex}};
    *((*p)++) = {{tr.x, bl.y, tr.z}, { 0, -1,  0}, {uv_.zw(), ftex}};
    uv_ = uv[2];
    *((*p)++) = {{bl.x, tr.y, bl.z}, { 0,  1,  0}, {uv_.xy(), ftex}};
    *((*p)++) = {{bl.x, tr.y, tr.z}, { 0,  1,  0}, {uv_.xw(), ftex}};
    *((*p)++) = {{tr.x, tr.y, bl.z}, { 0,  1,  0}, {uv_.zy(), ftex}};
    *((*p)++) = { tr               , { 0,  1,  0}, {uv_.zw(), ftex}};
}

inline float Gen::text_color(u8 r, u8 g, u8 b) {
    return Gen::text_color((static_cast<u32>(r) << 16)
        | (static_cast<u32>(g) << 8)
        | static_cast<u32>(b));
}

inline float Gen::text_color(u32 c) {
    float ret = {};
    static_assert(sizeof(ret) == sizeof(c));
    std::memcpy(&ret, &c, sizeof(ret));
    return ret;
}

inline void Gen::sprite_ortho(Vertex **p, SpriteRenderer *x) {
    x->flags.clear(Renderer::Flag::UPDATED);
    const auto pos = x->pos.xy();
    const auto s = x->size / 2.0f;
    Gen::quad_vertices(
        p, pos - s, pos + s, -pos.y - x->z_off,
        {0, 0, 1}, x->tex, x->uv0, x->uv1);
}

inline void Gen::sprite_orthoz(Vertex **p, SpriteRenderer *x) {
    constexpr auto nl = Math::sq2_2<float>();
    x->flags.clear(Renderer::Flag::UPDATED);
    const auto pos = x->pos.xy();
    const auto s = x->size / 2.0f;
    Gen::quad_vertices_zsprite(
        p, pos - s, pos + s, -(s.y + x->z_off),
        {0, -nl, nl}, x->tex, x->uv0, x->uv1);
}

inline void Gen::sprite_persp(Vertex **p, SpriteRenderer *x) {
    x->flags.clear(Renderer::Flag::UPDATED);
    const auto s = x->size / 2.0f;
    const vec2 bl = {x->pos.x - s.x, -s.y - x->z_off};
    Gen::quad_vertices_persp(
        p, bl, {x->pos.x + s.x, bl.y + x->size.y},
        x->pos.y + x->z_off, {0, -1, 0}, x->tex, x->uv0, x->uv1);
}

inline void Gen::cube_ortho(Vertex **p, CubeRenderer *x) {
    x->flags.clear(Renderer::Flag::UPDATED);
    Gen::cube_vertices(
        p, {x->pos.x, x->pos.y, x->pos.z - x->pos.y},
        vec3{x->size}, x->color);
}

inline void Gen::cube_persp(Vertex **p, CubeRenderer *x) {
    x->flags.clear(Renderer::Flag::UPDATED);
    Gen::cube_vertices(p, x->pos, vec3{x->size}, x->color);
}

inline void Gen::voxel_ortho(Vertex **p, VoxelRenderer *x) {
    x->flags.clear(Renderer::Flag::UPDATED);
    Gen::cube_vertices(
        p, {x->pos.x, x->pos.y, x->pos.z - x->pos.y},
        x->size, x->tex, x->uv);
}

inline void Gen::voxel_persp(Vertex **p, VoxelRenderer *x) {
    x->flags.clear(Renderer::Flag::UPDATED);
    Gen::cube_vertices(p, x->pos, x->size, x->tex, x->uv);
}

inline void Gen::sprite_debug(Vertex **p, SpriteRenderer *x) {
    constexpr vec3 norm = {0, 0, 1};
    const auto pos = x->pos.xy();
    auto s = x->size / 2.0f;
    vec2 bl = pos - s, tr = pos + s;
    Gen::quad_vertices(p, bl, tr, 0, norm, {1, 1, 1});
    s = {0.5f, 0.5f};
    bl = pos - s;
    tr = pos + s;
    Gen::quad_vertices(p, bl, tr, 0, norm, {1, 0, 0});
    bl.y = x->pos.y + x->z_off - s.y;
    tr.y = x->pos.y + x->z_off + s.y;
    Gen::quad_vertices(p, bl, tr, 0, norm, {1, 0, 0});
}

inline void Gen::cube_debug(Vertex **p, CubeRenderer *x) {
    Gen::cube_vertices(p, x->pos, vec3{x->size}, {1, 1, 1});
}

inline void Gen::voxel_debug(Vertex **p, VoxelRenderer *x) {
    Gen::cube_vertices(p, x->pos, vec3{x->size}, {1, 1, 1});
}

inline void Gen::text(
    Vertex **p, u64 n,
    const Font &font, const Text &txt, bool mono, float left,
    vec2 *pos_p, float *color_p, u64 *i_p, u64 *n_visible_p
) {
    auto pos = *pos_p;
    auto color = *color_p;
    auto i = static_cast<std::size_t>(*i_p);
    auto n_visible = *n_visible_p;
    const auto font_size = static_cast<float>(font.size);
    while(n--) {
        using C = Textbox::Command;
        const auto c = static_cast<unsigned char>(txt.str[i++]);
        switch(c) {
        case '\n': pos = {left, pos.y - font_size - txt.spacing}; continue;
        case C::TEXT_WHITE: color = Gen::text_color(255, 255, 255); continue;
        case C::TEXT_RED: color = Gen::text_color(255, 32, 32); continue;
        case C::TEXT_GREEN: color = Gen::text_color(32, 255, 32); continue;
        case C::TEXT_BLUE: color = Gen::text_color(32, 32, 255); continue;
        }
        const auto fc = font.chars[static_cast<std::size_t>(c)];
        const auto size = static_cast<vec2>(fc.size);
        const auto cpos = pos
            + vec2(font_size / 2, txt.size.y - font_size / 2)
            + static_cast<vec2>(fc.bearing);
        Gen::quad_vertices(
            p, cpos, cpos + size, color,
            {0, 0, 1}, static_cast<u32>(c),
            {0, size.y / font_size}, {size.x / font_size, 0});
        pos.x += mono ? font_size : fc.advance;
        ++n_visible;
    }
    *pos_p = pos;
    *color_p = color;
    *i_p = static_cast<u64>(i);
    *n_visible_p = n_visible;
}

inline void Gen::textbox(Vertex **p, const Textbox &x) {
    constexpr vec3 norm = {0, 0, 1};
    constexpr vec3 title_color = {0, 0, 1};
    constexpr vec3 box_color = {1, 1, 1};
    Gen::quad_vertices(p, x.title_bl, x.title_tr, 0, norm, title_color);
    Gen::quad_vertices(p, x.str_bl, x.str_tr, 0, norm, box_color);
}

inline void Gen::selection(Vertex **p, const SpriteRenderer &x) {
    const auto pos = x.pos.xy();
    const auto s = x.size / 2.0f;
    nngn::Gen::quad_vertices(p, pos - s, pos + s, 0, {0, 0, 1}, {1, 1, 0});
}

inline void Gen::aabb(Vertex **p, const AABBCollider &x, vec3 color) {
    Gen::quad_vertices(p, x.bl, x.tr, 0, {0, 0, 1}, color);
}

inline void Gen::aabb_circle(Vertex **p, const AABBCollider &x) {
    const auto uv = x.radius < 32.0f ? Gen::CIRCLE_UV_32 : Gen::CIRCLE_UV_64;
    Gen::quad_vertices(
        p, x.center - x.radius, x.center + x.radius, 0, {0, 0, 1}, 1,
        uv[0], uv[1]);
}

inline void Gen::bb(Vertex **pp, const BBCollider &x, vec3 color) {
    constexpr vec3 norm = {0, 0, 1};
    std::array<vec2, 4> rot = {{
        {x.bl.x, x.bl.y},
        {x.tr.x, x.bl.y},
        {x.bl.x, x.tr.y},
        {x.tr.x, x.tr.y},
    }};
    const auto pos = x.bl + (x.tr - x.bl) / 2.0f;
    for(auto &r : rot) {
        r -= pos;
        r = {
            r.x * x.cos - r.y * x.sin,
            r.x * x.sin + r.y * x.cos,
        };
        r += pos;
    }
    auto *p = *pp;
    *p++ = {{rot[0], 0}, norm, color};
    *p++ = {{rot[1], 0}, norm, color};
    *p++ = {{rot[2], 0}, norm, color};
    *p++ = {{rot[3], 0}, norm, color};
    *pp = p;
}

inline void Gen::coll_sphere(Vertex **p, const SphereCollider &x) {
    const auto uv = x.r / 2.0f < 32.0f ? CIRCLE_UV_32 : CIRCLE_UV_64;
    const auto pos = x.pos.xy();
    Gen::quad_vertices(p, pos - x.r, pos + x.r, 0, {0, 0, 1}, 1, uv[0], uv[1]);
}

inline void Gen::light(Vertex **p, const Light &x, vec3 pos) {
    Gen::cube_vertices(p, pos, vec3{8}, x.color.xyz());
}

inline void Gen::light_range(Vertex **p, const Light &x) {
    Gen::cube_vertices(p, x.pos, vec3{0.2f * x.range()}, x.color.xyz());
}

}

#endif

#ifndef NNGN_RENDER_GEN_H
#define NNGN_RENDER_GEN_H

#include "collision/colliders.h"
#include "font/font.h"
#include "font/text.h"
#include "font/textbox.h"
#include "graphics/graphics.h"

#include "renderers.h"

namespace nngn {

struct Gen {
    // Primitives
    static void quad_indices(u64 i, u64 n, u32 *p);
    static void quad_vertices(
        Vertex **p, vec2 bl, vec2 tr, float z,
        u32 tex, vec2 uv0, vec2 uv1);
    static void quad_vertices(
        Vertex **p, vec2 bl, vec2 tr, float z, vec3 color);
    static void quad_vertices_persp(
        Vertex **p, vec2 bl, vec2 tr, float y,
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
    static void sprite_persp(Vertex **p, SpriteRenderer *x);
    static void screen_sprite(Vertex **p, SpriteRenderer *x);
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
    Vertex **pp, vec2 bl, vec2 tr, float z, vec3 color
) {
    auto *p = *pp;
    *p++ = {{bl,         z}, color};
    *p++ = {{tr.x, bl.y, z}, color};
    *p++ = {{bl.x, tr.y, z}, color};
    *p++ = {{tr,         z}, color};
    *pp = p;
}

inline void Gen::quad_vertices(
    Vertex **pp, vec2 bl, vec2 tr, float z,
    u32 tex, vec2 uv0, vec2 uv1
) {
    const auto tex_f = static_cast<float>(tex);
    auto *p = *pp;
    *p++ = {{bl,         z}, {uv0.x, uv0.y, tex_f}};
    *p++ = {{tr.x, bl.y, z}, {uv1.x, uv0.y, tex_f}};
    *p++ = {{bl.x, tr.y, z}, {uv0.x, uv1.y, tex_f}};
    *p++ = {{tr,         z}, {uv1.x, uv1.y, tex_f}};
    *pp = p;
}

inline void Gen::quad_vertices_persp(
    Vertex **pp, vec2 bl, vec2 tr, float y,
    u32 tex, vec2 uv0, vec2 uv1
) {
    const auto tex_f = static_cast<float>(tex);
    auto *p = *pp;
    *p++ = {{bl.x, y, bl.y}, {uv0.x, uv0.y, tex_f}};
    *p++ = {{tr.x, y, bl.y}, {uv1.x, uv0.y, tex_f}};
    *p++ = {{bl.x, y, tr.y}, {uv0.x, uv1.y, tex_f}};
    *p++ = {{tr.x, y, tr.y}, {uv1.x, uv1.y, tex_f}};
    *pp = p;
}

inline void Gen::cube_vertices(Vertex **pp, vec3 pos, vec3 size, vec3 color) {
    const auto s = size / 2.0f;
    const auto bl = pos - s, tr = pos + s;
    auto *p = *pp;
    *p++ = { bl               , color};
    *p++ = {{bl.x, tr.y, bl.z}, color};
    *p++ = {{tr.x, bl.y, bl.z}, color};
    *p++ = {{tr.x, tr.y, bl.z}, color};
    *p++ = {{bl.x, bl.y, tr.z}, color};
    *p++ = {{tr.x, bl.y, tr.z}, color};
    *p++ = {{bl.x, tr.y, tr.z}, color};
    *p++ = { tr               , color};
    *p++ = { bl               , color};
    *p++ = {{bl.x, bl.y, tr.z}, color};
    *p++ = {{bl.x, tr.y, bl.z}, color};
    *p++ = {{bl.x, tr.y, tr.z}, color};
    *p++ = {{tr.x, bl.y, bl.z}, color};
    *p++ = {{tr.x, tr.y, bl.z}, color};
    *p++ = {{tr.x, bl.y, tr.z}, color};
    *p++ = { tr               , color};
    *p++ = { bl               , color};
    *p++ = {{tr.x, bl.y, bl.z}, color};
    *p++ = {{bl.x, bl.y, tr.z}, color};
    *p++ = {{tr.x, bl.y, tr.z}, color};
    *p++ = {{bl.x, tr.y, bl.z}, color};
    *p++ = {{bl.x, tr.y, tr.z}, color};
    *p++ = {{tr.x, tr.y, bl.z}, color};
    *p++ = { tr               , color};
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
    *((*p)++) = { bl               , {uv_.xy(), ftex}};
    *((*p)++) = {{bl.x, tr.y, bl.z}, {uv_.xw(), ftex}};
    *((*p)++) = {{tr.x, bl.y, bl.z}, {uv_.zy(), ftex}};
    *((*p)++) = {{tr.x, tr.y, bl.z}, {uv_.zw(), ftex}};
    uv_ = uv[4];
    *((*p)++) = {{bl.x, bl.y, tr.z}, {uv_.xy(), ftex}};
    *((*p)++) = {{tr.x, bl.y, tr.z}, {uv_.zy(), ftex}};
    *((*p)++) = {{bl.x, tr.y, tr.z}, {uv_.xw(), ftex}};
    *((*p)++) = { tr               , {uv_.zw(), ftex}};
    uv_ = uv[1];
    *((*p)++) = { bl               , {uv_.xy(), ftex}};
    *((*p)++) = {{bl.x, bl.y, tr.z}, {uv_.xw(), ftex}};
    *((*p)++) = {{bl.x, tr.y, bl.z}, {uv_.zy(), ftex}};
    *((*p)++) = {{bl.x, tr.y, tr.z}, {uv_.zw(), ftex}};
    uv_ = uv[0];
    *((*p)++) = {{tr.x, bl.y, bl.z}, {uv_.xy(), ftex}};
    *((*p)++) = {{tr.x, tr.y, bl.z}, {uv_.zy(), ftex}};
    *((*p)++) = {{tr.x, bl.y, tr.z}, {uv_.xw(), ftex}};
    *((*p)++) = { tr               , {uv_.zw(), ftex}};
    uv_ = uv[3];
    *((*p)++) = { bl               , {uv_.xy(), ftex}};
    *((*p)++) = {{tr.x, bl.y, bl.z}, {uv_.zy(), ftex}};
    *((*p)++) = {{bl.x, bl.y, tr.z}, {uv_.xw(), ftex}};
    *((*p)++) = {{tr.x, bl.y, tr.z}, {uv_.zw(), ftex}};
    uv_ = uv[2];
    *((*p)++) = {{bl.x, tr.y, bl.z}, {uv_.xy(), ftex}};
    *((*p)++) = {{bl.x, tr.y, tr.z}, {uv_.xw(), ftex}};
    *((*p)++) = {{tr.x, tr.y, bl.z}, {uv_.zy(), ftex}};
    *((*p)++) = { tr               , {uv_.zw(), ftex}};
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
        x->tex, x->uv[0], x->uv[1]);
}

inline void Gen::sprite_persp(Vertex **p, SpriteRenderer *x) {
    x->flags.clear(Renderer::Flag::UPDATED);
    const auto s = x->size / 2.0f;
    const vec2 bl = {x->pos.x - s.x, -s.y - x->z_off};
    Gen::quad_vertices_persp(
        p, bl, {x->pos.x + s.x, bl.y + x->size.y},
        x->pos.y + x->z_off, x->tex, x->uv[0], x->uv[1]);
}

inline void Gen::screen_sprite(Vertex **p, SpriteRenderer *x) {
    x->flags.clear(Renderer::Flag::UPDATED);
    const auto pos = x->pos.xy();
    const auto s = x->size / 2.0f;
    Gen::quad_vertices(
        p, pos - s, pos + s, x->pos.z,
        x->tex, x->uv[0], x->uv[1]);
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
    const auto pos = x->pos.xy();
    auto s = x->size / 2.0f;
    vec2 bl = pos - s, tr = pos + s;
    Gen::quad_vertices(p, bl, tr, 0, {1, 1, 1});
    s = {0.5f, 0.5f};
    bl = pos - s;
    tr = pos + s;
    Gen::quad_vertices(p, bl, tr, 0, {1, 0, 0});
    bl.y = x->pos.y + x->z_off - s.y;
    tr.y = x->pos.y + x->z_off + s.y;
    Gen::quad_vertices(p, bl, tr, 0, {1, 0, 0});
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
            p, cpos, cpos + size, color, static_cast<u32>(c),
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
    constexpr vec3 title_color = {0, 0, 1};
    constexpr vec3 box_color = {1, 1, 1};
    Gen::quad_vertices(p, x.title_bl, x.title_tr, 0, title_color);
    Gen::quad_vertices(p, x.str_bl, x.str_tr, 0, box_color);
}

inline void Gen::selection(Vertex **p, const SpriteRenderer &x) {
    const auto pos = x.pos.xy();
    const auto s = x.size / 2.0f;
    nngn::Gen::quad_vertices(p, pos - s, pos + s, 0, {1, 1, 0});
}

inline void Gen::aabb(Vertex **p, const AABBCollider &x, vec3 color) {
    Gen::quad_vertices(p, x.bl, x.tr, 0, color);
}

inline void Gen::aabb_circle(Vertex **p, const AABBCollider &x) {
    const auto uv = x.radius < 32.0f ? Gen::CIRCLE_UV_32 : Gen::CIRCLE_UV_64;
    Gen::quad_vertices(
        p, x.center - x.radius, x.center + x.radius, 0, 1,
        uv[0], uv[1]);
}

}

#endif

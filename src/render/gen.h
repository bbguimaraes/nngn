#ifndef NNGN_RENDER_GEN_H
#define NNGN_RENDER_GEN_H

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

}

#endif

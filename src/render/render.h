#ifndef NNGN_RENDER_RENDER_H
#define NNGN_RENDER_RENDER_H

#include <bit>
#include <string>
#include <unordered_set>
#include <vector>

#include <sol/forward.hpp>

#include "graphics/graphics.h"
#include "math/vec2.h"
#include "math/vec3.h"
#include "math/vec4.h"
#include "utils/flags.h"

#include "renderers.h"

struct Entity;

namespace nngn {

struct Colliders;
class Fonts;
class Grid;
class Textures;
class Textbox;

class Renderers {
    enum Flag : u8 {
        SPRITES_UPDATED = 1u << 0,
        CUBES_UPDATED = 1u << 1,
        VOXELS_UPDATED = 1u << 2,
        RECT_UPDATED = 1u << 3,
        SELECTION_UPDATED = 1u << 4,
        PERSPECTIVE = 1u << 5,
    };
    Textures *textures = nullptr;
    Graphics *graphics = nullptr;
    const Fonts *fonts = nullptr;
    const Textbox *textbox = nullptr;
    const Grid *grid = nullptr;
    const Colliders *colliders = nullptr;
    Flags<Flag> flags = {};
    std::vector<SpriteRenderer> sprites = {};
    std::vector<CubeRenderer> cubes = {};
    std::vector<VoxelRenderer> voxels = {};
    std::unordered_set<const Renderer*> selections = {};
    u32
        sprite_vbo = {}, sprite_ebo = {},
        cube_vbo = {}, cube_ebo = {},
        cube_debug_vbo = {}, cube_debug_ebo = {},
        voxel_vbo = {}, voxel_ebo = {},
        voxel_debug_vbo = {}, voxel_debug_ebo = {},
        box_vbo = {}, box_ebo = {},
        text_vbo = {}, text_ebo = {},
        textbox_vbo = {}, textbox_ebo = {},
        selection_vbo = {}, selection_ebo = {},
        aabb_vbo = {}, aabb_ebo = {},
        aabb_circle_vbo = {}, aabb_circle_ebo = {};
public:
    enum Debug : u8 {
        RECT = 1u << 0, CIRCLE = 1u << 1, BB = 1u << 2, N_DEBUG = 3,
    };
private:
    Flags<Debug> m_debug = {};
public:
    static void gen_quad_idxs(u64 i, u64 n, u32 *p);
    static void gen_quad_verts(
        Vertex **p, vec2 bl, vec2 tr, float z, u32 tex, vec2 uv0, vec2 uv1);
    static void gen_quad_verts(
        Vertex **p, vec2 bl, vec2 tr, float z, vec3 color);
    static void gen_quad_verts_persp(
        Vertex **p, vec2 bl, vec2 tr, float y,
        u32 tex, vec2 uv0, vec2 uv1);
    static void gen_cube_verts(Vertex **p, vec3 pos, vec3 size, vec3 color);
    static void gen_cube_verts(
        Vertex **p, vec3 pos, vec3 size,
        u32 tex, const std::array<vec4, 6> &uv);
    void init(
        Textures *t, const Fonts *f, const Textbox *tb, const Grid *g,
        const Colliders *c);
    auto max_sprites() const { return this->sprites.capacity(); }
    auto max_cubes() const { return this->cubes.capacity(); }
    auto max_voxels() const { return this->voxels.capacity(); }
    auto debug() const { return this->m_debug.t; }
    bool perspective() const { return this->flags.is_set(Flag::PERSPECTIVE); }
    std::size_t n() const;
    std::size_t n_sprites() const { return this->sprites.size(); }
    std::size_t n_cubes() const { return this->cubes.size(); }
    std::size_t n_voxels() const { return this->voxels.size(); }
    bool selected(const Renderer *p) const;
    bool set_max_sprites(std::size_t n);
    bool set_max_cubes(std::size_t n);
    bool set_max_voxels(std::size_t n);
    bool set_max_text(std::size_t n);
    bool set_max_colliders(std::size_t n);
    void set_debug(std::underlying_type_t<Debug> d);
    void set_perspective(bool p);
    bool set_graphics(Graphics *g);
    Renderer *load(const sol::stack_table &t);
    void remove(Renderer *p);
    void add_selection(const Renderer *p);
    void remove_selection(const Renderer *p);
    bool update();
};

inline void Renderers::gen_quad_idxs(u64 i_64, u64 n, u32 *p) {
    constexpr std::size_t n_verts = 4, log2 = std::countr_zero(n_verts);
    auto i = static_cast<u32>(i_64) << log2;
    for(const auto e = i + (n << log2); i != e; i += n_verts) {
        *p++ = i    ; *(p++) = i + 1; *(p++) = i + 2;
        *p++ = i + 2; *(p++) = i + 1; *(p++) = i + 3;
    }
}

inline void Renderers::gen_quad_verts(
    Vertex **pp, vec2 bl, vec2 tr, float z, vec3 color
) {
    auto *p = *pp;
    *p++ = {{bl,         z}, color};
    *p++ = {{tr.x, bl.y, z}, color};
    *p++ = {{bl.x, tr.y, z}, color};
    *p++ = {{tr,         z}, color};
    *pp = p;
}

inline void Renderers::gen_quad_verts(
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

inline void Renderers::gen_quad_verts_persp(
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

inline void Renderers::gen_cube_verts(
    Vertex **p, vec3 pos, vec3 size, vec3 color
) {
    const auto s = size / 2.0f;
    const auto bl = pos - s, tr = pos + s;
    *((*p)++) = { bl               , color};
    *((*p)++) = {{bl.x, tr.y, bl.z}, color};
    *((*p)++) = {{tr.x, bl.y, bl.z}, color};
    *((*p)++) = {{tr.x, tr.y, bl.z}, color};
    *((*p)++) = {{bl.x, bl.y, tr.z}, color};
    *((*p)++) = {{tr.x, bl.y, tr.z}, color};
    *((*p)++) = {{bl.x, tr.y, tr.z}, color};
    *((*p)++) = { tr               , color};
    *((*p)++) = { bl               , color};
    *((*p)++) = {{bl.x, bl.y, tr.z}, color};
    *((*p)++) = {{bl.x, tr.y, bl.z}, color};
    *((*p)++) = {{bl.x, tr.y, tr.z}, color};
    *((*p)++) = {{tr.x, bl.y, bl.z}, color};
    *((*p)++) = {{tr.x, tr.y, bl.z}, color};
    *((*p)++) = {{tr.x, bl.y, tr.z}, color};
    *((*p)++) = { tr               , color};
    *((*p)++) = { bl               , color};
    *((*p)++) = {{tr.x, bl.y, bl.z}, color};
    *((*p)++) = {{bl.x, bl.y, tr.z}, color};
    *((*p)++) = {{tr.x, bl.y, tr.z}, color};
    *((*p)++) = {{bl.x, tr.y, bl.z}, color};
    *((*p)++) = {{bl.x, tr.y, tr.z}, color};
    *((*p)++) = {{tr.x, tr.y, bl.z}, color};
    *((*p)++) = { tr               , color};
}

inline void Renderers::gen_cube_verts(
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

inline bool Renderers::selected(const Renderer *p) const
    { return this->selections.find(p) != cend(this->selections); }

}

#endif

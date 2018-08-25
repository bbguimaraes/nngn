#ifndef NNGN_RENDER_RENDERERS_H
#define NNGN_RENDER_RENDERERS_H

#include "lua/table.h"
#include "math/vec2.h"
#include "math/vec3.h"
#include "math/vec4.h"
#include "utils/def.h"
#include "utils/flags.h"

struct Entity;

namespace nngn {

struct Renderer {
    enum Type : u8 { SPRITE = 1, CUBE, N_TYPES };
    enum Flag : u8 { UPDATED = 1u << 0 };
    Entity *entity = nullptr;
    vec3 pos = {};
    float z_off = 0;
    Flags<Flag> flags = {};
    bool updated() const { return this->flags.is_set(Flag::UPDATED); }
    void set_pos(vec3 p) { this->pos = p; this->flags |= Flag::UPDATED; }
};

struct SpriteRenderer : Renderer {
    vec2 size = {}, uv0 = {0, 1}, uv1 = {1, 0};
    u32 tex = 0;
    static void uv_coords(
        const uvec2 &uv0, const uvec2 &uv1,
        const uvec2 &scale, vec2 *p);
    void load(const nngn::lua::table &t);
};

struct CubeRenderer : Renderer {
    vec3 color = {1, 1, 1};
    float size = 0;
    void load(const nngn::lua::table &t);
};

inline void SpriteRenderer::uv_coords(
        const uvec2 &uv0, const uvec2 &uv1,
        const uvec2 &scale, vec2 *p) {
    const auto s = static_cast<vec2>(scale);
    const auto c0 = static_cast<vec2>(uv0) / s;
    const auto c1 = static_cast<vec2>(uv1) / s;
    p[0] = {c0.x, 1 - c0.y};
    p[1] = {c1.x, 1 - c1.y};
}

}

#endif

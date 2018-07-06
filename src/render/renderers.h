#ifndef NNGN_RENDER_RENDERERS_H
#define NNGN_RENDER_RENDERERS_H

#include "lua/table.h"
#include "math/vec2.h"
#include "math/vec3.h"
#include "math/vec4.h"
#include "utils/def.h"
#include "utils/flags.h"

namespace nngn {

struct Renderer {
    enum Type : u8 {
        SPRITE = 1,
        N_TYPES,
    };
    enum Flag : u8 { UPDATED = 1u << 0 };
    vec3 pos = {};
    Flags<Flag> flags = {};
    bool updated() const { return this->flags.is_set(Flag::UPDATED); }
    void set_pos(vec3 p) { this->pos = p; this->flags |= Flag::UPDATED; }
};

struct SpriteRenderer : Renderer {
    vec2 size = {};
    void load(nngn::lua::table_view t);
};

}

#endif

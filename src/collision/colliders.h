#ifndef NNGN_COLLISION_COLLIDERS_H
#define NNGN_COLLISION_COLLIDERS_H

#include "lua/table.h"
#include "math/vec2.h"
#include "math/vec3.h"
#include "utils/def.h"
#include "utils/flags.h"

struct Entity;

namespace nngn {

struct Collider {
    enum Type : u8 {
        NONE, AABB, N_TYPES,
    };
    enum Flag : u8 {
        COLLIDING = 1 << 0,
        TRIGGER = 1 << 1,
        SOLID = 1 << 2,
    };
    Entity *entity = nullptr;
    vec3 pos = {}, vel = {};
    float m = {};
    Flags<Flag> flags = {};
    Collider() = default;
    explicit Collider(vec3 p) : pos(p) {}
    Collider(vec3 p, float p_m) : pos(p), m(p_m) {}
    void load(nngn::lua::table_view t);
};

struct AABBCollider : Collider {
    vec2 rel_center = {}, rel_bl = {}, rel_tr = {};
    vec2 center = {}, bl = {}, tr = {};
    float radius = {};
    AABBCollider() = default;
    AABBCollider(vec2 p_bl, vec2 p_tr);
    static void update(std::span<AABBCollider> s);
    void load(nngn::lua::table_view t);
};

}

#endif

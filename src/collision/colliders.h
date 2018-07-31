#ifndef NNGN_COLLISION_COLLIDERS_H
#define NNGN_COLLISION_COLLIDERS_H

#include "lua/table.h"
#include "math/vec2.h"
#include "math/vec3.h"
#include "utils/flags.h"

struct Entity;

namespace nngn {

struct Collider {
    enum Type : uint8_t {
        NONE, AABB, N_TYPES,
    };
    enum Flag : uint8_t { TRIGGER = 0b1, SOLID = 0b10 };
    Entity *entity = nullptr;
    vec3 pos = {}, vel = {};
    float m = {};
    Flags<Flag> flags = {};
    Collider() = default;
    explicit Collider(vec3 p) : pos(p) {}
    Collider(vec3 p, float p_m) : pos(p), m(p_m) {}
    void load(const nngn::lua::table &t);
};

struct AABBCollider : Collider {
    vec2 rel_center = {}, rel_bl = {}, rel_tr = {};
    vec2 center = {}, bl = {}, tr = {};
    float radius = {};
    AABBCollider() = default;
    AABBCollider(vec2 p_bl, vec2 p_tr);
    static void update(size_t n, AABBCollider *v);
    void load(const nngn::lua::table &t);
};

}

#endif

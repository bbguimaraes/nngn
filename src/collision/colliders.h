#ifndef NNGN_COLLISION_COLLIDERS_H
#define NNGN_COLLISION_COLLIDERS_H

#include "lua/table.h"
#include "math/vec2.h"
#include "math/vec3.h"
#include "math/vec4.h"
#include "utils/def.h"
#include "utils/flags.h"

struct Entity;

namespace nngn {

struct Collider {
    enum Type : u8 {
        NONE, AABB, BB, SPHERE, PLANE, GRAVITY, N_TYPES,
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

struct BBCollider : AABBCollider {
    float cos = {}, sin = {};
    BBCollider() = default;
    BBCollider(vec2 p_bl, vec2 p_tr, float p_cos, float p_sin) :
        AABBCollider(p_bl, p_tr), cos(p_cos), sin(p_sin) {}
    static void update(std::span<BBCollider> s);
    void load(nngn::lua::table_view t);
};

struct SphereCollider : Collider {
    float r = 0;
    SphereCollider() = default;
    SphereCollider(vec3 p_pos, float p_r) : Collider(p_pos), r(p_r) {}
    static void update(std::span<SphereCollider> s);
    void load(nngn::lua::table_view t);
};

struct PlaneCollider : Collider {
    vec4 abcd = {};
    PlaneCollider() = default;
    PlaneCollider(vec3 p, vec4 v) : Collider(p), abcd(v) {}
    static void update(std::span<PlaneCollider> s);
    void load(nngn::lua::table_view t);
};

struct GravityCollider : Collider {
    static constexpr float G = 6.674e-11f;
    float max_distance2 = {};
    GravityCollider() = default;
    GravityCollider(vec3 p, float p_m, float max_distance)
        : Collider(p, p_m), max_distance2(max_distance * max_distance) {}
    static void update(std::span<GravityCollider> s);
    void load(const nngn::lua::table &t);
};

}

#endif

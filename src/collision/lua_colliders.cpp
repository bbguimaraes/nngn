#include "lua/state.h"

#include "colliders.h"

using nngn::Collider;

NNGN_LUA_PROXY(Collider,
    "SIZEOF_AABB", nngn::lua::var(sizeof(nngn::AABBCollider)),
    "SIZEOF_BB", nngn::lua::var(sizeof(nngn::BBCollider)),
    "SIZEOF_SPHERE", nngn::lua::var(sizeof(nngn::SphereCollider)),
    "SIZEOF_PLANE", nngn::lua::var(sizeof(nngn::PlaneCollider)),
    "SIZEOF_GRAVITY", nngn::lua::var(sizeof(nngn::GravityCollider)),
    "AABB", nngn::lua::var(Collider::Type::AABB),
    "BB", nngn::lua::var(Collider::Type::BB),
    "SPHERE", nngn::lua::var(Collider::Type::SPHERE),
    "PLANE", nngn::lua::var(Collider::Type::PLANE),
    "GRAVITY", nngn::lua::var(Collider::Type::GRAVITY),
    "TRIGGER", nngn::lua::var(Collider::Flag::TRIGGER),
    "SOLID", nngn::lua::var(Collider::Flag::SOLID),
    "flags", [](const Collider &c) { return c.flags.t; })

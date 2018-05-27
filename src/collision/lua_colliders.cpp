#include "lua/state.h"

#include "colliders.h"

using nngn::Collider;

NNGN_LUA_PROXY(Collider,
    "SIZEOF_AABB", nngn::lua::var(sizeof(nngn::AABBCollider)),
    "SIZEOF_BB", nngn::lua::var(sizeof(nngn::BBCollider)),
    "AABB", nngn::lua::var(Collider::Type::AABB),
    "BB", nngn::lua::var(Collider::Type::BB),
    "TRIGGER", nngn::lua::var(Collider::Flag::TRIGGER),
    "SOLID", nngn::lua::var(Collider::Flag::SOLID),
    "flags", [](const Collider &c) { return c.flags.t; })

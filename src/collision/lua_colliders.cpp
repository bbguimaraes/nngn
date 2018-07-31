#include "lua/state.h"

#include "colliders.h"

using nngn::Collider;

NNGN_LUA_PROXY(Collider,
    "SIZEOF_AABB", nngn::lua::var(sizeof(nngn::AABBCollider)),
    "AABB", nngn::lua::var(Collider::Type::AABB),
    "TRIGGER", nngn::lua::var(Collider::Flag::TRIGGER),
    "SOLID", nngn::lua::var(Collider::Flag::SOLID),
    "flags", [](const Collider &c) { return c.flags.t; })

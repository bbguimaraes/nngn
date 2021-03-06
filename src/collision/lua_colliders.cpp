#include <sol/state_view.hpp>
#include <sol/usertype_proxy.hpp>

#include "luastate.h"

#include "colliders.h"

using nngn::Collider;

NNGN_LUA_PROXY(Collider,
    sol::no_constructor,
    "SIZEOF_AABB", sol::var(sizeof(nngn::AABBCollider)),
    "SIZEOF_BB", sol::var(sizeof(nngn::BBCollider)),
    "SIZEOF_SPHERE", sol::var(sizeof(nngn::SphereCollider)),
    "SIZEOF_PLANE", sol::var(sizeof(nngn::PlaneCollider)),
    "SIZEOF_GRAVITY", sol::var(sizeof(nngn::GravityCollider)),
    "AABB", sol::var(Collider::Type::AABB),
    "BB", sol::var(Collider::Type::BB),
    "SPHERE", sol::var(Collider::Type::SPHERE),
    "PLANE", sol::var(Collider::Type::PLANE),
    "GRAVITY", sol::var(Collider::Type::GRAVITY),
    "TRIGGER", sol::var(Collider::Flag::TRIGGER),
    "SOLID", sol::var(Collider::Flag::SOLID),
    "flags", [](const Collider &c) { return c.flags.t; })

#include "lua/function.h"
#include "lua/register.h"
#include "lua/table.h"

#include "colliders.h"

using nngn::Collider;

namespace {

void register_collider(nngn::lua::table_view t) {
    t["SIZEOF_AABB"] = nngn::narrow<lua_Integer>(sizeof(nngn::AABBCollider));
    t["SIZEOF_BB"] = nngn::narrow<lua_Integer>(sizeof(nngn::BBCollider));
    t["SIZEOF_SPHERE"] =
        nngn::narrow<lua_Integer>(sizeof(nngn::SphereCollider));
    t["SIZEOF_PLANE"] = nngn::narrow<lua_Integer>(sizeof(nngn::PlaneCollider));
    t["AABB"] = Collider::Type::AABB;
    t["BB"] = Collider::Type::BB;
    t["SPHERE"] = Collider::Type::SPHERE;
    t["PLANE"] = Collider::Type::PLANE;
    t["TRIGGER"] = Collider::Flag::TRIGGER;
    t["SOLID"] = Collider::Flag::SOLID;
    t["flags"] = [](const Collider &c) { return *c.flags; };
}

}

NNGN_LUA_DECLARE_USER_TYPE(Collider)
NNGN_LUA_PROXY(Collider, register_collider)

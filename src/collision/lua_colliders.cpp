#include "lua/register.h"
#include "lua/table.h"

#include "colliders.h"

using nngn::Collider;

namespace {

void register_collider(nngn::lua::table_view t) {
    t["SIZEOF_AABB"] = nngn::narrow<lua_Integer>(sizeof(nngn::AABBCollider));
    t["AABB"] = Collider::Type::AABB;
    t["TRIGGER"] = Collider::Flag::TRIGGER;
}

}

NNGN_LUA_DECLARE_USER_TYPE(Collider)
NNGN_LUA_PROXY(Collider, register_collider)
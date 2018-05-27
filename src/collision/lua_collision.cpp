#include "entity.h"

#include "lua/function.h"
#include "lua/register.h"
#include "lua/table.h"
#include "lua/utils.h"
#include "timing/stats.h"

#include "collision.h"

using nngn::Colliders;
using CollisionBackend = nngn::Colliders::Backend;

NNGN_LUA_DECLARE_USER_TYPE(Entity)
NNGN_LUA_DECLARE_USER_TYPE(nngn::Collider, "Collider")

namespace {
auto native(void) {
    return nngn::Colliders::native_backend().release();
}

template<auto f>
auto size(const Colliders &c) {
    return nngn::narrow<lua_Integer>((c.*f)().size());
}

auto n_colliders(const Colliders &c) {
    return nngn::narrow<lua_Integer>(c.aabb().size() + c.bb().size());
}

auto max_colliders(const Colliders &c) {
    return nngn::narrow<lua_Integer>(c.max_colliders());
}

auto max_collisions(const Colliders &c) {
    return nngn::narrow<lua_Integer>(c.max_collisions());
}

void set_max_colliders(Colliders &c, lua_Integer n) {
    c.set_max_colliders(nngn::narrow<std::size_t>(n));
}

void set_max_collisions(Colliders &c, lua_Integer n) {
    c.set_max_collisions(nngn::narrow<std::size_t>(n));
}

auto collisions(const Colliders &c, nngn::lua::state_view lua) {
    const auto &v = c.collisions();
    const auto n = v.size();
    auto ret = lua.create_table(nngn::narrow<int>(v.size()), 0);
    for(std::size_t i = 0; i < n; ++i) {
        const auto &x = v[i];
        ret[nngn::narrow<lua_Integer>(i + 1)] = nngn::lua::table_array(
            lua, x.entity0, x.entity1,
            nngn::lua::table_array(lua,
                nngn::narrow<lua_Number>(x.force.x),
                nngn::narrow<lua_Number>(x.force.y)));
    }
    return ret;
}

auto stats_names(nngn::lua::state_view lua) {
    return nngn::lua::table_from_range(lua, Colliders::Stats::names).release();
}

auto stats(nngn::lua::state_view lua) {
    auto &v = *nngn::Stats::u64_data<Colliders>();
    const auto n = v.size();
    auto ret = lua.create_table(nngn::narrow<int>(n), 0);
    for(std::size_t i = 0; i != n; ++i)
        ret.raw_set(
            nngn::narrow<lua_Integer>(i + 1),
            nngn::narrow<lua_Integer>(v[i]));
    return ret.release();
}

void set_backend(Colliders &c, Colliders::Backend *p) {
    c.set_backend(std::unique_ptr<Colliders::Backend>{p});
}

void register_backend(nngn::lua::table_view t) {
    t["native"] = native;
}

void register_colliders(nngn::lua::table_view t) {
    t["STATS_IDX"] = nngn::narrow<lua_Integer>(Colliders::STATS_IDX);
    t["STATS_N_EVENTS"] = nngn::narrow<lua_Integer>(Colliders::Stats::N_EVENTS);
    t["stats_names"] = stats_names;
    t["stats"] = stats;
    t["check"] = &Colliders::check;
    t["resolve"] = &Colliders::resolve;
    t["n_aabbs"] = size<&Colliders::aabb>;
    t["n_bbs"] = size<&Colliders::bb>;
    t["n_colliders"] = n_colliders;
    t["n_collisions"] = size<&Colliders::collisions>;
    t["max_colliders"] = max_colliders;
    t["max_collisions"] = max_collisions;
    t["collisions"] = collisions;
    t["has_backend"] = &Colliders::has_backend;
    t["set_check"] = &Colliders::set_check;
    t["set_resolve"] = &Colliders::set_resolve;
    t["set_max_colliders"] = set_max_colliders;
    t["set_max_collisions"] = set_max_collisions;
    t["set_backend"] = set_backend;
    t["load"] = &Colliders::load;
    t["remove"] = &Colliders::remove;
}

}

NNGN_LUA_DECLARE_USER_TYPE(CollisionBackend)
NNGN_LUA_DECLARE_USER_TYPE(Colliders)
NNGN_LUA_PROXY(CollisionBackend, register_backend)
NNGN_LUA_PROXY(Colliders, register_colliders)

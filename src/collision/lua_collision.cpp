#include <lua.hpp>
#include <ElysianLua/elysian_lua_table_proxy.hpp>
#include <ElysianLua/elysian_lua_thread.hpp>
#include <sol/state_view.hpp>
#include <sol/usertype_proxy.hpp>
#include "../xxx_elysian_lua_push_int.h"
#include "../xxx_elysian_lua_push_sol.h"
#include "../xxx_elysian_lua_push_sol_table.h"

#include "entity.h"
#include "luastate.h"

#include "timing/stats.h"

#include "collision.h"

using nngn::Colliders;
using CollisionBackend = nngn::Colliders::Backend;

namespace {

size_t n_colliders(const Colliders &c) {
    return
        c.aabb().size() + c.bb().size()
        + c.sphere().size() + c.plane().size()
        + c.gravity().size();
}

auto collisions(sol::this_state sol, const Colliders &c) {
    using elysian::lua::LuaTableValues;
    using elysian::lua::LuaPair;
    const elysian::lua::ThreadView el(sol);
    const auto &v = c.collisions();
    const auto n = v.size();
    const auto ret = el.createTable(static_cast<int>(n));
    for(size_t i = 0; i < n; ++i) {
        const auto &x = v[i];
        ret.setFieldRaw(i + 1, LuaTableValues{
            LuaPair{1, sol_usertype_wrapper(x.entity0)},
            LuaPair{2, sol_usertype_wrapper(x.entity1)},
            LuaPair{3,
                el.createTable(LuaTableValues{
                    LuaPair{1, x.force.x},
                    LuaPair{2, x.force.y}})}});
    }
    return ret;
}

auto stats_names() { return sol::as_table(Colliders::Stats::names); }
auto stats() { return sol::as_table(*nngn::Stats::u64_data<Colliders>()); }

}

NNGN_LUA_PROXY(CollisionBackend,
    "native", nngn::Colliders::native_backend,
    "compute", nngn::Colliders::compute_backend)
NNGN_LUA_PROXY(Colliders,
    sol::no_constructor,
    "STATS_IDX", sol::var(Colliders::STATS_IDX),
    "STATS_N_EVENTS", sol::var(Colliders::Stats::N_EVENTS),
    "stats_names", stats_names,
    "stats", stats,
    "check", &Colliders::check,
    "resolve", &Colliders::resolve,
    "n_aabb", [](const Colliders &c) { return c.aabb().size(); },
    "n_bb", [](const Colliders &c) { return c.bb().size(); },
    "n_sphere", [](const Colliders &c) { return c.sphere().size(); },
    "n_plane", [](const Colliders &c) { return c.plane().size(); },
    "n_gravity", [](const Colliders &c) { return c.gravity().size(); },
    "n_colliders", n_colliders,
    "n_collisions", [](const Colliders &c) { return c.collisions().size(); },
    "max_colliders", &Colliders::max_colliders,
    "max_collisions", &Colliders::max_collisions,
    "collisions", collisions,
    "has_backend", &Colliders::has_backend,
    "set_check", &Colliders::set_check,
    "set_resolve", &Colliders::set_resolve,
    "set_max_colliders", &Colliders::set_max_colliders,
    "set_max_collisions", &Colliders::set_max_collisions,
    "set_backend", [](Colliders &c, std::unique_ptr<Colliders::Backend> &p)
        { c.set_backend(std::move(p)); },
    "load", &Colliders::load,
    "remove", &Colliders::remove)

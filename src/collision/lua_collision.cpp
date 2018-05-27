#include <sol/state_view.hpp>
#include <sol/usertype_proxy.hpp>

#include "entity.h"
#include "luastate.h"

#include "timing/stats.h"

#include "collision.h"

using nngn::Colliders;
using CollisionBackend = nngn::Colliders::Backend;

namespace {

size_t n_colliders(const Colliders &c) {
    return c.aabb().size() + c.bb().size();
}

auto collisions(const Colliders &c, sol::this_state sol) {
    auto lua = sol::state_view(sol);
    const auto &v = c.collisions();
    const auto n = v.size();
    auto ret = sol::stack_table(
        lua, sol::new_table(static_cast<int>(v.size())));
    for(size_t i = 0; i < n; ++i) {
        const auto &x = v[i];
        ret[i + 1] = lua.create_table_with(
            1, x.entity0, 2, x.entity1,
            3, lua.create_table_with(1, x.force.x, 2, x.force.y));
    }
    return ret;
}

auto stats_names() { return sol::as_table(Colliders::Stats::names); }
auto stats() { return sol::as_table(*nngn::Stats::u64_data<Colliders>()); }

}

NNGN_LUA_PROXY(CollisionBackend,
    "native", nngn::Colliders::native_backend)
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

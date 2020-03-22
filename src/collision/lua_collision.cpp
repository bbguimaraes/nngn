#include "entity.h"

#include "lua/state.h"
#include "lua/utils.h"
#include "timing/stats.h"

#include "collision.h"

using nngn::Colliders;
using CollisionBackend = nngn::Colliders::Backend;

using nngn::lua::var;

namespace {

size_t n_colliders(const Colliders &c) {
    return
        c.aabb().size() + c.bb().size()
        + c.sphere().size() + c.plane().size()
        + c.gravity().size();
}

auto collisions(const Colliders &c, nngn::lua::state_arg lua_) {
    auto lua = nngn::lua::state_view(lua_);
    const auto &v = c.collisions();
    const auto n = v.size();
    auto ret = lua.create_table(static_cast<int>(v.size()), 0);
    for(std::size_t i = 0; i < n; ++i) {
        const auto &x = v[i];
        ret[i + 1] = nngn::lua::table_array(
            lua, x.entity0, x.entity1,
            nngn::lua::table_array(lua, x.force.x, x.force.y));
    }
    return ret;
}

auto stats_names() {
    return nngn::lua::as_table(Colliders::Stats::names);
}

auto stats() {
    return nngn::lua::as_table(*nngn::Stats::u64_data<Colliders>());
}

}

using nngn::lua::var;

NNGN_LUA_PROXY(CollisionBackend,
    "native", nngn::Colliders::native_backend)
NNGN_LUA_PROXY(Colliders,
    "STATS_IDX", var(Colliders::STATS_IDX),
    "STATS_N_EVENTS", var(Colliders::Stats::N_EVENTS),
    "stats_names", stats_names,
    "stats", stats,
    "check", &Colliders::check,
    "resolve", &Colliders::resolve,
    "n_aabbs", [](const Colliders &c) { return c.aabb().size(); },
    "n_bbs", [](const Colliders &c) { return c.bb().size(); },
    "n_spheres", [](const Colliders &c) { return c.sphere().size(); },
    "n_planes", [](const Colliders &c) { return c.plane().size(); },
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

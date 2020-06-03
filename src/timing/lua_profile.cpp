#include <lua.hpp>
#include <ElysianLua/elysian_lua_table_proxy.hpp>
#include <ElysianLua/elysian_lua_thread.hpp>
#include "../xxx_elysian_lua_as_table.h"
#include "../xxx_elysian_lua_push_int.h"
#include <sol/state_view.hpp>
#include <sol/usertype_proxy.hpp>
#include "../xxx_elysian_lua_push_sol_table.h"

#include "luastate.h"

#include "profile.h"
#include "stats.h"

using nngn::Profile;

namespace {

auto stats_names(sol::this_state sol) {
    return as_table(elysian::lua::ThreadView(sol), nngn::ProfileStats::names);
}

auto stats() { return sol::as_table(*nngn::Stats::u64_data<nngn::Profile>()); }

auto stats_as_timeline(sol::this_state sol) {
    const auto &v = *nngn::Stats::u64_data<nngn::Profile>();
    const auto n = 2 * v.size();
    const elysian::lua::ThreadView el(sol);
    const auto ret = el.createTable(static_cast<int>(n * 2));
    for(size_t i = 0; i < n; ++i)
        ret.setFieldRaw(i + 1, v[i >> 1]);
    return ret;
}

}

NNGN_LUA_PROXY(Profile,
    sol::no_constructor,
    "STATS_IDX", sol::var(Profile::STATS_IDX),
    "STATS_N_EVENTS", sol::var(Profile::Stats::N_EVENTS),
    "stats_names", stats_names,
    "stats", stats,
    "stats_as_timeline", stats_as_timeline)

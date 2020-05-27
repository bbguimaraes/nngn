#include <sol/state_view.hpp>
#include <sol/usertype_proxy.hpp>

#include "luastate.h"

#include "profile.h"
#include "stats.h"

using nngn::Profile;

namespace {

auto stats_names() { return sol::as_table(nngn::ProfileStats::names); }

auto stats() { return sol::as_table(*nngn::Stats::u64_data<nngn::Profile>()); }

auto stats_as_timeline(sol::this_state sol) {
    const auto &v = *nngn::Stats::u64_data<nngn::Profile>();
    const auto n = 2 * v.size();
    auto ret = sol::stack_table(sol, sol::new_table(static_cast<int>(n)));
    for(size_t i = 0; i < n; ++i)
        ret.raw_set(i + 1, v[i >> 1]);
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

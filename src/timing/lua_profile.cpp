#include "lua/state.h"

#include "profile.h"
#include "stats.h"

using nngn::Profile;

namespace {

auto stats_as_timeline(nngn::lua::state_arg lua) {
    const auto &v = *nngn::Stats::u64_data<nngn::Profile>();
    const auto n = 2 * v.size();
    auto ret = nngn::lua::state_view{lua}.create_table(static_cast<int>(n), 0);
    for(size_t i = 0; i < n; ++i)
        ret.raw_set(i + 1, v[i >> 1]);
    return ret.release();
}

}

NNGN_LUA_PROXY(Profile,
    "STATS_IDX", nngn::lua::var(Profile::STATS_IDX),
    "STATS_N_EVENTS", nngn::lua::var(Profile::Stats::N_EVENTS),
    "stats_names", [] {
        return nngn::lua::as_table(nngn::ProfileStats::names);
    },
    "stats", [] {
        return nngn::lua::as_table(*nngn::Stats::u64_data<nngn::Profile>());
    },
    "stats_as_timeline", stats_as_timeline)

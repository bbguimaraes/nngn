#include "lua/state.h"

#include "profile.h"
#include "stats.h"

using nngn::Profile;

NNGN_LUA_PROXY(Profile,
    "STATS_IDX", nngn::lua::var(Profile::STATS_IDX),
    "STATS_N_EVENTS", nngn::lua::var(Profile::Stats::N_EVENTS),
    "stats_names", [] {
        return nngn::lua::as_table(nngn::ProfileStats::names);
    },
    "stats", [] {
        return nngn::lua::as_table(*nngn::Stats::u64_data<nngn::Profile>());
    })

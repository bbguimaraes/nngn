#include "lua/state.h"

#include "stats.h"

using nngn::Stats;

NNGN_LUA_PROXY(Stats,
    "active", &Stats::active,
    "set_active", &Stats::set_active)

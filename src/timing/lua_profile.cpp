#include <sol/state_view.hpp>
#include <sol/usertype_proxy.hpp>

#include "luastate.h"

#include "profile.h"
#include "stats.h"

using nngn::Profile;

namespace {

auto stats_names() { return sol::as_table(nngn::ProfileStats::names); }

auto stats() { return sol::as_table(*nngn::Stats::u64_data<nngn::Profile>()); }

}

NNGN_LUA_PROXY(Profile,
    sol::no_constructor,
    "STATS_IDX", sol::var(Profile::STATS_IDX),
    "STATS_N_EVENTS", sol::var(Profile::Stats::N_EVENTS),
    "stats_names", stats_names,
    "stats", stats)

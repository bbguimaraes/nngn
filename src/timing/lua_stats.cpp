#include <sol/state_view.hpp>
#include <sol/usertype_proxy.hpp>

#include "luastate.h"

#include "stats.h"

using nngn::Stats;

NNGN_LUA_PROXY(Stats,
    sol::no_constructor,
    "active", &Stats::active,
    "set_active", &Stats::set_active)

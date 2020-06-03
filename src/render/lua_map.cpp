#include <ElysianLua/elysian_lua_thread.hpp>
#include <sol/state_view.hpp>
#include <sol/usertype_proxy.hpp>
#include "../xxx_elysian_lua_push_sol_table.h"

#include "luastate.h"

#include "map.h"

using nngn::Map;

NNGN_LUA_PROXY(Map,
    sol::no_constructor,
    "enabled", &Map::enabled,
    "perspective", &Map::perspective,
    "set_enabled", &Map::set_enabled,
    "set_max", &Map::set_max,
    "load", &Map::load)

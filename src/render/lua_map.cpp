#include "lua/state.h"

#include "map.h"

using nngn::Map;

NNGN_LUA_PROXY(Map,
    "enabled", &Map::enabled,
    "perspective", &Map::perspective,
    "set_enabled", &Map::set_enabled,
    "set_max", &Map::set_max,
    "load", &Map::load)

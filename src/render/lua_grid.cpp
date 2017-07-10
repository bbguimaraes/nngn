#include <sol/state_view.hpp>
#include <sol/usertype_proxy.hpp>

#include "luastate.h"

#include "grid.h"

using nngn::Grid;

NNGN_LUA_PROXY(Grid,
    sol::no_constructor,
    "enabled", &Grid::enabled,
    "set_enabled", &Grid::set_enabled,
    "set_dimensions", &Grid::set_dimensions,
    "set_color", &Grid::set_color)

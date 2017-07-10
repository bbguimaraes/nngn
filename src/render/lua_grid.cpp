#include "lua/state.h"

#include "grid.h"

using nngn::Grid;

NNGN_LUA_PROXY(Grid,
    "enabled", &Grid::enabled,
    "size", &Grid::size,
    "spacing", &Grid::spacing,
    "set_enabled", &Grid::set_enabled,
    "set_dimensions", &Grid::set_dimensions,
    "set_color", &Grid::set_color)

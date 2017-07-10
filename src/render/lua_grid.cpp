#include "lua/function.h"
#include "lua/register.h"
#include "lua/table.h"

#include "grid.h"

using nngn::Grid;

namespace {

auto size(const Grid &g) {
    return nngn::narrow<lua_Integer>(g.size());
}

auto spacing(const Grid &g) {
    return nngn::narrow<lua_Number>(g.spacing());
}

void register_grid(nngn::lua::table_view t) {
    t["enabled"] = &Grid::enabled;
    t["size"] = size;
    t["spacing"] = spacing;
    t["set_enabled"] = &Grid::set_enabled;
    t["set_dimensions"] = &Grid::set_dimensions;
    t["set_color"] = &Grid::set_color;
}

}

NNGN_LUA_DECLARE_USER_TYPE(Grid)
NNGN_LUA_PROXY(Grid, register_grid)

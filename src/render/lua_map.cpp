#include "lua/function.h"
#include "lua/register.h"
#include "lua/table.h"

#include "map.h"

using nngn::Map;

namespace {

void set_max(Map &m, lua_Integer n) {
    m.set_max(nngn::narrow<std::size_t>(n));
}

void register_map(nngn::lua::table_view t) {
    t["enabled"] = &Map::enabled;
    t["perspective"] = &Map::perspective;
    t["set_enabled"] = &Map::set_enabled;
    t["set_max"] = set_max;
    t["load"] = &Map::load;
}

}

NNGN_LUA_DECLARE_USER_TYPE(Map)
NNGN_LUA_PROXY(Map, register_map)

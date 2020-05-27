#include "lua/function.h"
#include "lua/register.h"
#include "lua/table.h"

#include "stats.h"

using nngn::Stats;

namespace {

bool active(lua_Integer i) {
    return Stats::active(nngn::narrow<std::size_t>(i));
}

void set_active(lua_Integer i, bool a) {
    Stats::set_active(nngn::narrow<std::size_t>(i), a);
}

void register_stats(nngn::lua::table_view t) {
    t["active"] = active;
    t["set_active"] = set_active;
}

}

NNGN_LUA_DECLARE_USER_TYPE(Stats)
NNGN_LUA_PROXY(Stats, register_stats)

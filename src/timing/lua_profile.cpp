#include "lua/function.h"
#include "lua/register.h"
#include "lua/table.h"

#include "profile.h"
#include "stats.h"

using nngn::Profile;

namespace {

nngn::lua::table_view to_table(
    nngn::lua::state_view lua, std::ranges::sized_range auto &&r)
{
    auto ret = lua.create_table(static_cast<int>(std::ranges::size(r)), 0);
    lua_Integer i = 1;
    for(auto &&x : FWD(r))
        ret.raw_set(i++, static_cast<lua_Integer>(x));
    return ret.release();
}

void register_profile(nngn::lua::table_view t) {
    t["STATS_IDX"] = static_cast<lua_Integer>(Profile::STATS_IDX);
    t["STATS_N_EVENTS"] = static_cast<lua_Integer>(Profile::Stats::N_EVENTS);
    t["stats_names"] = [](nngn::lua::state_view lua) {
        return nngn::lua::table_from_range(lua, nngn::ProfileStats::names)
            .release();
    };
    t["stats"] = [](nngn::lua::state_view lua) {
        return to_table(lua, *nngn::Stats::u64_data<nngn::Profile>());
    };
}

}

NNGN_LUA_DECLARE_USER_TYPE(Profile)
NNGN_LUA_PROXY(Profile, register_profile)

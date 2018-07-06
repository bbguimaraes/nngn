#include "lua/function.h"
#include "lua/register.h"
#include "lua/table.h"
#include "lua/utils.h"

#include "fps.h"

using nngn::FPS;

namespace {

auto dump(const FPS &fps, nngn::lua::state_view lua) {
    constexpr auto cast = [](auto d) {
        using D = std::chrono::duration<lua_Number, std::milli>;
        return std::chrono::duration_cast<D>(d).count();
    };
    return nngn::lua::table_map(lua,
        "last_dt", cast(fps.avg_hist.back()),
        "min_dt", cast(fps.min_dt),
        "max_dt", cast(fps.max_dt),
        "avg", static_cast<lua_Number>(fps.avg),
        "sec_count", static_cast<lua_Integer>(fps.sec_count),
        "sec_last", static_cast<lua_Integer>(fps.sec_last)
    ).release();
}

void register_fps(nngn::lua::table_view t) {
    t["reset_min_max"] = &FPS::reset_min_max;
    t["dump"] = dump;
}

}

NNGN_LUA_DECLARE_USER_TYPE(FPS)
NNGN_LUA_PROXY(FPS, register_fps)

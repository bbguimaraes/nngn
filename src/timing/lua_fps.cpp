#include <ElysianLua/elysian_lua_thread.hpp>
#include "../xxx_elysian_lua_push_int.h"
#include <sol/state_view.hpp>
#include <sol/usertype_proxy.hpp>

#include "luastate.h"

#include "fps.h"

using nngn::FPS;

namespace {

auto dump(const FPS &fps, sol::this_state sol) {
    const auto cast = [](const auto &d) {
        using D = std::chrono::duration<float, std::milli>;
        return std::chrono::duration_cast<D>(d).count();
    };
    using elysian::lua::LuaTableValues;
    using elysian::lua::LuaPair;
    return elysian::lua::ThreadView(sol)
        .createTable(LuaTableValues{
            LuaPair{"last_dt", cast(fps.avg_hist.back())},
            LuaPair{"min_dt", cast(fps.min_dt)},
            LuaPair{"max_dt", cast(fps.max_dt)},
            LuaPair{"avg", fps.avg},
            LuaPair{"sec_count", fps.sec_count},
            LuaPair{"sec_last", fps.sec_last}});
}

}

NNGN_LUA_PROXY(FPS,
    sol::no_constructor,
    "reset_min_max", &FPS::reset_min_max,
    "dump", dump)

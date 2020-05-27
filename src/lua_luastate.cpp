#include <sol/state_view.hpp>
#include <sol/usertype_proxy.hpp>

#include "luastate.h"

NNGN_LUA_PROXY(LuaState,
    sol::no_constructor,
    "ferror", [](sol::this_state sol)
        { return ferror(*static_cast<FILE**>(lua_touserdata(sol, 1))); })

#include <sol/state_view.hpp>
#include <sol/usertype_proxy.hpp>

#include "luastate.h"

#include "platform.h"

using nngn::Platform;

NNGN_LUA_PROXY(Platform,
    sol::no_constructor,
    "DEBUG", sol::var(Platform::debug))

#include "lua/state.h"

#include "platform.h"

using nngn::Platform;

NNGN_LUA_PROXY(Platform,
    "DEBUG", nngn::lua::var(Platform::debug))

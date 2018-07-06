#include "lua/function.h"
#include "lua/register.h"
#include "lua/table.h"

#include "platform.h"

using nngn::Platform;

namespace {

void register_platform(nngn::lua::table_view t) {
    t["debug"] = Platform::debug;
}

}

NNGN_LUA_DECLARE_USER_TYPE(Platform)
NNGN_LUA_PROXY(Platform, register_platform)

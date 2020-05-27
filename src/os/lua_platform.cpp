#include <cstdlib>

#include "lua/function.h"
#include "lua/register.h"
#include "lua/table.h"

#include "platform.h"

using nngn::Platform;

namespace {

nngn::lua::table_view argv(nngn::lua::state_view lua) {
    return nngn::lua::table_from_range(lua, Platform::argv).release();
}

void setenv_(std::string_view name, std::string_view value) {
    Platform::setenv(name.data(), value.data());
}

bool ferror_(nngn::lua::state_view lua) {
    return ferror(*nngn::chain_cast<FILE**, void*>(lua.get(1)));
}

void clearerr_(nngn::lua::state_view lua) {
    clearerr(*nngn::chain_cast<FILE**, void*>(lua.get(1)));
}

bool set_non_blocking(nngn::lua::state_view lua) {
    return Platform::set_non_blocking(
        *nngn::chain_cast<FILE**, void*>(lua.get(1)));
}

void register_platform(nngn::lua::table_view t) {
    t["EAGAIN"] = EAGAIN;
    t["debug"] = Platform::debug;
    t["errno"] = [] { return errno; };
    t["clear_errno"] = [] { errno = 0; };
    t["argv"] = argv;
    t["setenv"] = setenv_;
    t["ferror"] = ferror_;
    t["clearerr"] = clearerr_;
    t["set_non_blocking"] = set_non_blocking;
}

}

NNGN_LUA_DECLARE_USER_TYPE(Platform)
NNGN_LUA_PROXY(Platform, register_platform)

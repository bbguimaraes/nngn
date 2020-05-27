#include <cstdlib>

#include "lua/state.h"

#include "platform.h"

using nngn::Platform;

namespace {

void setenv_(std::string_view name, std::string_view value) {
    Platform::setenv(name.data(), value.data());
}

bool ferror_(nngn::lua::state_arg lua) {
    return ferror(*static_cast<FILE**>(lua_touserdata(lua, 1)));
}

void clearerr_(nngn::lua::state_arg lua) {
    clearerr(*static_cast<FILE**>(lua_touserdata(lua, 1)));
}

bool set_non_blocking(nngn::lua::state_arg lua) {
    auto *const f = static_cast<FILE**>(lua_touserdata(lua, 1));
    assert(f);
    return Platform::set_non_blocking(*f);
}

}

using nngn::lua::var;

NNGN_LUA_PROXY(Platform,
    "EAGAIN", var(EAGAIN),
    "DEBUG", var(Platform::debug),
    "errno", [] { return errno; },
    "clear_errno", [] { errno = 0; },
    "argv", [] { return nngn::lua::as_table(Platform::argv); },
    "setenv", setenv_,
    "ferror", ferror_,
    "clearerr", clearerr_,
    "set_non_blocking", set_non_blocking)

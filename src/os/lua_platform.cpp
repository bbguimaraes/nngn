#include <cstdlib>

#include <sol/state_view.hpp>
#include <sol/usertype_proxy.hpp>

#include "luastate.h"

#include "platform.h"

using nngn::Platform;

NNGN_LUA_PROXY(Platform,
    sol::no_constructor,
    "DEBUG", sol::var(Platform::debug),
    "HAS_LIBPNG", sol::var(Platform::has_libpng),
    "argv", [] {
        const auto *const p = Platform::argv;
        return sol::as_table(std::vector(p, p + Platform::argc));
    },
    "setenv", [](std::string_view name, std::string_view value)
        { Platform::setenv(name.data(), value.data()); })

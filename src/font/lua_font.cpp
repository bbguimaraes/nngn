#include <string_view>

#include <sol/state_view.hpp>
#include <sol/usertype_proxy.hpp>

#include "luastate.h"

#include "font.h"

using nngn::Fonts;

NNGN_LUA_PROXY(Fonts,
    sol::no_constructor,
    "n", &Fonts::n,
    "load", &Fonts::load)

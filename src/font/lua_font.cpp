#include "lua/state.h"

#include "font.h"

using nngn::Fonts;

NNGN_LUA_PROXY(Fonts,
    "n", &Fonts::n,
    "load", &Fonts::load)

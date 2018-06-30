#include "lua/function.h"
#include "lua/register.h"
#include "lua/table.h"

#include "font.h"

using nngn::Fonts;

namespace {

void register_fonts(nngn::lua::table_view t) {
    t["n"] = [](Fonts &f) { return nngn::narrow<lua_Integer>(f.n()); };
    t["load"] = &Fonts::load;
}

}

NNGN_LUA_DECLARE_USER_TYPE(Fonts)
NNGN_LUA_PROXY(Fonts, register_fonts)

#include "lua/function.h"
#include "lua/register.h"
#include "lua/table.h"

#include "sun.h"

using nngn::Sun;

namespace {

template<float (Sun::*f)(void) const>
auto get(const Sun &s) {
    return nngn::narrow<lua_Number>((s.*f)());
}

template<nngn::vec3 (Sun::*f)(void) const>
auto get(const Sun &s) {
    const auto ret = (s.*f)();
    return std::tuple{
        nngn::narrow<lua_Number>(ret.x),
        nngn::narrow<lua_Number>(ret.y),
        nngn::narrow<lua_Number>(ret.z),
    };
}

void register_sun(nngn::lua::table_view t) {
    t["incidence"] = get<&Sun::incidence>;
    t["time_ms"] = &Sun::time_ms;
    t["set_incidence"] = &Sun::set_incidence;
    t["set_time_ms"] = &Sun::set_time_ms;
    t["dir"] = get<&Sun::dir>;
}

}

NNGN_LUA_DECLARE_USER_TYPE(Sun)
NNGN_LUA_PROXY(Sun, register_sun)

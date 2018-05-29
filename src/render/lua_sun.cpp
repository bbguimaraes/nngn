#include "lua/state.h"

#include "sun.h"

using nngn::Sun;

namespace {

auto dir(const Sun &s) {
    const auto d = s.dir();
    return std::tuple(d.x, d.y, d.z);
}

}

NNGN_LUA_PROXY(Sun,
    "incidence", &Sun::incidence,
    "time_ms", &Sun::time_ms,
    "set_incidence", &Sun::set_incidence,
    "set_time_ms", &Sun::set_time_ms,
    "dir", dir)

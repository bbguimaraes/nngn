#include "lua/function.h"
#include "lua/register.h"
#include "lua/table.h"

#include "timing.h"

using nngn::Timing, nngn::lua::function_view;

namespace {

template<typename P>
auto time(function_view f) {
    using D = std::chrono::duration<Timing::duration::rep, P>;
    return std::chrono::duration_cast<D>(Timing::time(f)).count();
}

template<float (Timing::*f)(void) const>
auto get(const Timing &t) {
    return nngn::narrow<lua_Number>((t.*f)());
}

auto frame(const Timing &t) {
    return nngn::narrow<lua_Integer>(t.frame);
}

void register_timing(nngn::lua::table_view t) {
    t["frame"] = frame;
    t["time_s"] = [](function_view f) { return time<std::ratio<1>>(f); };
    t["time_ms"] = [](function_view f) { return time<std::milli>(f); };
    t["time_ns"] = [](function_view f) { return time<std::nano>(f); };
    t["now_ns"] = &Timing::now_ns;
    t["now_us"] = &Timing::now_us;
    t["now_ms"] = &Timing::now_ms;
    t["now_s"] = &Timing::now_s;
    t["fnow_ns"] = get<&Timing::fnow_ns>;
    t["fnow_us"] = get<&Timing::fnow_us>;
    t["fnow_ms"] = get<&Timing::fnow_ms>;
    t["fnow_s"] = get<&Timing::fnow_s>;
    t["dt_ns"] = &Timing::dt_ns;
    t["dt_us"] = &Timing::dt_us;
    t["dt_ms"] = &Timing::dt_ms;
    t["dt_s"] = &Timing::dt_s;
    t["fdt_ns"] = get<&Timing::fdt_ns>;
    t["fdt_us"] = get<&Timing::fdt_us>;
    t["fdt_ms"] = get<&Timing::fdt_ms>;
    t["fdt_s"] = get<&Timing::fdt_s>;
}

}

NNGN_LUA_DECLARE_USER_TYPE(Timing)
NNGN_LUA_PROXY(Timing, register_timing)

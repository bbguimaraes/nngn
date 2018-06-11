#include "lua/state.h"

#include "timing.h"

using nngn::Timing;
using time_f = std::function<void()>;

namespace {

template<typename P> auto time(const time_f &f) {
    using D = std::chrono::duration<Timing::duration::rep, P>;
    return std::chrono::duration_cast<D>(Timing::time(f)).count();
}

void set_now_ns(Timing &t, Timing::time_point::rep r)
    { t.now = Timing::time_point(Timing::duration(r)); }
void set_dt_ns(Timing &t, Timing::duration::rep r)
    { t.dt = Timing::duration(r); }

}

NNGN_LUA_PROXY(Timing,
    "frame", nngn::lua::readonly(&Timing::frame),
    "time_s", [](const time_f &f) { return time<std::ratio<1>>(f); },
    "time_ms", [](const time_f &f) { return time<std::milli>(f); },
    "time_ns", [](const time_f &f) { return time<std::nano>(f); },
    "now_ns", &Timing::now_ns,
    "now_us", &Timing::now_us,
    "now_ms", &Timing::now_ms,
    "now_s", &Timing::now_s,
    "fnow_ns", &Timing::fnow_ns,
    "fnow_us", &Timing::fnow_us,
    "fnow_ms", &Timing::fnow_ms,
    "fnow_s", &Timing::fnow_s,
    "dt_ns", &Timing::dt_ns,
    "dt_us", &Timing::dt_us,
    "dt_ms", &Timing::dt_ms,
    "dt_s", &Timing::dt_s,
    "fdt_ns", &Timing::fdt_ns,
    "fdt_us", &Timing::fdt_us,
    "fdt_ms", &Timing::fdt_ms,
    "fdt_s", &Timing::fdt_s,
    "scale", [](const Timing &t) { return t.scale; },
    "set_now_ns", set_now_ns,
    "set_dt_ns", set_dt_ns,
    "set_scale", [](Timing &t, float s) { t.scale = s; })

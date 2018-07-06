#include "lua/state.h"
#include "lua/utils.h"

#include "fps.h"

using nngn::FPS;

namespace {

auto dump(const FPS &fps, nngn::lua::state_arg lua) {
    const auto cast = [](const auto &d) {
        using D = std::chrono::duration<float, std::milli>;
        return std::chrono::duration_cast<D>(d).count();
    };
    return nngn::lua::table_map(
        nngn::lua::state_view{lua},
        "last_dt", cast(fps.avg_hist.back()),
        "min_dt", cast(fps.min_dt),
        "max_dt", cast(fps.max_dt),
        "avg", fps.avg,
        "sec_count", fps.sec_count,
        "sec_last", fps.sec_last
    ).release();
}

}

NNGN_LUA_PROXY(FPS,
    "reset_min_max", &FPS::reset_min_max,
    "dump", dump)

#include <sol/state_view.hpp>
#include <sol/usertype_proxy.hpp>

#include "luastate.h"

#include "fps.h"

using nngn::FPS;

namespace {

auto dump(const FPS &fps, sol::this_state sol) {
    const auto cast = [](const auto &d) {
        using D = std::chrono::duration<float, std::milli>;
        return std::chrono::duration_cast<D>(d).count();
    };
    sol::state_view lua = sol;
    return lua.create_table_with(
        "last_dt", cast(fps.avg_hist.back()),
        "min_dt", cast(fps.min_dt),
        "max_dt", cast(fps.max_dt),
        "avg", fps.avg,
        "sec_count", fps.sec_count,
        "sec_last", fps.sec_last);
}

}

NNGN_LUA_PROXY(FPS,
    sol::no_constructor,
    "reset_min_max", &FPS::reset_min_max,
    "dump", dump)

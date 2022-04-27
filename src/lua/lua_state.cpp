#include "alloc.h"
#include "function.h"
#include "register.h"
#include "table.h"

using nngn::lua::state;

namespace {

nngn::lua::table_view alloc_info(nngn::lua::state_view lua) {
   const auto &info =
       *static_cast<nngn::lua::alloc_info*>(lua.allocator().second);
    constexpr auto n = std::tuple_size_v<decltype(info.v)>;
    constexpr auto cast = [](auto x) { return static_cast<lua_Integer>(x); };
    auto ret = lua.create_table(2 * static_cast<int>(n), 0);
    for(std::size_t i = 0; i != n; ++i) {
        ret.raw_set(cast(2 * i + 1), cast(info.v[i].n));
        ret.raw_set(cast(2 * i + 2), cast(info.v[i].bytes));
    }
    return ret.release();
}

void register_state(nngn::lua::table_view t) {
    t["alloc_info"] = alloc_info;
}

}

NNGN_LUA_DECLARE_USER_TYPE(state)
NNGN_LUA_PROXY(state, register_state)

#include "alloc.h"
#include "state.h"
#include "utils.h"

namespace {

auto alloc_info(nngn::lua::state_arg lua_) {
    nngn::lua::state_view lua = {lua_};
    auto *const info =
        static_cast<nngn::lua::alloc_info*>(lua.allocator().second);
    constexpr auto n = std::tuple_size_v<decltype(info->v)>;
    auto ret = lua.create_table(2 * static_cast<int>(n), 0);
    for(std::size_t i = 0; i != n; ++i) {
        ret.raw_set(2 * i + 1, info->v[i].n);
        ret.raw_set(2 * i + 2, info->v[i].bytes);
    }
    return ret.release();
}

}

using nngn::lua::state;

NNGN_LUA_PROXY(state,
    "alloc_info", alloc_info)

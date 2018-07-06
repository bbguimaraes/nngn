#ifndef NNGN_LUA_UTILS_H
#define NNGN_LUA_UTILS_H

#include <lua.hpp>

#include "os/platform.h"
#include "utils/log.h"
#include "utils/scoped.h"
#include "utils/utils.h"

#include "state.h"

namespace nngn::lua {

/** Pops \c n values from the Lua stack at scope exit. */
class defer_pop {
public:
    NNGN_MOVE_ONLY(defer_pop)
    explicit defer_pop(lua_State *L_) : defer_pop{L_, 1} {}
    defer_pop(lua_State *L_, int n_) : L{L_}, n{n_} {}
    ~defer_pop(void) { if(this->n) lua_pop(L, n); }
    void set_n(int n_) { this->n = n_; }
private:
    lua_State *L;
    int n;
};

/** Validates the desired stack top at scope exit. */
inline auto stack_mark(lua_State *L, int extra = 0) {
    if constexpr(!nngn::Platform::debug)
        return nngn::empty{};
    else
        return make_scoped([](auto L_, int t0) {
            if(const int t1 = lua_gettop(L_); t0 != t1)
                Log::l()
                    << "unexpected stack top: "
                    << t1 << " != " << t0 << '\n';
        }, L, lua_gettop(L) + extra);
}

auto table_array(nngn::lua::state_view lua, auto &&...args) {
    constexpr auto n = sizeof...(args);
    auto ret = lua.create_table(n, 0);
    [&ret]<std::size_t ...I>(std::index_sequence<I...>, auto &&...args_) {
        (..., ret.raw_set(I + 1, args_));
    }(std::make_index_sequence<n>{}, FWD(args)...);
    return ret;
}

template<typename ...Ts>
auto table_map(nngn::lua::state_view lua, Ts &&...args) {
    constexpr auto n = sizeof...(args);
    auto ret = lua.create_table(n, 0);
    [&ret]<std::size_t ...I>(std::index_sequence<I...>, auto t) {
        (..., ret.raw_set(
            FWD(std::get<2 * I>(t)),
            FWD(std::get<2 * I + 1>(t)))
        );
    }(std::make_index_sequence<n / 2>{}, std::forward_as_tuple(FWD(args)...));
    return ret;
}

}

#endif

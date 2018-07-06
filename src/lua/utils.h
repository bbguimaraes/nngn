/**
 * \file
 * \brief General utilities for stack manipulation.
 */
#ifndef NNGN_LUA_UTILS_H
#define NNGN_LUA_UTILS_H

#include <lua.hpp>

#include "os/platform.h"
#include "utils/log.h"
#include "utils/scoped.h"
#include "utils/utils.h"

namespace nngn::lua {

/** Resets the top of the stack at scope exit. */
inline auto defer_settop(lua_State *L, int i = -1) {
    return make_scoped(
        std::bind_front(lua_settop, L, i == -1 ? lua_gettop(L) : i));
}

/** Pops `n` values from the Lua stack at scope exit. */
class defer_pop {
public:
    NNGN_MOVE_ONLY(defer_pop)
    explicit defer_pop(lua_State *L_, int n_ = 1) : L{L_}, n{n_} {}
    ~defer_pop(void) { if(this->n) lua_pop(L, n); }
    void set_n(int n_) { this->n = n_; }
private:
    lua_State *L;
    int n;
};

/**
 * Creates a scoped object which verifies the stack top at scope exit.
 * At the end of the object's scope, a message is logged if the current top of
 * the stack is different than `orig + extra`, where `orig` is the top at the
 * time of the `stack_mark` call.
 * \see nngn::scoped
 */
inline auto stack_mark(lua_State *L, int extra = 0) {
    if constexpr(!Platform::debug)
        return empty{};
    else
        return make_scoped([](auto L_, int t0) {
            if(const int t1 = lua_gettop(L_); t0 != t1)
                Log::l()
                    << "unexpected stack top: "
                    << t1 << " != " << t0 << '\n';
        }, L, lua_gettop(L) + extra);
}

}

#endif

#ifndef NNGN_LUA_FUNCTION_H
#define NNGN_LUA_FUNCTION_H

#include <lua.hpp>

#include <sol/stack.hpp>

namespace nngn::lua {

template<typename ...Args>
class c_closure {
public:
    c_closure(lua_CFunction f, Args &&...args);
    lua_CFunction fn(void) const { return this->f; }
    const std::tuple<Args...> &args(void) const { return this->m_args; }
private:
    lua_CFunction f = nullptr;
    std::tuple<Args...> m_args;
};

template<typename ...Args>
c_closure<Args...>::c_closure(lua_CFunction f_, Args &&...args_)
    : f{f_}, m_args{FWD(args_)...} {}

template<typename ...Args>
int sol_lua_push(
    sol::types<c_closure<Args...>>, lua_State *L, const c_closure<Args...> &v
) {
    const auto f = [L, &v]<std::size_t ...I>(std::index_sequence<I...>) {
        (..., push(L, std::get<I>(v.args())));
    };
    constexpr auto n = sizeof...(Args);
    f(std::make_index_sequence<n>());
    lua_pushcclosure(L, v.fn(), n);
    return 1;
}

}

#endif

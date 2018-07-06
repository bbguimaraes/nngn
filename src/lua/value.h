#ifndef NNGN_LUA_VALUE_H
#define NNGN_LUA_VALUE_H

#include <lua.hpp>

#include <sol/stack.hpp>

#include "utils/utils.h"

namespace nngn::lua {

class value_view {
public:
    value_view(void) = default;
    value_view(lua_State *L_, int i_) : L{L_}, i{i_} {}
    lua_State *state(void) const { return this->L; }
    /** Lua stack index. */
    int index(void) const { return this->i; }
    template<typename R = value_view> R push(void) const;
protected:
    lua_State *release(void) { return std::exchange(this->L, {}); }
private:
    lua_State *L = nullptr;
    int i = 0;
};

struct value : value_view {
    NNGN_NO_COPY(value)
    using value_view::value_view;
    value(void) = default;
    value(value_view v) : value_view{v} {}
    value(value &&rhs) noexcept;
    value &operator=(value &&rhs) noexcept;
    ~value(void) { if(this->state()) lua_remove(this->state(), this->index()); }
    value_view release(void) { return {value_view::release(), this->index()}; }
    template<typename R = value> R push(void) const;
};

struct stack_arg : value_view {
    using value_view::value_view;
    explicit stack_arg(value_view v) : value_view{v} {}
    explicit stack_arg(const value &v) : value_view{v} {}
};

template<typename T>
class light_user_data {
public:
    light_user_data(void) = default;
    light_user_data(T v_) : v{v_} {}
    decltype(auto) operator->(void) const { return this->v; }
    T get(void) const { return this->v; }
private:
    T v = nullptr;
};

inline value::value(value &&rhs) noexcept :
    value_view{rhs.value_view::release(), rhs.index()} {}

inline value &value::operator=(value &&rhs) noexcept {
    value_view::operator=(std::move(rhs).release());
    return *this;
}

template<typename R>
R value_view::push(void) const {
    lua_pushvalue(this->L, this->i);
    return {this->L, lua_gettop(this->L)};
}

template<typename R>
R value::push(void) const {
    return value_view::push<R>();
}

inline bool sol_lua_check(
    sol::types<value_view>, lua_State*, int, auto&&, sol::stack::record&
) {
    return true;
}

inline value_view sol_lua_get(
    sol::types<value_view>, lua_State *L, int i, sol::stack::record &tracking
) {
    tracking.use(1);
    return {L, lua_absindex(L, i)};
}

inline int sol_lua_push(
    sol::types<value_view>, lua_State*, const value_view &v
) {
    v.push();
    return 1;
}

inline bool sol_lua_check(
    sol::types<value>, lua_State*, int, auto&&, sol::stack::record&
) {
    return true;
}

inline value sol_lua_get(
    sol::types<value>, lua_State *L, int i, sol::stack::record &tracking
) {
    tracking.use(1);
    return {L, lua_absindex(L, i)};
}

inline int sol_lua_push(sol::types<value>, lua_State*, const value &v) {
    v.push<value_view>();
    return 1;
}

inline bool sol_lua_check(
    sol::types<stack_arg>, lua_State*, int, auto&&, sol::stack::record&
) {
    return true;
}

inline value sol_lua_get(
    sol::types<stack_arg>, lua_State *L, int i, sol::stack::record&
) {
    return {L, lua_absindex(L, i)};
}

inline int sol_lua_push(sol::types<stack_arg>, lua_State*, const stack_arg&) {
    return 1;
}

template<typename T>
bool sol_lua_check(
    sol::types<light_user_data<T>>,
    lua_State *L, int i, auto&&, sol::stack::record&
) {
    return lua_isuserdata(L, i);
}

template<typename T>
light_user_data<T> sol_lua_get(
    sol::types<light_user_data<T>>,
    lua_State *L, int i, sol::stack::record &tracking
) {
    tracking.use(1);
    return {static_cast<T>(lua_touserdata(L, i))};
}

template<typename T>
int sol_lua_push(
    sol::types<light_user_data<T>>, lua_State *L, const light_user_data<T> &v
) {
    lua_pushlightuserdata(L, v.get());
    return 1;
}

}

namespace sol {

template<> struct is_stack_based<nngn::lua::value_view> : std::true_type {};
template<> struct is_stack_based<nngn::lua::value> : std::true_type {};

template<> struct is_stack_based<std::optional<nngn::lua::value_view>>
    : std::true_type {};
template<> struct is_stack_based<std::optional<nngn::lua::value>>
    : std::true_type {};

}

#endif

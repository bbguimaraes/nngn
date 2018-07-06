/**
 * \file
 * \brief Operations on callable types, function call implementation.
 *
 * The main object which implements Lua function calls is \ref
 * nngn::lua::detail::function_base "function_base".  Its `operator()` pushes
 * all arguments onto the stack and performs a `lua_(p)call` (depending on
 * debug/release settings).  \ref nngn::lua::function_view "function_view" and
 * \ref nngn::lua::function_value "function_value" are the equivalent of \ref
 * nngn::lua::value_view "value_view" and \ref nngn::lua::value "value" with the
 * \ref nngn::lua::detail::function_base "function_base" mixin.
 *
 * An alternative group of functions is \ref nngn::lua::call "call", which can
 * be thought of as the reverse of \ref nngn::lua::detail::function_base
 * "function_base": they are used to call C/++ functions and take arguments and
 * return values from/to Lua stack.  All arguments are assumed to be already on
 * the stack; return values are pushed after the execution of the function.
 * Because the types of arguments and return values are inferred from the
 * function (pointer) argument to \ref nngn::lua::call "call", it must be a
 * resolved value: an overloaded function must be cast, a function template must
 * be explicitly instantiated, etc.
 *
 * ## Examples
 *
 * Calling a Lua function:
 *
 * \code{.cpp}
 * lua.dostring("function f(i) return i + 1 end");
 * function_value{lua.globals()["f"]}(42);
 * assert(lua.get(-1) == 43);
 * \endcode
 *
 * Pushing a C/++ function and calling it:
 *
 * \code{.cpp}
 * constexpr auto f = [](state_view l) {
 *     l.globals()["g"] = true;
 *     return 42;
 * };
 * function_value{lua.push(f)}();
 * assert(lua.get(-1) == 42);
 * assert(lua.globals()["g"] == true);
 * \endcode
 *
 * The same, but set and called through Lua:
 *
 * \code{.cpp}
 * lua.globals()["f"] = f;
 * function_value{lua.globals()["f"]}();
 * // …
 * \endcode
 *
 * Calling a C/++ function using stack values and pushing the result:
 *
 * \code{.cpp}
 * lua.push(42);
 * lua.push(43.0);
 * lua.push(call(lua, [](int i, float f) {
 *     return static_cast<float>(i) + f;
 * }));
 * assert(lua.get(-1) == 85.0);
 * \endcode
 */
#ifndef NNGN_LUA_FUNCTION_H
#define NNGN_LUA_FUNCTION_H

#include <optional>
#include <tuple>
#include <utility>

#include <lua.hpp>

#include "utils/log.h"

#include "lua.h"
#include "push.h"
#include "state.h"
#include "utils.h"
#include "value.h"

namespace nngn::lua {

namespace detail {

/** Reference to a function on the stack.  Can be called. */
template<typename CRTP>
struct function_base {
    /** Pushes each argument onto the stack and calls the function. */
    void operator()(auto &&...args) const;
};

}

/**
 * Non-owning reference to a function on the stack.
 * \see nngn::lua::detail::function_base
 */
struct function_view : value_view, detail::function_base<function_view> {
    using value_view::value_view;
};

/**
 * Owning reference to a function on the stack.
 * \see nngn::lua::detail::function_base
 */
struct function_value : value, detail::function_base<function_value> {
    NNGN_MOVE_ONLY(function_value)
    using value::value;
    ~function_value(void) = default;
};

namespace detail {

template<typename CRTP>
void function_base<CRTP>::operator()(auto &&...args) const {
    NNGN_LOG_CONTEXT_CF(function_base);
    const auto &crtp = static_cast<const CRTP&>(*this);
    crtp.state().call(crtp, FWD(args)...);
}

}

/** Calls the regular function \p f with arguments taken from the stack. */
template<typename R, typename ...Args>
R call(lua_State *L, R(*f)(Args...), int i) {
    return std::apply(f, stack_get<std::tuple<Args...>>::get(L, i));
}

/** Calls member function \p f with object/arguments taken from the stack. */
template<typename R, typename T, typename ...Args>
R call(lua_State *L, R(T::*f)(Args...), int i) {
    return std::apply(f, stack_get<std::tuple<T&, Args...>>::get(L, i));
}

/** Calls member function, \c const overload. */
template<typename R, typename T, typename ...Args>
R call(lua_State *L, R(T::*f)(Args...) const, int i) {
    return std::apply(f, stack_get<std::tuple<const T&, Args...>>::get(L, i));
}

/** Calls a stateless lambda. */
template<stateless_lambda T>
decltype(auto) call(lua_State *L, T, int i) {
    return call(L, +T{}, i);
}

static_assert(detail::stack_type<function_view>);
static_assert(detail::stack_type<function_value>);

}

#endif

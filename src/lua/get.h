/**
 * \file
 * \brief Functions for retrieving values from the stack.
 *
 * These implement the low-level retrieval of values from the Lua stack.
 * Regular users almost certainly will want a higher-level interface, such as
 * \ref nngn::lua::get "get" or \ref nngn::lua::state_view::get
 * "state_view::get".  These are the underlying functions used in their
 * implementation.
 *
 * Special handling for custom types can be added by creating \ref
 * nngn::lua::stack_get "stack_get" specializations in the \ref nngn::lua
 * namespace (_but note that to simply treat a type as a user value, follow the
 * instructions in \ref src/lua/register.h "register.h").  The single `static`
 * function `get` should take a `lua_State` and `int` index and return the
 * value.
 */
#ifndef NNGN_LUA_GET_H
#define NNGN_LUA_GET_H

#include <string_view>
#include <tuple>
#include <utility>
#include <vector>

#include "utils/concepts/fundamental.h"

#include "lua.h"
#include "user.h"

namespace nngn::lua {

/** Reads a `T` if a value exists at that index (no type check is performed). */
template<typename T>
struct stack_get<std::optional<T>> {
    static std::optional<T> get(lua_State *L, int i) {
        switch(lua_type(L, i)) {
        case LUA_TNONE:
        case LUA_TNIL:
            return {};
        default:
            return {stack_get<T>::get(L, i)};
        }
    }
};

/** Converts a value to boolean using Lua's rules (i.e. `!(false || nil)`). */
template<>
struct stack_get<bool> {
    static bool get(lua_State *L, int i) {
        return lua_toboolean(L, i);
    }
};

/** Reads a light user data value as `void*`. */
template<std::convertible_to<const void> T>
struct stack_get<T*> {
    static T* get(lua_State *L, int i) {
        return lua_touserdata(L, i);
    }
};

/**
 * Reads a value convertible to `lua_Integer`.
 * Explicit casting is required for types whose values cannot be fully
 * represented by a `lua_Integer`.
 * \see nngn::lua::integer
 */
template<detail::integer T>
struct stack_get<T> {
    static T get(lua_State *L, int i) {
        return static_cast<T>(lua_tointeger(L, i));
    }
};

/**
 * Reads a value convertible to `lua_Number`.
 * Same rules as the `lua_Integer` specialization.
 * \see nngn::lua::number
 */
template<detail::number T>
struct stack_get<T> {
    static T get(lua_State *L, int i) {
        return static_cast<T>(lua_tonumber(L, i));
    }
};

/** Reads an `enum` value if its underlying integral type can be read. */
template<scoped_enum T>
requires(detail::can_get<std::underlying_type_t<T>>)
struct stack_get<T> {
    static T get(lua_State *L, int i) {
        return static_cast<T>(stack_get<std::underlying_type_t<T>>::get(L, i));
    }
};

/** Reads a C string value. */
template<>
struct stack_get<const char*> {
    static const char *get(lua_State *L, int i) {
        return lua_tostring(L, i);
    }
};

/**
 * Reads a string value as ptr+size.
 * The stack value must outlive the returned value.
 */
template<>
struct stack_get<std::string_view> {
    static std::string_view get(lua_State *L, int i) {
        std::size_t n = 0;
        const char *const s = lua_tolstring(L, i, &n);
        return {s, n};
    }
};

/** Reads a string value as ptr+size. */
template<>
struct stack_get<std::string> {
    static std::string get(lua_State *L, int i) {
        return std::string{stack_get<std::string_view>::get(L, i)};
    }
};

/** Reads a `lua_CFunction` value. */
template<>
struct stack_get<lua_CFunction> {
    static lua_CFunction get(lua_State *L, int i) {
        return lua_tocfunction(L, i);
    }
};

/** Reads one of the libraries native stack types (e.g. \ref table). */
template<detail::stack_type T>
struct stack_get<T> {
    static T get(lua_State *L, int i) {
        return {L, i};
    }
};

/**
 * Reads a user type as a full user data value.
 * \see NNGN_LUA_DECLARE_USER_TYPE
 *     A type must be explicitly declared as a user type for this overload to be
 *     used.
 */
template<user_type T>
struct stack_get<T*> {
    static T *get(lua_State *L, int i) {
        return user_data<T*>::get(L, i);
    }
};

/**
 * Reads a user type as a full user data value (reference overload).
 * Same semantics as the pointer overload.
 */
template<user_type T>
struct stack_get<T&> {
    static T &get(lua_State *L, int i) {
        return *stack_get<T*>::get(L, i);
    }
};

/**
 * Reads a sequence of values from the stack.
 * Each individual value must be readable.  Values are read from the range
 * `[i, i + sizeof...(Ts))`.
 */
template<typename ...Ts>
requires(... && detail::can_get<Ts>)
struct stack_get<std::tuple<Ts...>> {
    static std::tuple<Ts...> get(lua_State *L, int i) {
        constexpr auto n = static_cast<int>(sizeof...(Ts));
        return [L, i]<int ...Is>(std::integer_sequence<int, Is...>) {
            return std::tuple<Ts...>{stack_get<Ts>::get(L, i + Is)...};
        }(std::make_integer_sequence<int, n>{});
    }
};

}

#endif

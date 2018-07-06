/**
 * \file
 * \brief Functions for pushing values onto the stack.
 *
 * These implement the low-level sending of values to the Lua stack.  Regular
 * users almost certainly will want a higher-level interface, such as \ref
 * nngn::lua::push "push" or \ref nngn::lua::state_view::push
 * "state_view::push".  These are the underlying functions used in their
 * implementation.
 *
 * Special handling for custom types can be added by creating \ref
 * nngn::lua::stack_push "stack_push" specializations in the \ref nngn::lua
 * namespace (_but note that to simply treat a type as a user value, follow the
 * instructions in \ref src/lua/register.h "register.h").  The single `static`
 * function `push` should take a `lua_State` and a value and return the number
 * of items pushed.
 */
#ifndef NNGN_LUA_PUSH_H
#define NNGN_LUA_PUSH_H

#include <string_view>
#include <tuple>
#include <utility>
#include <variant>

#include "utils/concepts/fundamental.h"

#include "lua.h"
#include "user.h"

namespace nngn::lua {

/** Pushes a `T` if the object contains a value, otherwise nothing. */
template<typename T>
struct stack_push<std::optional<T>> {
    static int push(lua_State *L, std::optional<T> &&o) {
        return o ? stack_push<T>::push(L, *FWD(o)) : 0;
    }
};

/** Pushes the active member of the variant. */
template<typename ...Ts>
struct stack_push<std::variant<Ts...>> {
    static int push(lua_State *L, std::variant<Ts...> &&v) {
        return std::visit(
            [L](auto &&x) { return stack_push<>::push(L, FWD(x)); },
            FWD(v));
    }
};

/**
 * Emits an error.
 * The usual limitations of mixing `lua_error` and C++ apply.
 * \see lua_error
 */
template<typename T>
struct stack_push<error<T>> {
    static int push(lua_State *L, const error<T> &e) {
        return stack_push::push(L, error<T>{e});
    }
    static int push(lua_State *L, error<T> &&e) {
        stack_push<T>::push(L, FWD(e).e);
        return lua_error(L);
    }
};

/** Pushes a Lua `nil` value. */
template<>
struct stack_push<nil_type> {
    static int push(lua_State *L, nil_type) {
        lua_pushnil(L);
        return 1;
    }
};

/** Pushes a value as a Lua boolean. */
template<>
struct stack_push<bool> {
    static int push(lua_State *L, bool b) {
        lua_pushboolean(L, b);
        return 1;
    }
};

/** Pushes a generic pointer as a light user data value. */
template<std::convertible_to<const void> T>
struct stack_push<T*> {
    static int push(lua_State *L, T *p) {
        // `const`ness is lost when a value is later retrieved.
        lua_pushlightuserdata(L, const_cast<void*>(p));
        return 1;
    }
};

/**
 * Pushes a value convertible to `lua_Integer`.
 * Explicit casting is required for types whose values cannot be fully
 * represented by a `lua_Integer`.
 * \see nngn::lua::integer
 */
template<detail::integer T>
struct stack_push<T> {
    static int push(lua_State *L, lua_Integer i) {
        lua_pushinteger(L, i);
        return 1;
    }
};

/**
 * Pushes a value convertible to `lua_Number`.
 * Same rules as the `lua_Integer` specialization.
 * \see nngn::lua::number
 */
template<detail::number T>
struct stack_push<T> {
    static int push(lua_State *L, lua_Number n) {
        lua_pushnumber(L, n);
        return 1;
    }
};

/** Pushes an `enum` value if its underlying integral type can be pushed. */
template<scoped_enum T>
requires(detail::can_push<std::underlying_type_t<T>>)
struct stack_push<T> {
    static int push(lua_State *L, T t) {
        return stack_push<>::push(L, to_underlying(t));
    }
};

/** Pushes a C string value. */
template<>
struct stack_push<const char*> {
    static int push(lua_State *L, const char *s) {
        lua_pushstring(L, s);
        return 1;
    }
};

/** Pushes a ptr+size string value. */
template<>
struct stack_push<std::string_view> {
    static int push(lua_State *L, std::string_view s) {
        lua_pushlstring(L, s.data(), s.size());
        return 1;
    }
};

/** Convenience specialization for the `std::string_view` overload. */
template<>
struct stack_push<std::string> {
    static int push(lua_State *L, const std::string &s) {
        stack_push<std::string_view>::push(L, s);
        return 1;
    }
};

/** Pushes a `lua_CFunction` value. */
template<>
struct stack_push<lua_CFunction> {
    static int push(lua_State *L, lua_CFunction f) {
        lua_pushcfunction(L, f);
        return 1;
    }
};

/**
 * Convenience specialization for the `lua_CFunction` overload.
 * Pushes callables which can be converted to a Lua function.
 */
template<typename T>
requires(std::is_convertible_v<std::decay_t<T>, lua_CFunction>)
struct stack_push<T> {
    static int push(lua_State *L, T f) {
        return stack_push<lua_CFunction>::push(L, f);
    }
};

/** Pushes the call operator of a stateless lambda. */
template<stateless_lambda T>
requires(!std::convertible_to<T, lua_CFunction>)
struct stack_push<T> {
    static int push(lua_State *L, const T&) {
        constexpr auto *f = +T{};
        return stack_push<>::push(L, f);
    }
};

namespace detail {

template<typename R, typename T>
int push_fn_wrapper(lua_State *L, T f) {
    *static_cast<T*>(lua_newuserdatauv(L, sizeof(f), 0)) = f;
    lua_pushcclosure(L, [](lua_State *L_) {
        constexpr auto i = lua_upvalueindex(1);
        auto f_ = *static_cast<T*>(lua_touserdata(L_, i));
        if constexpr(!std::is_void_v<R>)
            return stack_push<>::push(L_, call(L_, f_));
        call(L_, f_);
        return 0;
    }, 1);
    return 1;
}

}

/**
 * Pushes a function pointer.
 * The function will be callable from Lua and will have its arguments and return
 * value automatically handled.
 */
template<typename R, typename ...Args>
struct stack_push<R(*)(Args...)> {
    static int push(lua_State *L, R(*f)(Args...)) {
        return detail::push_fn_wrapper<R>(L, f);
    }
};

/** Same as the previous specialization, but for member functions. */
template<typename R, typename T, typename ...Args>
struct stack_push<R(T::*)(Args...)> {
    static int push(lua_State *L, R(T::*f)(Args...)) {
        return detail::push_fn_wrapper<R>(L, f);
    }
};

/** Same as the previous specialization, but for `const` member functions. */
template<typename R, typename T, typename ...Args>
struct stack_push<R(T::*)(Args...) const> {
    static int push(lua_State *L, R(T::*f)(Args...) const) {
        return detail::push_fn_wrapper<R>(L, f);
    }
};

/** Pushes one of the libraries native stack types (e.g. \ref table). */
template<detail::stack_type T>
struct stack_push<T> {
    static int push([[maybe_unused]] lua_State *L, const T &t) {
        assert(t.state() == L);
        return t.push();
    }
};

/**
 * Pushes a user type as a full user data value.
 * A copy of the value is pushed.
 * \see NNGN_LUA_DECLARE_USER_TYPE
 *     A type must be explicitly declared as a user type for this overload to be
 *     used.
 */
template<user_type T>
struct stack_push<T> {
    static int push(lua_State *L, T t) {
        return user_data<T>::push(L, std::move(t));
    }
};

/**
 * Pushes a user type as a full user data value (pointer specialization).
 * The user data value contains only a pointer to the value.
 */
template<user_type T>
struct stack_push<T*> {
    static int push(lua_State *L, T *t) {
        return user_data<T>::push(L, t);
    }
};

/**
 * Pushes a user type as a full user data value (reference specialization).
 * Same as the pointer overload.
 */
template<user_type T>
struct stack_push<T&> {
    static int push(lua_State *L, T &t) {
        return user_data<T>::push(L, &t);
    }
};

/**
 * Pushes a sequence of values onto the stack.
 * Each individual value must be pushable.
 */
template<typename ...Ts>
requires(... && detail::can_push<Ts>)
struct stack_push<std::tuple<Ts...>> {
    template<typename T>
    requires(std::same_as<std::decay_t<T>, std::tuple<Ts...>>)
    static int push(lua_State *L, T &&t) {
        constexpr auto n = sizeof...(Ts);
        return [L, &t]<std::size_t ...Is>(std::index_sequence<Is...>) {
            return (0 + ...  + stack_push<Ts>::push(L, std::get<Is>(FWD(t))));
        }(std::make_index_sequence<n>{});
    }
};

/**
 * Lua function template which pushes an object's member.
 * Receives a user data value of the containing type and returns a pointer to
 * the member.
 */
template<member_pointer auto p>
int accessor(lua_State *L) {
    assert(lua_gettop(L) || !"called without an object");
    return [L]<typename T, typename M>(M T::*) {
        return stack_push<>::push(L, &(user_data<T>::get(L, 1)->*p));
    }(p);
}

/** Similar to \ref nngn::lua::accessor, but returns the member by value. */
template<member_pointer auto p>
int value_accessor(lua_State *L) {
    assert(lua_gettop(L) || !"called without an object");
    return [L]<typename T, typename M>(M T::*) {
        return stack_push<>::push(L, user_data<T>::get(L, 1)->*p);
    }(p);
}

}

#endif

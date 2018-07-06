/**
 * \dir src/lua
 * \brief Lua library and scripts.
 *
 * This module is divided in three parts:
 *
 * - The C++ files (`*.{h,cpp}`) are a generic library for C++ ←→ Lua
 *   interaction.  It provides a convenient, typed interface built on top of
 *   Lua's C API that can be used to expose functionality from C++ to Lua and
 *   interact with the VM.  The main interfaces are described below.
 * - The Lua files in the `lib/` subdirectory are a convenience library built on
 *   top of the engine in the main `src/` directory.  It both wraps the existing
 *   components in Lua interfaces and augments them with peripheral,
 *   higher-level components.
 * - The Lua files at the root (`*.lua`) are a wrapper around the `lib/`
 *   subdirectory with preset values and functions for common tasks, used mostly
 *   in interactive exploratory and debugging sessions.
 *
 * ## User types
 *
 * See \ref ./register.h "register.h" for information on how to register and
 * manipulate user types and their meta tables.
 *
 * ## Stack
 *
 * Most of the code is centered around manipulating the Lua data stack, which is
 * done automatically using templates in this module.  See the following files
 * for more information:
 *
 * - \ref ./get.h "get.h": \copybrief src/lua/get.h
 * - \ref ./push.h "push.h": \copybrief src/lua/push.h
 * - \ref ./function.h "function.h": \copybrief src/lua/function.h
 * - \ref ./table.h "table.h": \copybrief src/lua/table.h
 * - \ref ./iter.h "iter.h": \copybrief src/lua/iter.h
 * - \ref ./user.h "user.h": \copybrief src/lua/user.h
 * - \ref ./value.h "value.h": \copybrief src/lua/value.h
 * - \ref ./traceback.h "traceback.h": \copybrief src/lua/traceback.h
 * - \ref ./utils.h "utils.h": \copybrief src/lua/utils.h
 */
#ifndef NNGN_LUA_LUA_H
#define NNGN_LUA_LUA_H

#include <numeric>
#include <optional>

#include <lua.hpp>

#include "utils/concepts.h"
#include "utils/fixed_string.h"
#include "utils/fn.h"
#include "utils/ranges.h"
#include "utils/utils.h"

struct lua_State;

namespace nngn::lua {

class global_table;
class state_view;
struct table;
struct value;
class value_view;

#define NNGN_LUA_TYPES(X) \
    X(none, LUA_TNONE) \
    X(nil, LUA_TNIL) \
    X(boolean, LUA_TBOOLEAN) \
    X(light_user_data, LUA_TLIGHTUSERDATA) \
    X(number, LUA_TNUMBER) \
    X(string, LUA_TSTRING) \
    X(table, LUA_TTABLE) \
    X(function, LUA_TFUNCTION) \
    X(user_data, LUA_TUSERDATA) \
    X(thread, LUA_TTHREAD)

/** `LUA_T*` constants as a scoped enumeration. */
enum class type : int {
#define X(n, v) n = v,
    NNGN_LUA_TYPES(X)
#undef X
};

/** Used to push `nil` values onto the stack. */
inline constexpr struct nil_type {} nil;

/** "Pushing" this value causes `lua_error` to be called. */
template<typename T> struct error { T e; };

// XXX clang
template<typename T> error(T) -> error<T>;

/** \ref type values as a sequential array. */
inline constexpr std::array types = [] {
    using U = std::underlying_type_t<type>;
    constexpr auto f = to_underlying<type>;
    constexpr auto b = f(type::none), e = f(type::thread) + 1;
    constexpr auto n = e - b;
    std::array<U, n> u = {};
    std::array<type, n> ret = {};
    std::iota(begin(u), end(u), b);
    std::transform(
        std::ranges::begin(u), std::ranges::end(u),
        std::ranges::begin(ret), to<type>{});
    return ret;
}();
static_assert(is_sequence(types, to_underlying<type>));

/** Maps `LUA_T*` values to \ref type. */
inline constexpr type type_from_lua(int t) {
    assert(to_underlying(type::nil) <= t);
    assert(t <= to_underlying(types.back()));
    constexpr auto b = std::find(begin(types), end(types), type::nil);
    static_assert(b != end(types));
    return b[t];
}

/** Reads a value from the Lua stack. */
template<typename> struct stack_get;
/** Pushes a value onto the Lua stack. */
template<typename = void> struct stack_push;

/** Generic version of \ref stack_push, deduces type from the argument. */
// Note: a function template would remove the need for the `stack_push<>::push`
// circumlocution, but non dependent names (e.g. `stack_push(L, x)`) are bound
// when a template is defined, not instantiated:
//
// https://en.cppreference.com/w/cpp/language/dependent_name#Binding_rules
//
// This meant any members of the overload set defined after a particular use of
// \ref stack_push would not be seen.
template<>
struct stack_push<void> {
    template<typename T>
    static int push(lua_State *L, T &&t) {
        return stack_push<std::decay_t<T>>::push(L, FWD(t));
    }
};

/**
 * Flag which indicates that a type is to be treated as a user type.
 * Specializations should be added (see \ref NNGN_LUA_DECLARE_USER_TYPE) to
 * indicate that values of this type should be read from / pushed onto the stack
 * as Lua full user data values.
 */
template<typename T> inline constexpr auto is_user_type = false;

/** \see is_user_type */
template<typename T> concept user_type = is_user_type<std::decay_t<T>>;

/**
 * Key in the global table where the meta-table for \p T is stored.
 * Actual type is \ref fixed_string, \ref empty is used to make the
 * unspecialized value invalid.
 * \see NNGN_LUA_DECLARE_USER_TYPE
 */
template<typename T> inline constexpr empty metatable_name = {};

template<typename R, typename ...Args>
R call(lua_State *L, R(*f)(Args...), int i = 1);
template<typename R, typename T, typename ...Args>
R call(lua_State *L, R(T::*f)(Args...), int i = 1);
template<typename R, typename T, typename ...Args>
R call(lua_State *L, R(T::*f)(Args...) const, int i = 1);
template<stateless_lambda T>
decltype(auto) call(lua_State *L, T, int i = 1);

namespace detail {

template<typename CRTP, typename T> class table_iter_base;
template<typename T> class table_iter;
template<typename T> class table_seq_iter;

enum op_mode { normal, raw };

/** Tag to relate all \ref table_base instantiations via inheritance. */
struct table_base_tag {};

/** Tag to relate all \ref table_proxy instantiations via inheritance. */
struct table_proxy_tag {};

/** Whether this library knows how to read a \p T value from the stack. */
template<typename T>
inline constexpr bool can_get = requires(lua_State *L) {
    stack_get<T>::get(L, 0);
};

/** Whether this library knows how to push a \p T value onto the stack. */
template<typename T>
inline constexpr bool can_push = requires(lua_State *L, T t) {
    stack_push<T>::push(L, t);
};

/** A type which can be read from / pushed onto the stack. */
template<typename T>
concept stack_value = can_get<T> && can_push<T>;

/** A built-in type from this library with stack manipulation operations. */
template<typename T>
concept stack_type = requires(lua_State *L, int i, T t) {
    { T{L, i} };
    { t.state() } -> std::same_as<state_view>;
    { t.push() } -> std::same_as<int>;
};

/** A type which can be fully represented by a `lua_Integer`. */
template<typename T>
concept integer = convertible_to_strict<T, lua_Integer>;

/** A type which can be fully represented by a `lua_Number`. */
template<typename T>
concept number = convertible_to_strict<T, lua_Number>;

template<typename T> inline constexpr bool is_optional = false;
template<typename T> inline constexpr bool is_optional<std::optional<T>> = true;

}

/**
 * Determines whether a type is a reference to a value on the stack.
 * These will be left on the stack after they are retrieved from a table.
 * Specializations can be added for user-provided types.
 */
template<typename T> inline constexpr bool is_stack_ref = false;

template<typename T>
inline constexpr bool is_stack_ref<std::optional<T>> = is_stack_ref<T>;

/** \see is_stack_ref */
template<typename T> concept stack_ref = is_stack_ref<std::decay_t<T>>;

std::string_view type_str(type t);
/** Default message handler for `lua_pcall`. */
int msgh(lua_State *L);
/** Logs the current data stack. */
void print_stack(lua_State *L);
/** Logs the current call stack. */
void print_traceback(lua_State *L);

inline std::string_view type_str(type t) {
    switch(t) {
#define X(n, _) case type::n: return #n;
    NNGN_LUA_TYPES(X)
#undef X
    default: return "unknown";
    }
}

}

#endif

/**
 * \file
 * \brief Functions/macros for registering user types.
 *
 * The macros \ref NNGN_LUA_DECLARE_USER_TYPE and \ref NNGN_LUA_PROXY can be
 * used to create and automatically register a user data proxy for a type.  The
 * first declares that a type is to be pushed/popped as user data and declares
 * its meta table key in the global table.  Two versions are available:
 *
 * - `NNGN_LUA_DECLARE_USER_TYPE(T)` declares the type `T` with key `"T"` (i.e.
 *   `#T`).
 * - `NNGN_LUA_DECLARE_USER_TYPE(T, "U")` uses a different name for the meta
 *   table, also useful when the argument has other characters (e.g.  namespace
 *   qualifiers).
 *
 * The macro \ref NNGN_LUA_PROXY defines the members of the meta table.  It
 * takes as parameters the type (`T`) and a function which will be called to
 * register the members:
 *
 * \code{.cpp}
 * static void register_T(nngn::lua::table_view t);
 * NNGN_LUA_PROXY(T, register_T)
 * \endcode
 *
 * The second line will arrange for the registration function to be called when
 * the state is initialized (see \ref nngn::lua::static_register).  The \ref
 * nngn::lua::table_view argument is a reference to the meta table which will
 * be:
 *
 * - automatically initialized and placed in the global table prior to
 *   the execution of the registration function
 * - set for all user data objects of type `T` pushed onto the stack
 *
 * \ref NNGN_LUA_PROXY only needs to be called once for each type (the
 * registration must be performed at run time before any user data values are
 * used).  \ref NNGN_LUA_DECLARE_USER_TYPE has to appear in every translation
 * unit before that type is used in stack operations.
 *
 * Below is an example of a registration function:
 *
 * \code{.cpp}
 * // t.h
 * struct S { … };
 *
 * struct T {
 *     static constexpr int constant = 42;
 *     int member_fn(float);
 *     S member_obj;
 * };
 *
 * // lua_t.cpp
 *
 * #include "lua/function.h" // function/method calls
 * #include "lua/state.h" // registration
 * #include "lua/table.h" // table manipulation
 *
 * #include "t.h"
 *
 * static void register_t(nngn::lua::table_view t) {
 *     t["constant"] = T::constant;
 *     t["accessor"] = nngn::lua::accessor<&T::member_obj>;
 *     t["value_accessor"] = nngn::lua::value_accessor<&T::member_obj>;
 *     t["member_fn"] = &T::member_fn;
 *     t["lambda"] = [](float f, double d) { return f + d; };
 *     t["c_fn"] = [](lua_State *L) { lua_error(L, "error"); return 0; };
 * }
 * \endcode
 *
 * The following fields are set in the meta table:
 *
 * - `constant` is a Lua number set to the value of `T::constant`.
 * - `accessor` is a Lua function which takes a `T*` and returns a pointer to
 *   its `member_obj` member (note that `S` also has to be registered as a user
 *   type in this case).
 * - `value_accessor` is a Lua function which takes a `T*` and returns (a copy
 *   of) its `member_obj` member by value.
 * - `member_fn` is a Lua function which takes a `T*` and a `float` (i.e. the
 *   same arguments as `T::member_fn`) and returns an `int` (the return value of
 *   the same function).
 * - `lambda` is a Lua function which takes two `lua_Number`s (one converted to
 *   `float`) and returns a `lua_Number` (a regular function or function pointer
 *   could also be used).
 * - `c_fn` is a Lua function implemented as a `lua_CFunction`.  No special
 *   treatment of arguments or return values is done.
 *
 * Note that these macros declare members of the \ref nngn::lua namespace, so
 * they should appear in the global namespace.  They do not affect the name
 * resolution of `T` itself, it will be resolved as if it had been used just
 * outside the macro calls.
 */
#ifndef NNGN_LUA_REGISTER_H
#define NNGN_LUA_REGISTER_H

#include <vector>

#include "utils/pp.h"

#include "lua.h"

struct lua_State;

/**
 * Macro to automatically create and register a user type meta table.
 * \see nngn::lua::static_register
 * \see nngn::lua::state_view::new_user_type
 */
#define NNGN_LUA_PROXY(T, ...) \
    NNGN_OVERLOAD(NNGN_LUA_PROXY, T __VA_OPT__(,) __VA_ARGS__)
#define NNGN_LUA_PROXY1(T) NNGN_LUA_PROXY2(T, [](auto){})
#define NNGN_LUA_PROXY2(T, f) \
    static auto NNGN_ANON() = nngn::lua::static_register{ \
        [](lua_State *nngn_L) { \
            [[maybe_unused]] const auto mark = nngn::lua::stack_mark(nngn_L); \
            (f)(nngn::lua::state_view{nngn_L}.new_user_type<T>()); \
        }, \
    };

/**
 * Declares that \p T objects should be manipulated as a user type.
 * Required for stack operations involving user data values of this type.
 * A second optional parameter `N` can specify the name (default: `#T`).
 * \see NNGN_LUA_PROXY Can be used to register the user type meta table.
 */
#define NNGN_LUA_DECLARE_USER_TYPE(T, ...) \
    NNGN_OVERLOAD(NNGN_LUA_DECLARE_USER_TYPE, T __VA_OPT__(,) __VA_ARGS__)
#define NNGN_LUA_DECLARE_USER_TYPE1(T) NNGN_LUA_DECLARE_USER_TYPE2(T, #T)
#define NNGN_LUA_DECLARE_USER_TYPE2(T, N) \
    template<> \
    inline constexpr auto nngn::lua::is_user_type<T> = true; \
    template<> \
    inline constexpr nngn::fixed_string nngn::lua::metatable_name<T> = N;

namespace nngn::lua {

/**
 * Registers a function to be executed when the Lua state is initialized.
 * \see NNGN_LUA_PROXY
 */
class static_register {
public:
    using fn = void(lua_State*);
    /** Executes all registered function and removes them from the list. */
    static void register_all(lua_State *L);
    /** Adds a function to the list. */
    explicit static_register(fn *f) { registry().push_back(f); }
private:
    using V = std::vector<fn*>;
    static V &registry(void) { static V v; return v; }
};

}

#endif

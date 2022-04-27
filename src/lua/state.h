/**
 * \file
 * \brief `lua_State` wrappers.
 *
 * \ref nngn::lua::state_view "state_view" and \ref nngn::lua::state "state" are
 * the main types in this library.  They each contain a `lua_State` and provide
 * a generic and strongly typed interface which closely mirrors the Lua API,
 * with methods such as \ref nngn::lua::state_view::top "state_view::top", \ref
 * nngn::lua::state_view::get_type "state_view::get_type", \ref
 * nngn::lua::state_view::len "state_view::len", etc., all with generic and
 * uniform handling of arguments.  Convenience methods are also provided, such
 * as \ref nngn::lua::state_view::dostring "state_view::do_string", \ref
 * nngn::lua::state_view::print_stack "state_view::print_stack", etc.
 *
 * A new state can be initialized with:
 *
 * \code{.cpp}
 * state lua = {};
 * const bool ok = lua.init();
 * assert(ok);
 * \endcode
 *
 * `lua` will contain the `lua_State`, which will be closed (i.e. `lua_close`)
 * when the object is itself destroyed.  Multiple \ref nngn::lua::state_view
 * "state_view"s can be created from an existing \ref nngn::lua::state "state"
 * (or directly from a `lua_State`): these are non-owning but provide the same
 * interface.
 *
 * \ref nngn::lua::state_view "state_view" can also be used as an argument in
 * C/++ functions registered in Lua.  This is a convenient way to receive the
 * current `lua_State`:
 *
 * \code{.cpp}
 * lua.call([](state_view l) { l.globals()["g"] = true; });
 * assert(lua.globals()["g"] == true);
 * \endcode
 *
 * ## Examples
 *
 * \code{.cpp}
 * // Initialize
 * state lua = {};
 * lua.init();
 * // Push and get stack values.
 * const auto t = lua.push(42).get_type();
 * assert(lua.top() == 1);
 * assert(t == type::number);
 * lua.push("test");
 * assert(lua.len(-1) == 4);
 * // Manipulate the global table.
 * lua.globals()["g"] = "g";
 * assert(lua.globals()["g"] == "g"sv);
 * // Convenience methods.
 * lua.dostring("f = function() end");
 * lua.push(lua.globals()["f"]);
 * lua.print_stack();
 * // print_stack: top: 3
 * //   3 function (function: 0x01234567)
 * //   2 string (h)
 * //   1 number (42)
 * lua.create_table();
 * std::cout << lua.to_string(-1).second << '\n';
 * // table: 0x01234567
 * \endcode
 */
#ifndef NNGN_LUA_STATE_H
#define NNGN_LUA_STATE_H

#include <tuple>
#include <utility>
#include <vector>

#include "math/math.h"
#include "utils/fixed_string.h"
#include "utils/pp.h"
#include "utils/utils.h"

#include "lua.h"
#include "utils.h"

struct lua_State;

namespace nngn::lua {

/** Non-owning \c lua_State wrapper. */
struct alloc_info;

class state_view {
public:
    /** Empty state, must call \c init before any other function is called. */
    state_view(void) = default;
    /** Wraps an existing \c luaState. */
    state_view(lua_State *L_) : L{L_} {}
    /** Implicit conversion so the state can be passed to Lua functions. */
    operator lua_State*(void) const { return this->L; }
    /**
     * Initializes a new state.
     * The current state, if it exists, is destroyed.
     * \param i
     *     If set, track all allocations (if compiled with custom allocator
     *     support).
     */
    bool init(alloc_info *i = nullptr);
    /* Disowns the current reference and returns a non-owning view to it. */
    state_view release(void) { return std::exchange(this->L, {}); }
    /** Destroys the associated \c lua_State. */
    void destroy(void);
    /** \see lua_gettop */
    int top(void) const { return lua_gettop(*this); }
    /** \see lua_type */
    type get_type(int i) const;
    /** \see lua_len */
    template<typename T = lua_Integer> T len(int i) const;
    /** Get memory allocator data.  \see lua_getallocf */
    std::pair<lua_Alloc, void*> allocator(void) const;
    /** \see nngn::lua::global_table */
    global_table globals(void) const;
    /** \see lua_settop */
    void set_top(int i) const { lua_settop(*this, i); }
    /** \see lua_setmetatable */
    void set_meta_table(auto &&mt, int i);
    /** Non-static version. */
    void print_stack(void) const { nngn::lua::print_stack(*this); }
    /** Non-static version. */
    void print_traceback(void) const { nngn::lua::print_traceback(*this); }
    /**
     * Calls \c dofile or \c dostring depending on <tt>s[0] == '@'</tt>.
     * See how the \c lua interpreter program handles \c LUA_INIT.
     */
    bool doarg(std::string_view s) const;
    /** \see luaL_dofile */
    bool dofile(std::string_view filename) const;
    /** \see luaL_dostring */
    bool dostring(std::string_view src) const;
    /** Executes the global function \c f. */
    bool dofunction(std::string_view f) const;
    /** \see lua_newuserdatauv */
    template<typename T> T *new_user_data(int nv = 0) const;
    /** Registers a new user type and returns its meta table. */
    template<typename T> table new_user_type(void) const;
    /** Creates a table and pushes it on the stack.  \see lua_createtable */
    table create_table(void) const;
    /** Creates a table and pushes it on the stack. */
    table create_table(int narr, int nrec) const;
    /** Reads a value from the stack. */
    template<typename T = value_view> T get(int i) const;
    /** Pushes a value onto the stack. */
    template<typename T = value_view> T push(auto &&x) const;
    /** \see lua_pop */
    void pop(int n = 1) const { lua_pop(*this, n); }
    /** \see lua_remove */
    void remove(int i) const { lua_remove(*this, i); }
    /** Function call, protected in debug mode.  \see lua_call */
    void call(auto &&f, auto &&...args) const;
    /** Protected function call with error handler.  \see lua_pcall */
    int pcall(auto &&h, auto &&f, auto &&...args) const;
    /** Pushes a string representation of a value onto the stack. */
    std::pair<value, std::string_view> to_string(int i) const;
    /** Execute the global \c heartbeat function. */
    bool heartbeat(void) const;
protected:
    lua_State *L = nullptr;
};

/** Owning \c lua_State wrapper. */
struct state : state_view {
    NNGN_NO_COPY(state)
    /** Empty state, must call \c init before any other function is called. */
    state(void) = default;
    /** Takes ownership of an existing \c luaState. */
    explicit state(lua_State *L_) : state_view{L_} {}
    /** Transfers the \c lua_State from another object. */
    state(state &&rhs) noexcept { *this = std::move(rhs); }
    /** Transfers the \c lua_State from another object. */
    state &operator=(state &&rhs) noexcept;
    /** Destroys the associated \c lua_State. */
    ~state(void) { this->destroy(); }
};

template<typename T>
T get(nngn::lua::state_view L, int i) {
    return stack_get<T>::get(L, i);
}

template<typename T = value_view>
T push(nngn::lua::state_view lua, auto &&v) {
    [[maybe_unused]] const auto i = lua.top() + 1;
    stack_push<>::push(lua, FWD(v));
    if constexpr(!std::is_void_v<T>)
        return lua.get<T>(i);
}

inline type state_view::get_type(int i) const {
    const auto ret = static_cast<type>(lua_type(*this, i));
    if constexpr(Platform::debug)
        if(std::find(begin(types), end(types), ret) == end(types)) {
            NNGN_LOG_CONTEXT_CF(state_view);
            Log::l() << "invalid type at index " << i << '\n';
        }
    return ret;
}

template<typename T>
T state_view::len(int i) const {
    lua_len(*this, i);
    NNGN_ANON_DECL(defer_pop(*this));
    return this->get<T>(this->top());
}

inline std::pair<lua_Alloc, void*> state_view::allocator(void) const {
    void *p = nullptr;
    auto *const f = lua_getallocf(*this, &p);
    return {f, p};
}

void state_view::set_meta_table(auto &&mt, int i) {
    this->push(FWD(mt));
    lua_setmetatable(*this, i);
}

template<typename T>
T *state_view::new_user_data(int nv) const {
    return static_cast<T*>(
        Math::align_ptr(
            lua_newuserdatauv(*this, sizeof(T) + alignof(T), nv),
            alignof(T)));
}

template<typename T>
T state_view::get(int i) const {
    return lua::get<T>(*this, i);
}

template<typename T>
T state_view::push(auto &&x) const {
    return lua::push<T>(*this, FWD(x));
}

void state_view::call(auto &&f, auto &&...args) const {
    [[maybe_unused]] const int i = this->top() + 1;
    if constexpr(Platform::debug)
        lua_pushcfunction(*this, msgh);
    lua::push(*this, FWD(f));
    (..., lua::push(*this, FWD(args)));
    constexpr auto n_args = static_cast<int>(sizeof...(args));
    if constexpr(Platform::debug) {
        lua_pcall(*this, n_args, LUA_MULTRET, i);
        lua_remove(*this, i);
    } else
        lua_call(*this, n_args, LUA_MULTRET);
}

int state_view::pcall(auto &&h, auto &&f, auto &&...args) const {
    const int h_idx = h.index();
    assert(h_idx <= this->top());
    lua::push(*this, FWD(f));
    (..., lua::push(*this, FWD(args)));
    constexpr auto n_args = static_cast<int>(sizeof...(args));
    return lua_pcall(*this, n_args, LUA_MULTRET, h_idx);
}

inline state &state::operator=(state &&rhs) noexcept {
    state_view::operator=(std::move(rhs).release());
    return *this;
}

template<>
struct stack_get<state_view> {
    static state_view get(lua_State *L, int) { return L; }
};

}

#endif

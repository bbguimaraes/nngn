#ifndef NNGN_LUA_STATE_H
#define NNGN_LUA_STATE_H

#include <tuple>
#include <utility>
#include <vector>

#include <sol/property.hpp>

#include "utils/utils.h"

#include "table.h"
#include "types.h"

struct lua_State;

namespace nngn::lua {

using function = sol::stack_function;
using sol::as_table;
using sol::as_table_t;
using object = sol::stack_object;
using sol::property;
using sol::readonly;
using state_arg = sol::this_state;
using sol::var;

/** Default message handler.  \see lua_pcall */
int msgh(lua_State *L);
/** Logs <tt>L</tt>'s current data stack */
void print_stack(lua_State *L);
/** Logs <tt>L</tt>'s current call stack */
void print_traceback(lua_State *L);

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
    static std::vector<fn*> &registry(void);
};

/** Non-wning \c lua_State wrapper. */
class state_view {
public:
    /** Registers a new user type and returns a reference to its metatable. */
    template<typename T, typename ...Ts>
    static void new_user_type(lua_State *L, const char *name, Ts &&...ts);
    /** Empty state, must call \c init before any other function is called. */
    state_view(void) = default;
    /** Wraps an existing \c luaState. */
    state_view(lua_State *L_) : L{L_} {}
    /** Implicit conversion so the state can be passed to Lua functions. */
    operator lua_State*(void) const { return this->L; }
    /** Initializes a new state, destroying the existing one. */
    bool init(void);
    /** Destroys the associated \c lua_State. */
    void destroy(void);
    /** \see lua_gettop */
    int top(void) const { return lua_gettop(*this); }
    /** Get memory allocator data.  \see lua_getallocf */
    std::pair<lua_Alloc, void*> allocator(void) const;
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
    /** Creates a table and pushes it on the stack.  \see lua_createtable */
    table create_table(void) const { return this->create_table(0, 0); }
    /** Creates a table and pushes it on the stack. */
    table create_table(int narr, int nrec) const;
    /** Protected function call with error handler. \see lua_pcall */
    int pcall(auto &&h, auto &&f, auto &&...args) const;
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
    /* Disown the current reference and return a non-owning view to it. */
    state_view release(void) { return state_view{std::exchange(this->L, {})}; }
};

/**
 * Macro to automatically create and register a user type.
 * \see static_register
 * \see state::new_user_type
 */
#define NNGN_LUA_PROXY(T, ...) \
    static auto NNGN_ANON() = nngn::lua::static_register{ \
        [](lua_State *nngn_L) { \
            nngn::lua::state_view::new_user_type<T>( \
                nngn_L, #T __VA_OPT__(,) __VA_ARGS__); \
        } \
    };

nngn::lua::value_view push(nngn::lua::state_view L, auto &&v) {
    sol::stack::push(L, FWD(v));
    return {L, L.top()};
}

inline auto static_register::registry(void) -> std::vector<fn*>& {
    static std::vector<fn*> r;
    return r;
}

template<typename T, typename ...Ts>
void state_view::new_user_type(lua_State *L, const char *name, Ts &&...ts) {
    const auto gt = nngn::lua::global_table(L);
    const auto t = gt.new_user_type<T>(name);
    t.set(std::forward_as_tuple(FWD(ts)...));
    gt.set(name, t);
}

inline std::pair<lua_Alloc, void*> state_view::allocator(void) const {
    void *p = nullptr;
    auto *const f = lua_getallocf(*this, &p);
    return {f, p};
}

inline table state_view::create_table(int narr, int nrec) const {
    lua_createtable(*this, narr, nrec);
    return {this->L, lua_gettop(*this)};
}

int state_view::pcall(auto &&h, auto &&f, auto &&...args) const {
    constexpr auto n_args = static_cast<int>(sizeof...(args));
    int h_idx = 0;
    // XXX
    if constexpr(requires { h.stack_index(); })
        h_idx = h.stack_index();
    else
        h_idx = h.index();
    assert(h_idx < this->top());
    sol::stack::push(this->L, FWD(f));
    (..., sol::stack::push(this->L, FWD(args)));
    return lua_pcall(this->L, n_args, LUA_MULTRET, h_idx);
}

inline state &state::operator=(state &&rhs) noexcept {
    state_view::operator=(std::move(rhs).release());
    return *this;
}

}

#endif

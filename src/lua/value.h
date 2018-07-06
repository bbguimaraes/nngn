/**
 * \file
 * \brief Operations on generic stack values.
 */
#ifndef NNGN_LUA_VALUE_H
#define NNGN_LUA_VALUE_H

#include <lua.hpp>

#include "utils/utils.h"

#include "lua.h"
#include "state.h"

namespace nngn::lua {

/** Base, non-owning generic stack value reference. */
class value_view {
public:
    NNGN_DEFAULT_CONSTRUCT(value_view)
    value_view(lua_State *L, int i) : value_view{state_view{L}, i} {}
    value_view(state_view l, int i) : m_state{l}, m_index{i} {}
    ~value_view(void) = default;
    state_view state(void) const { return this->m_state; }
    /* Disowns the current reference and returns a non-owning view to it. */
    state_view release(void) { return this->m_state.release(); }
    /** Lua stack index. */
    int index(void) const { return this->m_index; }
    /** Type of value at index `i`. */
    type get_type(void) const;
    /** \see lua_isnil */
    bool is_nil(void) const { return lua_isnil(this->state(), this->index()); }
    /** \see state_view::to_string */
    auto to_string(void) const;
    // Stack API
    void get(lua_State *L, int i) { *this = value_view{L, i}; }
    template<typename T> requires(detail::can_get<T>) T get(void) const;
    int push(void) const;
    template<typename R = value_view> R push(void) const;
    template<typename T>
    explicit operator T(void) const { return this->get<T>(); }
private:
    state_view m_state = {};
    int m_index = 0;
};

/** Owning stack value reference. */
struct value : value_view {
    NNGN_NO_COPY(value)
    using value_view::value_view;
    value(value_view v) : value_view{v} {}
    value(value &&rhs) noexcept : value_view{rhs.release()} {}
    value &operator=(value &&rhs) noexcept;
    ~value(void) { if(this->state()) this->state().remove(this->index()); }
    /* Disowns the current reference and returns a non-owning view to it. */
    value_view release(void) { return {value_view::release(), this->index()}; }
    using value_view::push;
    template<typename R = value>
    R push(void) const { return value_view::push<R>(); }
    value remove(void) { return std::move(*this); }
};

inline type value_view::get_type(void) const {
    return static_cast<type>(lua_type(this->state(), this->index()));
}

inline auto value_view::to_string(void) const {
    return this->state().to_string(this->index());
}

inline int value_view::push(void) const {
    lua_pushvalue(this->state(), this->index());
    return 1;
}

template<typename T>
requires(detail::can_get<T>)
T value_view::get(void) const {
    return this->state().get<T>(this->index());
}

template<typename R>
R value_view::push(void) const {
    lua_pushvalue(this->state(), this->index());
    return {this->state(), lua_gettop(this->state())};
}

inline value &value::operator=(value &&rhs) noexcept {
    this->remove();
    static_cast<value_view&>(*this) = rhs.release();
    return *this;
}

inline std::pair<value, std::string_view> state_view::to_string(int i) const {
    std::size_t n = 0;
    const char *s = luaL_tolstring(*this, i, &n);
    return {
        value{*this, lua_gettop(*this)},
        std::string_view{s, n},
    };
}

template<typename T>
bool operator==(T &&lhs, const value_view &rhs) {
    return FWD(lhs) == rhs.state().get<std::decay_t<T>>(rhs.index());
}

template<typename T>
bool operator==(const value_view &lhs, T &&rhs) {
    return lhs.state().get<std::decay_t<T>>(lhs.index()) == FWD(rhs);
}

template<std::derived_from<value> T>
inline constexpr bool is_stack_ref<T> = true;

static_assert(detail::stack_type<value_view>);
static_assert(detail::stack_type<value>);

}

#endif

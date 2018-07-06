/**
 * \file
 * \brief Operations on table values.
 *
 * A reference to the global table can be obtained either via \ref
 * nngn::lua::state_view::globals "state_view::globals" or by constructing a
 * \ref nngn::lua::global_table "global_table" object directly:
 *
 * \code{.cpp}
 * const global_table g = lua.globals();
 * const global_table g = {lua};
 * \endcode
 *
 * ## Indexing
 *
 * Values can be obtained/set using `operator[]`:
 *
 * \code{.cpp}
 * g["i"] = 42;
 * assert(g["i"] == 42);
 * \endcode
 *
 * Accesses can be nested:
 *
 * \code{.cpp}
 * g["x"]["y"]["z"] = 42;
 * assert(g["x"]["y"]["z"] == 42);
 * \endcode
 *
 * References to the ultimate object can be created naturally:
 *
 * \code{.cpp}
 * const table t = g["x"]["y"]["z"];
 * t["w"] = 42;
 * std::cout << t["w"] << ' ' << g["x"]["y"]["z"];
 * \endcode
 *
 * Each level of access is popped from the stack as necessary during the
 * traversal, so that at the end only the ultimate object remains.
 *
 * \warning
 * \parblock
 *
 * `operator[]` creates a \ref nngn::lua::table_proxy "table_proxy" object,
 * which is an expression template holding a reference to the original table
 * object.  Most of the time, this is an irrelevant implementation detail,
 * but care must be taken to guarantee that the table outlives the proxy:
 *
 * \code{.cpp}
 * const auto g = lua.globals();
 * const auto y = g["x"]["y"];
 * // Bad!  Temporary value (g["x"]) no longer exists!
 * std::cout << y;
 * \endcode
 *
 * Usually, immediately converting the proxy to an actual value, as shown in the
 * examples above, is enough:
 *
 * \code{.cpp}
 * const auto y = g["x"]["y"];
 * // Good.
 * std::cout << y;
 * // Or simply:
 * std::cout << g["x"]["y"].get<int>();
 * \endcode
 *
 * \endparblock
 *
 * ## Meta tables
 *
 * Meta tables, in particular those used for user types, are no different than
 * regular tables and can be manipulated in the same manner:
 *
 * \code{.cpp}
 * auto mt = lua.create_table();
 * mt["__name"] = "user_type";
 * mt["__index"] = mt;
 * mt["f"] = &user_type::f;
 * lua.globals()["user_type"] = mt.remove();
 * // …
 * const value u = lua.new_user_data<user_type>();
 * lua.set_meta_table(lua.globals()["user_type"], u.index());
 * std::cout << u.to_string().second;
 * // user_type: 0x01234567
 * call(lua, &user_type::f);
 * \endcode
 *
 * \see
 *    nngn::lua::state_view::new_user_type performs a similar setup for user
 *    types automatically.
 *
 * ## Iteration
 *
 * See \ref ./iter.h "iter.h" for iterator support for table objects.
 */
#ifndef NNGN_LUA_TABLE_H
#define NNGN_LUA_TABLE_H

#include <functional>
#include <optional>

#include "utils/concepts.h"
#include "utils/tuple.h"

#include "get.h"
#include "push.h"
#include "state.h"
#include "value.h"

namespace nngn::lua {

template<typename T, typename ...Ks> class table_proxy;

namespace detail {

/** CRTP base for stack table types. */
template<typename CRTP>
class table_base : public table_base_tag {
public:
    template<typename K>
    table_proxy<CRTP, std::decay_t<K>> operator[](K &&k) const;
    /** \see lua_len */
    lua_Integer size(void) const;
    /** `lua_next`-based iteration. */
    table_iter<const CRTP> begin(void) const;
    /** Sentinel for \ref begin. */
    table_iter<const CRTP> end(void) const;
    /** `ipairs`-style iteration.  \see nngn::lua::ipairs */
    table_seq_iter<const CRTP> ibegin(void) const;
    /** Sentinel for \ref ibegin. */
    table_seq_iter<const CRTP> iend(void) const;
    /** Gets field without meta methods. */
    template<typename T> T raw_get(auto &&k, T &&def = T{}) const;
    /** Gets field, with optional default value. */
    template<typename T> T get_field(auto &&k, T &&def = T{}) const;
    /** Sets a table field without meta methods. */
    void raw_set(auto &&k, auto &&v) const;
    /** Sets a table field. */
    void set(auto &&k, auto &&v) const;
    /**
     * Sets multiple table fields.
     * \param t: a tuple of <tt>{k0, v0, k1, v1, ...}</tt>
     */
    template<typename ...Ts> void set(std::tuple<Ts...> &&t) const;
protected:
    template<op_mode mode, typename T>
    T get_common(auto &&k, T &&def = T{}) const;
    template<op_mode mode>
    void set_common(auto &&k, auto &&v) const;
private:
    const CRTP &crtp(void) const { return static_cast<const CRTP&>(*this); }
    int begin_op(void) const { return this->crtp().index(); }
    void end_op(int) const {}
};

/** Internal type used in nested table accesses. */
struct table_accessor : value_view, detail::table_base<table_accessor> {
    using value_view::value_view;
    int begin_op(void) const { return this->state().top(); }
    void end_op(int i) const { lua_remove(this->state(), i); }
};

}

/** Non-owning reference to a table on the stack. */
struct table_view : value_view, detail::table_base<table_view> {
    using value_view::value_view;
    table_view(value_view v) : value_view{v} {}
};

/** Owning reference to a table on the stack, popped when destroyed. */
struct table : value, detail::table_base<table> {
    NNGN_MOVE_ONLY(table)
    using value::value;
    ~table(void) = default;
    /** Disowns the current reference and returns a non-owning view to it. */
    table_view release(void) { return value::release(); }
};

/** Table interface to the global environment. */
class global_table : public detail::table_base<global_table> {
public:
    NNGN_DEFAULT_CONSTRUCT(global_table)
    explicit global_table(state_view lua) : m_state{lua} {}
    ~global_table(void) = default;
    state_view state(void) const { return this->m_state; }
    int push(void) const { lua_pushglobaltable(this->state()); return 1; }
private:
    friend detail::table_base<global_table>;
    int begin_op(void) const { this->push(); return this->state().top(); }
    void end_op(int i) const { lua_remove(this->state(), i); }
    state_view m_state = {};
};

/** Expression template for table assignemnts. */
template<typename T, typename ...Ks>
class table_proxy : public detail::table_proxy_tag {
public:
    // Constructors
    table_proxy(const T &t, auto &&k) : m_table{t}, m_keys(FWD(k)) {}
    // Operators
    template<typename V>
    table_proxy &operator=(V &&v) { this->set(FWD(v)); return *this; }
    template<typename V>
    operator V(void) const { return this->get<V>(); }
    template<typename K>
    table_proxy<T, Ks..., std::decay_t<K>> operator[](K &&k) const;
    // Accessors
    const T &table(void) const { return this->m_table; }
    const std::tuple<Ks...> &keys(void) const { return this->m_keys; }
    state_view state(void) const { return this->table().state(); }
    // Operations
    template<typename V> V get(V &&def = V{}) const;
    void set(auto &&v) const;
    int push(void) const { value{*this}.release(); return 1; }
private:
    auto key(void) const { return std::get<0>(this->keys()); }
    auto push_sub(void) const;
    auto make_sub(const auto &t) const;
    const T &m_table;
    std::tuple<Ks...> m_keys;
};

namespace detail {

template<typename CRTP>
template<typename K>
table_proxy<CRTP, std::decay_t<K>> table_base<CRTP>::operator[](K &&k) const {
    return {static_cast<const CRTP&>(*this), FWD(k)};
}

template<typename CRTP>
lua_Integer table_base<CRTP>::size(void) const {
    return this->crtp().state().len(this->crtp().index());
}

template<typename CRTP>
template<typename T>
T table_base<CRTP>::raw_get(auto &&k, T &&def) const {
    return this->get_common<op_mode::raw>(FWD(k), FWD(def));
}

template<typename CRTP>
template<typename T>
T table_base<CRTP>::get_field(auto &&k, T &&def) const {
    return this->get_common<op_mode::normal>(FWD(k), FWD(def));
}

template<typename CRTP>
void table_base<CRTP>::raw_set(auto &&k, auto &&v) const {
    return this->set_common<op_mode::raw>(FWD(k), FWD(v));
}

template<typename CRTP>
void table_base<CRTP>::set(auto &&k, auto &&v) const {
    return this->set_common<op_mode::normal>(FWD(k), FWD(v));
}

template<typename CRTP>
template<typename ...Ts>
void table_base<CRTP>::set(std::tuple<Ts...> &&t) const {
    constexpr auto n = sizeof...(Ts);
    static_assert(!(n % 2));
    [this, &t]<std::size_t ...I>(std::index_sequence<I...>) {
        (..., static_cast<const CRTP*>(this)->set(
            std::get<2 * I>(FWD(t)),
            std::get<2 * I + 1>(FWD(t))));
    }(std::make_index_sequence<n / 2>{});
}

template<typename CRTP>
template<op_mode mode, typename T>
T table_base<CRTP>::get_common(auto &&k, T &&def) const {
    const auto &crtp = static_cast<const CRTP&>(*this);
    const auto lua = crtp.state();
    const int i = crtp.begin_op();
    lua.push(FWD(k));
    constexpr auto get = mode == op_mode::raw ? lua_rawget : lua_gettable;
    const auto type = type_from_lua(get(lua, i));
    crtp.end_op(i);
    NNGN_ANON_DECL(defer_pop(lua, !stack_ref<T>));
    if(type != type::nil)
        return lua.template get<T>(lua.top());
    if constexpr(is_optional<T>)
        return {};
    return FWD(def);
}

template<typename CRTP>
template<op_mode mode>
void table_base<CRTP>::set_common(auto &&k, auto &&v) const {
    const auto &crtp = static_cast<const CRTP&>(*this);
    const auto lua = crtp.state();
    const int i = crtp.begin_op();
    lua.push(FWD(k));
    lua.push(FWD(v));
    if constexpr(mode == op_mode::raw)
        lua_rawset(lua, i);
    else
        lua_settable(lua, i);
    crtp.end_op(i);
}

}

template<typename T, typename ...Ks>
template<typename K>
auto table_proxy<T, Ks...>::operator[](K &&k) const
    -> table_proxy<T, Ks..., std::decay_t<K>>
{
    return {this->m_table, std::tuple_cat(this->m_keys, std::tuple{FWD(k)})};
}

template<typename T, typename ...Ks>
auto table_proxy<T, Ks...>::push_sub(void) const {
    return this->table()
        .template get_field<detail::table_accessor>(this->key());
}

template<typename T, typename ...Ks>
template<typename T1>
auto table_proxy<T, Ks...>::make_sub(const T1 &t) const {
    return [&t]<typename ...Ks1>(std::tuple<Ks1...> &&k) {
        return table_proxy<T1, Ks1...>{t, FWD(k)};
    }(tuple_tail(this->keys()));
}

template<typename T, typename ...Ks>
template<typename V>
V table_proxy<T, Ks...>::get(V &&def) const {
    if constexpr(sizeof...(Ks) == 1)
        return this->table().template get_field<V>(this->key(), FWD(def));
    else
        return this->make_sub(this->push_sub()).template get<V>(FWD(def));
}

template<typename T, typename ...Ks>
void table_proxy<T, Ks...>::set(auto &&v) const {
    if constexpr(sizeof...(Ks) == 1)
        return this->table().template set(this->key(), FWD(v));
    else
        return this->make_sub(this->push_sub()).template set(FWD(v));
}

template<typename T, typename PT, typename ...Ks>
decltype(auto) operator==(T &&lhs, const table_proxy<PT, Ks...> &rhs) {
    return FWD(lhs) == rhs.template get<std::decay_t<T>>();
}

template<typename T, typename PT, typename ...Ks>
decltype(auto) operator==(const table_proxy<PT, Ks...> &lhs, T &&rhs) {
    return lhs.template get<std::decay_t<T>>() == FWD(rhs);
}

template<typename T>
table state_view::new_user_type(void) const {
    constexpr std::string_view meta = metatable_name<T>;
    table t = this->create_table();
    global_table{*this}.set(meta, t);
    t["__name"] = meta;
    t["__index"] = t;
    t["__gc"] = &user_data<T>::gc;
    t["__eq"] = &user_data<T>::eq;
    return t;
}

inline global_table state_view::globals(void) const {
    return global_table{*this};
}

inline table state_view::create_table(void) const {
    return this->create_table(0, 0);
}

inline table state_view::create_table(int narr, int nrec) const {
    lua_createtable(*this, narr, nrec);
    return {this->L, lua_gettop(*this)};
}

/** Creates a table array with each argument in succession. */
table table_array(state_view lua, auto &&...args) {
    using LI = lua_Integer;
    static_assert(sizeof...(args) <= std::numeric_limits<LI>::max());
    constexpr auto n = static_cast<LI>(sizeof...(args));
    auto ret = lua.create_table(n, 0);
    [&ret]<LI ...I>(std::integer_sequence<LI, I...>, auto &&...args_) {
        (..., ret.raw_set(I + 1, args_));
    }(std::make_integer_sequence<LI, n>{}, FWD(args)...);
    return ret;
}

/** Creates a table array from a range. */
template<std::ranges::range T>
table table_from_range(state_view lua, T &&r) {
    int n = 0;
    if constexpr(std::ranges::sized_range<T>)
        n = static_cast<int>(std::ranges::size(FWD(r)));
    auto ret = lua.create_table(n, 0);
    // TODO enumerate
    lua_Integer i = 1;
    for(auto &&x : FWD(r))
        ret.raw_set(i++, FWD(x));
    return ret;
}

/**
 * Creates a table with each successive argument pair as key/value.
 * I.e `{[args[0]] = args[1], [args[2]] = args[3], ...}`.
 */
table table_map(state_view lua, auto &&...args) {
    constexpr auto n = sizeof...(args);
    auto ret = lua.create_table(0, n / 2);
    [&ret]<std::size_t ...I>(std::index_sequence<I...>, auto &&t) {
        (..., ret.raw_set(
            FWD(std::get<2 * I>(t)),
            FWD(std::get<2 * I + 1>(t)))
        );
    }(std::make_index_sequence<n / 2>{}, std::forward_as_tuple(FWD(args)...));
    return ret;
}

/** Reads a table array as a vector. */
template<typename T>
requires(detail::can_get<T>)
struct stack_get<std::vector<T>> {
    static std::vector<T> get(lua_State *L, int i) {
        const table_view t = {L, i};
        const auto n = t.size();
        std::vector<T> ret = {};
        ret.reserve(static_cast<std::size_t>(n));
        for(lua_Integer ti = 1; ti <= n; ++ti)
            ret.push_back(t[ti]);
        return ret;
    }
};

template<typename T, typename ...Ks>
struct stack_push<table_proxy<T, Ks...>> {
    static int push(
        [[maybe_unused]] lua_State *L, const table_proxy<T, Ks...> &t)
    {
        assert(t.state() == L);
        return t.push();
    }
};

template<typename T>
requires(
    std::derived_from<T, detail::table_base_tag>
    && !std::derived_from<T, value>
    && !std::same_as<T, global_table>)
inline constexpr bool is_stack_ref<T> = true;

static_assert(detail::stack_type<table_view>);
static_assert(detail::stack_type<table>);
static_assert(stack_ref<table_view>);
static_assert(stack_ref<table>);
static_assert(!stack_ref<global_table>);

}

#endif

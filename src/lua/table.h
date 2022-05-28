#ifndef NNGN_LUA_TABLE_H
#define NNGN_LUA_TABLE_H

#include <optional>
#include <string>

#include <ElysianLua/elysian_lua_stack.hpp>
#include <ElysianLua/elysian_lua_table.hpp>
#include <ElysianLua/elysian_lua_thread_view.hpp>
#include <sol/state_view.hpp>

#include "utils/concepts.h"
#include "utils/tuple.h"
#include "utils/utils.h"

#include "value.h"

namespace elysian::lua {

inline auto begin(StackTable t) {
    return sol::stack_table{
        t.getThread()->getState(),
        t.getStackIndex(),
    }.begin();
}

inline auto end(StackTable t) {
    return sol::stack_table{
        t.getThread()->getState(),
        t.getStackIndex(),
    }.end();
}

}

namespace nngn::lua {

namespace detail {

/** Tag to relate all \c table_base instantiations via inheritance. */
struct table_base_tag {};

/** Tag to relate all \c table_proxy instantiations via inheritance. */
struct table_proxy_tag {};

// XXX
struct user_type_tag {};

/** CRTP base for stack table types. */
template<typename CRTP>
class table_base : public table_base_tag {
public:
    lua_Integer size(void) const;
    auto begin(void) const;
    auto end(void) const;
    void push(void) const;
    /** Get field without meta methods. */
    template<typename T> T raw_get(auto &&k, T &&def = T{}) const;
    /** Get field, with optional default value. */
    template<typename T> T get(auto &&k, T &&def = T{}) const;
    /** Set a table field without meta methods. */
    void raw_set(auto &&k, auto &&v) const;
    /** Set a table field. */
    void set(auto &&k, auto &&v) const;
    /**
     * Set multiple table fields.
     * \param t: a tuple of <tt>{k0, v0, k1, v1, ...}</tt>
     */
    template<typename ...Ts> void set(std::tuple<Ts...> &&t) const;
protected:
    template<typename T, bool raw>
    T get_common(auto &&k, T &&def = T{}) const;
};

/** Mapping from \c nngn::lua to \c sol types. */
template<typename T> struct sol_type;
template<typename T> using sol_type_t = typename sol_type<T>::type;

/** Transform \c nngn::lua to \c sol types via \c sol::type. */
template<typename T>
auto to_sol(const T &t) {
    // XXX
    if constexpr(std::derived_from<T, user_type_tag>)
        return detail::sol_type_t<T>{t.state(), sol::raw_index{t.index()}};
    else if constexpr(requires { t.index(); })
        return elysian::lua::ThreadView{t.state()}
            .toValue<sol_type_t<T>>(t.index());
    else
        return detail::sol_type_t<T>{};
}

// XXX
template<typename T>
constexpr bool is_stack_based =
    sol::is_stack_based_v<T> && (
        std::is_base_of_v<table_base_tag, T>
        || std::is_base_of_v<table_proxy_tag, T>
        || std::is_same_v<value, T>);

template<typename T>
inline constexpr bool is_optional_stack_based = false;

template<typename T>
inline constexpr bool is_optional_stack_based<std::optional<T>> =
    is_stack_based<T>;

}

template<typename T, typename ...Ks> class table_proxy;

/** Non-owning reference to a table on the stack. */
struct table_view : value_view, detail::table_base<table_view> {
    using value_view::value_view;
    using detail::table_base<table_view>::push;
    template<typename K>
    table_proxy<table_view, std::decay_t<K>> operator[](K &&k) const;
};

/** Owning reference to a table on the stack, popped when destroyed. */
struct table : value, detail::table_base<table> {
    using value::value;
    using detail::table_base<table>::push;
    operator table_view(void) const;
    template<typename K>
    table_proxy<table, std::decay_t<K>> operator[](K &&k) const;
    /**
     * Disown the current reference and return a non-owning view to it.
     * This can be used in contexts where the calling code is responsible for
     * popping the value from the stack, e.g. as a return value for Sol.
     */
    table_view release(void);
};

/** User type tables. */
template<typename T>
struct user_type : detail::user_type_tag, value, detail::table_base<user_type<T>> {
    NNGN_MOVE_ONLY(user_type)
    using value::value;
    using detail::table_base<user_type<T>>::push;
    ~user_type(void) = default;
    template<typename K>
    table_proxy<user_type, std::decay_t<K>> operator[](K &&k) const;
};

/** Table interface to the global environment. */
class global_table : public detail::table_base<global_table> {
    using base = detail::table_base<global_table>;
public:
    NNGN_DEFAULT_CONSTRUCT(global_table)
    using base::base;
    explicit global_table(lua_State *L_) : L{L_} {}
    ~global_table(void) = default;
    lua_State *state(void) const { return this->L; }
    template<typename K>
    table_proxy<global_table, std::decay_t<K>> operator[](K &&k) const;
    template<typename T> user_type<T> new_user_type(const char *name) const;
    void set(auto &&k, auto &&v) const;
private:
    auto to_sol(void) const { return elysian::lua::StaticGlobalsTable{}; }
    lua_State *L = nullptr;
};

/** Expression template for table assignemnts. */
template<typename T, typename ...Ks>
class table_proxy : public detail::table_proxy_tag {
public:
    table_proxy(const T &t, auto &&k) : table{t}, keys(FWD(k)) {}
    template<typename V> table_proxy &operator=(V &&v);
    template<typename V> operator V(void) const;
    template<typename K>
    table_proxy<T, Ks..., std::decay_t<K>> operator[](K &&k) const;
    template<typename V> V get(V &&def = V{}) const;
    void set(auto &&v) const;
private:
    template<typename T1, typename ...Ks1>
    static table_proxy<T1, Ks1...> from_tuple(T1 &&t, std::tuple<Ks1...> &&k);
    const T &table;
    std::tuple<Ks...> keys;
};

namespace detail {

template<> struct sol_type<table> { using type = elysian::lua::StaticStackTable; };
template<> struct sol_type<table_view> { using type = elysian::lua::StaticStackTable; };
template<> struct sol_type<global_table> { using type = elysian::lua::StaticGlobalsTable; };

template<typename T>
struct sol_type<user_type<T>> { using type = sol::stack_usertype<T>; };

template<>
inline auto to_sol<nngn::lua::table_view>(const nngn::lua::table_view &t) {
    return elysian::lua::ThreadView{t.state()}
        .toValue<sol_type_t<nngn::lua::table_view>>(t.index());
}

template<typename CRTP>
lua_Integer table_base<CRTP>::size(void) const {
    const auto *const crtp = static_cast<const CRTP*>(this);
    auto *const L_ = crtp->state();
    lua_len(L_, crtp->index());
    const auto ret = lua_tointeger(L_, -1);
    lua_pop(L_, 1);
    return ret;
}

template<typename CRTP>
auto table_base<CRTP>::begin(void) const {
    using elysian::lua::begin;
    return begin(to_sol(*static_cast<const CRTP*>(this)));
}

template<typename CRTP>
auto table_base<CRTP>::end(void) const {
    using elysian::lua::end;
    return end(to_sol(*static_cast<const CRTP*>(this)));
}

template<typename CRTP>
void table_base<CRTP>::push(void) const {
    const auto *const crtp = static_cast<const CRTP*>(this);
    elysian::lua::ThreadView{crtp->state()}.push(to_sol(*crtp));
}

template<typename CRTP>
template<typename T>
T table_base<CRTP>::raw_get(auto &&k, T &&def) const {
    return this->get_common<T, true>(FWD(k), FWD(def));
}

template<typename CRTP>
template<typename T>
T table_base<CRTP>::get(auto &&k, T &&def) const {
    return this->get_common<T, false>(FWD(k), FWD(def));
}

template<typename CRTP>
template<typename T, bool raw>
T table_base<CRTP>::get_common(auto &&k, T &&def) const {
    auto *const crtp = static_cast<const CRTP*>(this);
    auto *const L_ = crtp->state();
    // XXX
    if constexpr(std::is_same_v<global_table, CRTP>) {
        const auto lua = elysian::lua::ThreadView{L_};
        lua_pushglobaltable(L_);
        lua.push(FWD(k));
        lua_gettable(L_, -2);
        lua_remove(L_, -2);
        return lua.toValue<T>(lua_absindex(L_, -1));
    } else if constexpr(is_optional_stack_based<T>) {
        (void)def;
        lua_pushvalue(L_, crtp->index());
        const auto lua = elysian::lua::ThreadView{L_};
        lua.push(FWD(k));
        if constexpr(raw)
            lua_gettable(L_, -2);
        else
            lua_rawget(L_, -2);
        lua_remove(L_, -2);
        if(lua_type(L_, -1) != LUA_TTABLE)
            return {};
        return lua.toValue<T>(lua_gettop(L_));
    } else if constexpr(is_stack_based<T>) {
        lua_pushvalue(L_, static_cast<const CRTP*>(this)->index());
        sol::stack::push(L_, FWD(k));
        if constexpr(raw)
            lua_gettable(L_, -2);
        else
            lua_rawget(L_, -2);
        lua_remove(L_, -2);
        return {L_, lua_gettop(L_)};
    }
    const auto &sol = to_sol(*static_cast<const CRTP*>(crtp));
    if constexpr(raw) {
        std::optional<T> o = {};
        sol.template getFieldRaw<>(FWD(k), o);
        if(o)
            return std::move(*o);
    } else if(auto o = sol[FWD(k)].template get<std::optional<T>>())
        return std::move(*o);
    return FWD(def);
}

template<typename CRTP>
void table_base<CRTP>::raw_set(auto &&k, auto &&v) const {
    to_sol(*static_cast<const CRTP*>(this)).setFieldRaw(FWD(k), FWD(v));
}

template<typename CRTP>
void table_base<CRTP>::set(auto &&k, auto &&v) const {
    // XXX
    if constexpr(std::derived_from<CRTP, user_type_tag>)
        to_sol(*static_cast<const CRTP*>(this)).set(FWD(k), FWD(v));
    else
        to_sol(*static_cast<const CRTP*>(this)).setField(FWD(k), FWD(v));
}

template<typename CRTP>
template<typename ...Ts>
void table_base<CRTP>::set(std::tuple<Ts...> &&t) const {
    constexpr auto n = sizeof...(Ts);
    static_assert(!(n % 2));
    [this]<std::size_t ...I>(std::index_sequence<I...>, auto &&t_) {
        (..., to_sol(*static_cast<const CRTP*>(this)).set(
            std::get<2 * I>(FWD(t_)),
            std::get<2 * I + 1>(FWD(t_))));
    }(std::make_index_sequence<n / 2>{}, FWD(t));
}

}

inline table::operator table_view(void) const {
    return table_view{this->state(), this->index()};
}

template<typename K>
table_proxy<table_view, std::decay_t<K>> table_view::operator[](K &&k) const {
    return {*this, FWD(k)};
}

template<typename K>
table_proxy<table, std::decay_t<K>> table::operator[](K &&k) const {
    return {*this, FWD(k)};
}

template<typename T>
template<typename K>
auto user_type<T>::operator[](K &&k) const
    -> table_proxy<user_type<T>, std::decay_t<K>>
{
    return {*this, FWD(k)};
}

template<typename K>
auto global_table::operator[](K &&k) const
    -> table_proxy<global_table, std::decay_t<K>>
{
    return {*this, FWD(k)};
}

template<typename T, typename ...Ks>
template<typename K>
auto table_proxy<T, Ks...>::operator[](K &&k) const
    -> table_proxy<T, Ks..., std::decay_t<K>>
{
    return {this->table, std::tuple_cat(this->keys, std::tuple{FWD(k)})};
}

inline table_view table::release(void) {
    return {value_view::release(), this->index()};
}

template<typename T>
user_type<T> global_table::new_user_type(const char *name) const {
    sol::u_detail::register_usertype<T>(this->L);
    user_type<T> ret = {this->L, lua_gettop(this->L)};
    this->set(name, ret);
    return ret;
}

void global_table::set(auto &&k, auto &&v) const {
    this->to_sol()[FWD(k)] = FWD(v);
}

template<typename T, typename ...Ks>
template<typename T1, typename ...Ks1>
auto table_proxy<T, Ks...>::from_tuple(T1 &&t, std::tuple<Ks1...> &&k)
    -> table_proxy<T1, Ks1...>
{
    return {FWD(t), FWD(k)};
}

template<typename T, typename ...Ks>
template<typename V>
table_proxy<T, Ks...> &table_proxy<T, Ks...>::operator=(V &&v) {
    this->set(FWD(v));
    return *this;
}

template<typename T, typename ...Ks>
template<typename V>
table_proxy<T, Ks...>::operator V(void) const {
    return this->get<V>();
}

template<typename T, typename ...Ks>
template<typename V>
V table_proxy<T, Ks...>::get(V &&def) const {
    if constexpr(sizeof...(Ks) == 1)
        return this->table.template get<V>(std::get<0>(this->keys), FWD(def));
    else {
        // XXX
        using T1 = nngn::lua::table;
        return table_proxy::from_tuple(
                this->table.template get<T1>(std::get<0>(this->keys)),
                tuple_tail(this->keys))
            .template get<V>(FWD(def));
    }
}

template<typename T, typename ...Ks>
void table_proxy<T, Ks...>::set(auto &&v) const {
    if constexpr(sizeof...(Ks) == 1)
        return this->table.template set(std::get<0>(this->keys), FWD(v));
    else {
        // XXX
        using T1 = nngn::lua::table;
        return table_proxy::from_tuple(
                this->table.template get<T1>(std::get<0>(this->keys)),
                tuple_tail(this->keys))
            .template set(FWD(v));
    }
}

template<derived_from<detail::table_base_tag> T>
bool sol_lua_check(
    sol::types<T>, lua_State *L, int i,
    auto &&h, sol::stack::record &tracking
) {
    using ST = detail::sol_type_t<T>;
    auto L_ = elysian::lua::ThreadView{L};
    elysian::lua::StackRecord tracking_ = {};
    return elysian::lua::stack_check<ST>(&L_, tracking_, i)
        /*XXX*/|| sol::stack::check<ST>(L, i, FWD(h), tracking);
}

template<derived_from<detail::table_base_tag> T>
T sol_lua_get(
    sol::types<T>, lua_State *L, int i, sol::stack::record &tracking
) {
    tracking.use(1);
    return {L, lua_absindex(L, i)};
}

template<derived_from<detail::table_base_tag> T>
int sol_lua_push(sol::types<T>, [[maybe_unused]] lua_State *L, const T &t) {
    assert(L == t.state());
    t.push();
    return 1;
}

template<derived_from<detail::table_proxy_tag> T>
int sol_lua_push(sol::types<T>, [[maybe_unused]] lua_State *L, const T &t) {
    // XXX
//    assert(L == t.state());
    t.template get<value>().release();
    return 1;
}

}

namespace sol {

template<> struct is_stack_based<nngn::lua::table_view> : std::true_type {};
template<> struct is_stack_based<nngn::lua::table> : std::true_type {};

template<>
struct is_stack_based<std::optional<nngn::lua::table_view>> : std::true_type {};

template<>
struct is_stack_based<std::optional<nngn::lua::table>> : std::true_type {};

template<typename T>
struct is_stack_based<nngn::lua::user_type<T>> : std::true_type {};

template<typename T>
struct is_stack_based<std::optional<nngn::lua::user_type<T>>>
    : std::true_type {};

}

namespace elysian::lua::stack_impl {

template<std::derived_from<nngn::lua::detail::table_base_tag> T>
struct stack_checker<T> {
    static bool check(const ThreadViewBase *L, StackRecord &tracking, int i) {
        using ST = nngn::lua::detail::sol_type_t<T>;
        return elysian::lua::stack_check<ST>(L, tracking, i);
    }
};

template<std::derived_from<nngn::lua::detail::table_base_tag> T>
struct stack_getter<T> {
    static T get(const ThreadViewBase *L, StackRecord &tracking, int i) {
        tracking.use(1);
        return {L->getState(), lua_absindex(L->getState(), i)};
    }
};

template<std::derived_from<nngn::lua::detail::table_base_tag> T>
struct stack_pusher<T> {
    static int push(const ThreadViewBase *L, StackRecord&, const T &t) {
        assert(L->getState() == t.state());
        t.push();
        return 1;
    }
};

template<std::derived_from<nngn::lua::detail::table_proxy_tag> T>
struct stack_pusher<T> {
    static int push(const ThreadViewBase *L, StackRecord&, const T &t) {
        (void)L;
//        assert(L->getState() == t.state());
        t.push();
        return 1;
    }
};

template<typename T>
struct stack_pusher<sol::usertype<T>> {
    static int push(
        const ThreadViewBase* pBase, StackRecord&, const sol::usertype<T> &x
    ) {
        return sol::stack::push(pBase->getState(), x);
    }
};

template<typename T>
struct stack_pusher<sol::stack_usertype<T>> {
    static int push(
        const ThreadViewBase* pBase, StackRecord&,
        const sol::stack_usertype<T> &x
    ) {
        return sol::stack::push(pBase->getState(), x);
    }
};

}

#endif

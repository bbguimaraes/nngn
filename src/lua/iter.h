/**
 * \file
 * \brief Table iteration.
 *
 * Basic input iterators are available for very simple table iteration.  Since
 * it uses the Lua stack, this is only suitable for simple range `for` loops
 * which do not leave values on the stack at the end of each iteration (or other
 * cases, if you know what you are doing).  The objects on the stack are
 * automatically managed by the iterators and should not be moved.  Their scope
 * is limited to the `for` loop (or similar construct), (stack) references held
 * after it will dangle.
 *
 * The stack positions above the key/value can be manipulated freely _inside_
 * the loop body, including before the iterators are dereferenced.  In
 * particular, nested iteration is supported.
 *
 * Example `lua_next`-based iteration:
 *
 * \code{.cpp}
 * const auto t = lua.create_table();
 * for(const auto &[k, v] : …) t[k] = v;
 * for(auto [k, v] : t) use(k, v);
 * \endcode
 *
 * Example `ipairs`-style iteration:
 *
 * \code{.cpp}
 * const auto t = lua.create_table();
 * for(const auto &[i, x] : …) t[i] = x;
 * for(auto [i, x] : ipairs(t)) use(i, x);
 * \endcode
 *
 * Nested example:
 *
 * \code{.cpp}
 * for(auto [i, x] : ipairs(t))
 *     for(auto [k, v] : nngn::lua::table_view{x})
 *         ret[static_cast<std::size_t>(i) - 1].emplace_back(k, v);
 * \endcode
 */
#ifndef NNGN_LUA_ITER_H
#define NNGN_LUA_ITER_H

#include <iterator>

#include "table.h"

namespace nngn::lua {

namespace detail {

template<typename CRTP, typename T>
bool operator==(
    const table_iter_base<CRTP, T> &lhs,
    const table_iter_base<CRTP, T> &rhs);

/** CRTP base for table iterators. */
template<typename CRTP, typename T>
class table_iter_base : std::input_iterator_tag {
public:
    using difference_type = std::ptrdiff_t;
    table_iter_base(void) = default;
    table_iter_base(T *table_) : table{table_} {}
    table_iter_base &operator++(void);
    table_iter_base operator++(int);
    friend bool operator==<>(
        const table_iter_base &lhs,
        const table_iter_base &rhs);
protected:
    CRTP &pre_inc(void) { return static_cast<CRTP&>(++(*this)); }
    CRTP post_inc(void) { return static_cast<CRTP&>((*this)++); }
    T *table = nullptr;
};

/** `lua_next`-based table iterator. */
template<typename T>
class table_iter : public table_iter_base<table_iter<T>, T> {
    using base = table_iter_base<table_iter, T>;
public:
    using base::base;
    using value_type = std::pair<value_view, value_view>;
    value_type operator*(void) const;
    table_iter(T *table);
    table_iter &operator++(void) { return this->pre_inc(); }
    table_iter operator++(int) { return this->post_inc(); }
private:
    friend base;
    table_iter &next(void);
    int key_idx = 0;
};

/** `ipairs`-style table iterator. */
template<typename T>
class table_seq_iter : public table_iter_base<table_seq_iter<T>, T> {
    using base = table_iter_base<table_seq_iter, T>;
public:
    using base::base;
    using value_type = std::pair<lua_Integer, value_view>;
    table_seq_iter(T *table);
    value_type operator*(void) const;
    table_seq_iter &operator++(void) { return this->pre_inc(); }
    table_seq_iter operator++(int) { return this->post_inc(); }
private:
    friend base;
    table_seq_iter &next(void);
    lua_Integer i = 0;
    int value_idx = 0;
};

template<typename CRTP, typename T>
auto table_iter_base<CRTP, T>::operator++(void) -> table_iter_base& {
    return static_cast<CRTP*>(this)->next();
}

template<typename CRTP, typename T>
auto table_iter_base<CRTP, T>::operator++(int) -> table_iter_base {
    auto ret = static_cast<CRTP>(*this);
    this->next();
    return ret;
}

template<typename T>
table_iter<T>::table_iter(T *t) : base{t} {
    const auto lua = this->table->state();
    this->key_idx = lua.top() + 1;
    lua.push(std::tuple{nil, nil});
    this->next();
}

template<typename T>
auto table_iter<T>::next(void) -> table_iter& {
    const auto lua = this->table->state();
    assert(this->key_idx == lua.top() - 1);
    lua.pop(1);
    if(!lua_next(lua, this->table->index()))
        this->table = nullptr;
    return *this;
}

template<typename T>
auto table_iter<T>::operator*(void) const -> value_type {
    const auto lua = this->table->state();
    return {{lua, this->key_idx}, {lua, this->key_idx + 1}};
}

template<typename T>
table_seq_iter<T>::table_seq_iter(T *t) : base{t} {
    const auto lua = this->table->state();
    lua.push(nil);
    this->value_idx = lua.top();
    this->next();
}

template<typename T>
auto table_seq_iter<T>::next(void) -> table_seq_iter& {
    const auto lua = this->table->state();
    assert(this->value_idx == lua.top());
    lua.pop(1);
    if(lua.push((*this->table)[++this->i]).is_nil()) {
        lua.pop(1);
        this->table = nullptr;
    }
    return *this;
}

template<typename T>
auto table_seq_iter<T>::operator*(void) const -> value_type {
    const auto lua = this->table->state();
    return {this->i, {lua, this->value_idx}};
}

template<typename CRTP, typename T>
bool operator==(
    const table_iter_base<CRTP, T> &lhs,
    const table_iter_base<CRTP, T> &rhs)
{
    return lhs.table == rhs.table;
}

template<typename CRTP>
table_iter<const CRTP> table_base<CRTP>::begin(void) const {
    return {static_cast<const CRTP*>(this)};
}

template<typename CRTP>
table_seq_iter<const CRTP> table_base<CRTP>::ibegin(void) const {
    return {static_cast<const CRTP*>(this)};
}

template<typename CRTP>
table_iter<const CRTP> table_base<CRTP>::end(void) const {
    return {};
}

template<typename CRTP>
table_seq_iter<const CRTP> table_base<CRTP>::iend(void) const {
    return {};
}

static_assert(std::input_iterator<table_iter<table_view>>);
static_assert(std::input_iterator<table_seq_iter<table_view>>);
static_assert(
    std::sentinel_for<
        table_iter<table_view>,
        table_iter<table_view>>);
static_assert(
    std::sentinel_for<
        table_seq_iter<table_view>,
        table_seq_iter<table_view>>);

}

/**
 * Simple wrapper for a table value, returns `ipairs`-style iterators.
 * Intended to be used directly in a range `for` expressions to modify the
 * implicit `begin`/`end` calls, e.g.:
 *
 * \code{.cpp}
 * for(auto [i, x] : ipairs(t))
 *     use(i, x);
 * \endcode
 *
 * Since it is a function, most uses can omit the namespace qualification and
 * use the name directly due to ADL.
 */
template<typename T>
auto ipairs(const T &table) {
    struct {
        auto begin(void) const { return this->t.ibegin(); }
        auto end(void) const { return this->t.iend(); }
        const T &t;
    } ret = {table};
    return ret;
}

}

#endif

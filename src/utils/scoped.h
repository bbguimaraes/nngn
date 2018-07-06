#ifndef NNGN_UTILS_SCOPED_H
#define NNGN_UTILS_SCOPED_H

#include <type_traits>
#include <utility>

#include "delegate.h"
#include "utils.h"

namespace nngn {

namespace detail {

template<typename T, typename F, typename ...Args>
using scoped_base = std::conditional_t<
    std::is_same_v<T, nngn::empty>,
    delegate<F, Args...>,
    delegate<F, T, Args...>>;

}

template<typename T, typename F, typename ...Args>
class scoped : private detail::scoped_base<T, F, Args...> {
    using Base = detail::scoped_base<T, F, Args...>;
public:
    NNGN_MOVE_ONLY(scoped)
    scoped(void) = default;
    explicit scoped(T t, Args ...a) : Base(std::move(t), std::move(a)...) {}
    explicit scoped(F f, Args ...a) : Base(std::move(f), std::move(a)...) {}
    scoped(T t, F f, Args ...a) :
        Base(std::move(f), std::move(t), std::move(a)...) {}
    ~scoped(void) { std::move(*this)(); }
    constexpr const T &operator*() const { return this->template arg<0>(); }
    constexpr T &operator*() { return this->template arg<0>(); }
    constexpr const T *operator->() const { return &**this; }
    constexpr T *operator->() { return &**this; }
    constexpr const T *get() const { return &**this; }
    constexpr T *get() { return &**this; }
};

template<typename F, typename ...Ts>
auto make_scoped(F &&f, Ts &&...ts) {
    return scoped<empty, std::decay_t<F>, std::decay_t<Ts>...>(
        std::forward<F>(f), std::forward<Ts>(ts)...);
}

template<typename T, typename F, typename ...Ts>
auto make_scoped_obj(T &&t, F &&f, Ts &&...ts) {
    return scoped<std::decay_t<T>, std::decay_t<F>, std::decay_t<Ts>...>(
        std::forward<T>(t), std::forward<F>(f), std::forward<Ts>(ts)...);
}

}

#endif

#ifndef NNGN_UTILS_DELEGATE_H
#define NNGN_UTILS_DELEGATE_H

#include <functional>
#include <tuple>
#include <utility>

#include "concepts/fundamental.h"
#include "utils.h"

namespace nngn {

/**
 * Unique type for a function or member function pointer.
 * The pointer is fully preserved and not converted to a generic function
 * pointer, so that the call through #operator()() can be fully optimized.
 * Being a unique type for each pointer value, it can be used where callable
 * types are required (e.g. for \ref nngn::delegate).
 *
 * \tparam F A function or member function pointer.
 */
template<function_pointer auto F>
class delegate_fn : std::integral_constant<decltype(F), F> {
    using value_type = decltype(F);
    using base = std::integral_constant<value_type, F>;
    template<typename ...Ts>
    static constexpr bool invocable = std::is_invocable_v<value_type, Ts...>;
public:
    using base::operator value_type;
    /** Calls F.  \see `std::invoke` */
    template<typename ...Ts>
    decltype(auto) operator()(Ts &&...ts) requires(invocable<Ts...>) {
        return std::invoke(F, FWD(ts)...);
    }
};

/**
 * Static container for a callable and its arguments.
 * Unlike `std::function`, the type of the callable is fixed and not erased, so
 * it can be fully optimized.
 */
template<typename F, typename ...Args>
class delegate : private F {
    static constexpr bool has_args = sizeof...(Args);
    [[no_unique_address]] std::tuple<Args...> args = {};
public:
    NNGN_MOVE_ONLY(delegate)
    delegate(void) requires(has_args) = default;
    ~delegate(void) = default;
    explicit delegate(Args ...a) : F{}, args(std::move(a)...) {}
    explicit delegate(F f) requires(has_args) : F{std::move(f)} {}
    delegate(F f, Args ...a) : F{std::move(f)}, args(std::move(a)...) {}
    decltype(auto) operator()()
        { return std::apply(static_cast<F&>(*this), this->args); }
    template<size_t n>
    decltype(auto) arg() const { return std::get<n>(this->args); }
    template<size_t n>
    decltype(auto) arg() { return std::get<n>(this->args); }
};

}

#endif

#ifndef NNGN_UTILS_DELEGATE_H
#define NNGN_UTILS_DELEGATE_H

#include <functional>
#include <tuple>
#include <utility>

#include "concepts.h"
#include "utils.h"

namespace nngn {

/**
 * Unique type for a function or member function pointer.
 * The pointer is fully preserved and not converted to a generic function
 * pointer, so that the call through \ref operator() can be fully optimized.
 * Being a unique type for each pointer value, it can be used where callable
 * types are required (e.g. for \ref nngn::delegate.  \tparam T A function or
 * member function pointer.
 */
template<any_function_pointer auto T>
struct delegate_fn : private std::integral_constant<decltype(T), T> {
    /**
     * Calls \ref T.
     * \param ts
     *     Function arguments.  For member functions, \c this should be the
     *     first argument.
     */
    template<typename ...Ts>
    decltype(auto) operator()(Ts &&...ts)
    requires(std::is_invocable_v<decltype(T), Ts...>)
        { return std::invoke(T, FWD(ts)...); }
};

/**
 * Static container for a callable and its arguments.
 * Unlike \ref std::function, the type of the callable is fixed and not erased,
 * so it can be fully optimized.
 */
template<typename F, typename ...Args>
class delegate : private F {
    [[no_unique_address]] std::tuple<Args...> args;
public:
    NNGN_NO_COPY(delegate)
    ~delegate() = default;
    explicit delegate(F f, Args ...a)
        : F{std::move(f)}, args(std::move(a)...) {}
    decltype(auto) operator()()
        { return std::apply(static_cast<F&>(*this), this->args); }
    template<size_t n>
    decltype(auto) arg() const { return std::get<n>(this->args); }
    template<size_t n>
    decltype(auto) arg() { return std::get<n>(this->args); }
};

}

#endif

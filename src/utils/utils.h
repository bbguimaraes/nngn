#ifndef NNGN_UTILS_UTILS_H
#define NNGN_UTILS_UTILS_H

#include <bit>
#include <cassert>
#include <functional>
#include <span>
#include <string>
#include <string_view>
#include <utility>

#include "concepts/fundamental.h"

#define FWD(...) std::forward<decltype(__VA_ARGS__)>(__VA_ARGS__)

#define NNGN_DEFAULT_CONSTRUCT(x) \
    x() = default; \
    x(const x&) = default; \
    x &operator=(const x&) = default; \
    x(x&&) noexcept = default; \
    x &operator=(x&&) noexcept = default; \

#define NNGN_VIRTUAL(x) \
    NNGN_DEFAULT_CONSTRUCT(x) \
    virtual ~x() = default;

#define NNGN_PURE_VIRTUAL(x) \
    NNGN_DEFAULT_CONSTRUCT(x) \
    virtual ~x() = 0;

#define NNGN_NO_COPY(x) \
    x(const x&) = delete; \
    x &operator=(const x&) = delete;

#define NNGN_MOVE_ONLY(x) \
    NNGN_NO_COPY(x) \
    x(x&&) noexcept = default; \
    x &operator=(x&&) noexcept = default;

#define NNGN_NO_MOVE(x) \
    x(const x&) = delete; \
    x &operator=(const x&) = delete; \
    x(x&&) noexcept = delete; \
    x &operator=(x&&) noexcept = delete;

#define NNGN_ANON_IMPL1(l) nngn_anon_ ## l
#define NNGN_ANON_IMPL0(l) NNGN_ANON_IMPL1(l)
#define NNGN_ANON() NNGN_ANON_IMPL0(__LINE__)
#define NNGN_ANON_DECL(...) \
    [[maybe_unused]] const auto NNGN_ANON() = __VA_ARGS__;

#define NNGN_CONTAINER_OF(t, n, p) \
    (nngn::byte_cast<t*>(nngn::byte_cast<char*>(p) - offsetof(t, n)))

#define NNGN_BIND_MEM_FN(x, f) \
    std::bind_front([] { \
        constexpr auto nngn_ret = \
            &std::remove_pointer_t<std::decay_t<decltype(x)>>::f; \
        static_assert(std::is_member_function_pointer_v<decltype(nngn_ret)>); \
        return nngn_ret; \
    }(), (x))

namespace nngn {

namespace detail {

template<typename T>
struct chain_cast {
    constexpr decltype(auto) operator<<(auto &&x) {
        return static_cast<T>(FWD(x));
    }
};

}

enum class empty {};

/** Performs successive <tt>static_cast</tt>s, right to left. */
template<typename ...Ts>
constexpr decltype(auto) chain_cast(auto &&x) {
    return (detail::chain_cast<Ts>{} << ... << FWD(x));
}

/** Casts \p x to a narrower type, asserting that the value is preserved. */
template<typename To, typename From>
To narrow(const From &x) {
    To ret = static_cast<To>(x);
    assert(static_cast<From>(ret) == x);
    return ret;
}

/** Cast value to \c T with the strictest cast operator. */
template<typename T, typename U>
constexpr decltype(auto) cast(U &&x) {
    if constexpr(requires { const_cast<T>(FWD(x)); })
        return const_cast<T>(FWD(x));
    else if constexpr(requires { static_cast<T>(FWD(x)); })
        return static_cast<T>(FWD(x));
    else
        return reinterpret_cast<T>(FWD(x));
}

/** `reinterpret_cast` restricted to conversions from/to `char`/`uchar`. */
template<typename To, typename From>
requires byte_pointer<To> || byte_pointer<From>
constexpr To byte_cast(From p) {
    constexpr bool is_const = std::is_const_v<std::remove_pointer_t<From>>;
    using V = std::conditional_t<is_const, const void*, void*>;
    return chain_cast<To, V>(p);
}

/** Cast between `span`s where one is a byte type. */
template<typename To, typename From>
requires
    byte_type<To> || char_type<To>
    || byte_type<From> || char_type<From>
std::span<To> byte_cast(std::span<From> s) {
    assert(!(s.size() % sizeof(To)));
    return {byte_cast<To*>(s.data()), s.size() / sizeof(To)};
}

template<enum_ T>
inline constexpr auto to_underlying(T t) {
    return static_cast<std::underlying_type_t<T>>(t);
}

template<typename T>
constexpr auto set_bit(T t, T mask, bool value) {
    assert(std::popcount(mask) == 1);
    return t ^ ((t ^ -T{value}) & mask);
}

}

#endif

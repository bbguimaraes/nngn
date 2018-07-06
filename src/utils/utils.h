#ifndef NNGN_UTILS_UTILS_H
#define NNGN_UTILS_UTILS_H

#include <bit>
#include <cassert>
#include <functional>
#include <string>
#include <string_view>
#include <utility>

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
#define NNGN_ANON_DECL(x) [[maybe_unused]] const auto NNGN_ANON() = (x);

#define NNGN_BIND_MEM_FN(x, f) \
    std::bind_front([] { \
        constexpr auto nngn_ret = \
            &std::remove_pointer_t<std::decay_t<decltype(x)>>::f; \
        static_assert(std::is_member_function_pointer_v<decltype(nngn_ret)>); \
        return nngn_ret; \
    }(), (x))

namespace nngn {

enum class empty {};

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

template<typename T>
constexpr auto set_bit(T t, T mask, bool value) {
    assert(std::popcount(mask) == 1);
    return t ^ ((t ^ -T{value}) & mask);
}

}

#endif

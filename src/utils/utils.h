#ifndef NNGN_UTILS_UTILS_H
#define NNGN_UTILS_UTILS_H

#include <bit>
#include <cassert>
#include <cstddef>
#include <cstring>
#include <span>
#include <string>
#include <string_view>
#include <type_traits>

#include "concepts.h"

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

#define NNGN_NO_COPY(x) \
    x(const x&) = delete; \
    x &operator=(const x&) = delete; \
    x(x&&) noexcept = default; \
    x &operator=(x&&) noexcept = default;

#define NNGN_NO_MOVE(x) \
    x(const x&) = delete; \
    x &operator=(const x&) = delete; \
    x(x&&) noexcept = delete; \
    x &operator=(x&&) noexcept = delete;

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
requires std::is_enum_v<T>
inline constexpr auto to_underlying_type(T t)
    { return static_cast<std::underlying_type_t<T>>(t); }

/** Similar to the stdlib's \c offsetof, but using member data pointers. */
template<typename T>
std::size_t offsetof_ptr(auto T::*p) {
    T t = {};
    return static_cast<std::size_t>(
        static_cast<const char*>(static_cast<const void*>(&(t.*p)))
            - static_cast<const char*>(static_cast<const void*>(&t)));
}

inline auto as_bytes(const void *p) { return static_cast<const std::byte*>(p); }
inline auto as_bytes(void *p) { return static_cast<std::byte*>(p); }

inline std::span<const std::byte> as_byte_span(const auto *x)
    { return {as_bytes(x), sizeof(*x)}; }
inline std::span<std::byte> as_byte_span(auto *x)
    { return {as_bytes(x), sizeof(*x)}; }

inline constexpr bool str_less(const char *lhs, const char *rhs) {
    if(!std::is_constant_evaluated())
        return std::strcmp(lhs, rhs) < 0;
    while(*lhs && *rhs && *lhs == *rhs)
        ++lhs, ++rhs;
    return *rhs && (!*lhs || *lhs < *rhs);
}

template<typename To, typename From>
requires (
    sizeof(To) == sizeof(From)
    && std::is_trivially_copyable_v<To>
    && std::is_trivially_copyable_v<From>)
constexpr auto bit_cast(const From &from) {
    static_assert(std::is_trivially_constructible_v<To>);
    To ret = {};
    std::memcpy(&ret, &from, sizeof(To));
    return ret;
}

template<typename T>
constexpr auto set_bit(T t, T mask, bool value) {
    assert(std::popcount(mask) == 1);
    return t ^ ((t ^ -T{value}) & mask);
}

template<member_pointer auto p>
struct mem_obj {
    template<typename T>
    requires requires (T t) { {t.*p}; }
    constexpr decltype(auto) operator()(const T &t)
        { return t.*p; }
};

template<member_pointer auto p>
struct member_less {
    template<typename T>
    requires requires (T t) { {t.*p}; }
    constexpr bool operator()(const T &lhs, const T &rhs)
        { return lhs.*p < rhs.*p; }
};

/**
 * Allows passing an rvalue pointer to a function.
 * E.g. <tt>takes_ptr(rptr(Obj{}))</tt>.
 */
constexpr decltype(auto) rptr(auto &&r) { return &r; }

bool read_file(std::string_view filename, std::string *ret);

}

#endif

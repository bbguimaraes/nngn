#ifndef NNGN_UTILS_UTILS_H
#define NNGN_UTILS_UTILS_H

#include <bit>
#include <cassert>
#include <cstring>
#include <functional>
#include <span>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>
#include <vector>

#include "concepts.h"
#include "concepts/fundamental.h"

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
#define NNGN_ANON_DECL(x) [[maybe_unused]] const auto NNGN_ANON() = (x);

#define NNGN_CONTAINER_OF(t, n, p) \
    (nngn::byte_cast<decltype(t::n)*>( \
        nngn::byte_cast<char*>(p) - offsetof(t, n)))

#define NNGN_BIND_MEM_FN(x, f) \
    std::bind_front([] { \
        constexpr auto nngn_ret = \
            &std::remove_pointer_t<std::decay_t<decltype(x)>>::f; \
        static_assert(std::is_member_function_pointer_v<decltype(nngn_ret)>); \
        return nngn_ret; \
    }(), (x))

#define NNGN_EXPOSE_ITERATOR(sing, plur, member) \
    using sing##iterator = decltype(member)::iterator; \
    using const_##sing##iterator = decltype(member)::const_iterator; \
    const_##sing##iterator plur##cbegin() const \
        { using std::cbegin; return cbegin(this->member); } \
    const_##sing##iterator plur##cend() const \
        { using std::cend; return cend(this->member); } \
    sing##iterator plur##begin() \
        { using std::begin; return begin(this->member); } \
    sing##iterator plur##end() \
        { using std::end; return end(this->member); }

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

template<enum_ T>
inline constexpr auto to_underlying(T t) {
    return static_cast<std::underlying_type_t<T>>(t);
}

/** Signed distance in bytes between pointers. */
inline constexpr auto ptr_diff(const auto *lhs, const auto *rhs) {
    return byte_cast<const char*>(lhs) - byte_cast<const char*>(rhs);
}

/** Compares the address of two pointers. */
inline constexpr bool ptr_cmp(const auto *p0, const auto *p1)
    { return static_cast<const void*>(p0) == static_cast<const void*>(p1); }

/** Similar to the stdlib's \c offsetof, but using member data pointers. */
template<standard_layout T>
std::size_t offsetof_ptr(auto T::*p) {
    T t = {};
    return static_cast<std::size_t>(ptr_diff(&(t.*p), &t));
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
bool read_file(std::string_view filename, std::vector<std::byte> *ret);

}

#endif

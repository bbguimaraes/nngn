#ifndef NNGN_UTILS_CONCEPTS_FUNDAMENTAL_H
#define NNGN_UTILS_CONCEPTS_FUNDAMENTAL_H

#include <concepts>
#include <cstddef>
#include <type_traits>
#include <version>

namespace nngn {

template<typename T>
concept integral =
#ifdef _LIBCPP_VERSION
    std::is_integral_v<T>;
#else
    std::integral<T>;
#endif

template<typename T>
concept unsigned_integral =
#ifdef _LIBCPP_VERSION
    std::is_integral_v<T> && !std::is_signed_v<T>;
#else
    std::unsigned_integral<T>;
#endif

template<typename T>
concept arithmetic = std::is_arithmetic_v<T>;

template<typename T>
concept char_type =
    integral<T>
    && std::same_as<
        signed char,
        std::make_signed_t<std::remove_cv_t<T>>>;

template<typename T>
concept byte_type = std::is_same_v<std::byte, std::remove_cv_t<T>>;

template<typename T>
concept pointer = std::is_pointer_v<T>;

template<typename T>
concept byte_pointer = pointer<T> && (
    byte_type<std::remove_pointer_t<T>>
    || char_type<std::remove_pointer_t<T>>
);

template<typename T>
concept member_pointer = std::is_member_object_pointer_v<T>;

template<typename T>
concept function = std::is_function_v<T>;

template<typename T>
concept c_function_pointer =
    std::is_pointer_v<T>
    && function<std::remove_pointer_t<T>>;

template<typename T>
concept member_function_pointer = std::is_member_function_pointer_v<T>;

template<typename T>
concept function_pointer = c_function_pointer<T> || member_function_pointer<T>;

template<typename T>
concept lvalue_ref = std::is_lvalue_reference_v<T>;

template<typename T>
concept rvalue_ref = std::is_rvalue_reference_v<T>;

template<typename T>
concept enum_ = std::is_enum_v<T>;

template<typename T>
concept c_enum = enum_<T> && std::convertible_to<T, int>;

template<typename T>
concept scoped_enum = enum_<T> && !std::convertible_to<T, int>;

namespace detail {

template<typename>
inline constexpr bool is_stateless_lambda = false;

template<typename R, typename T, typename ...Args>
inline constexpr bool is_stateless_lambda<R(T::*)(Args...) const> =
    std::convertible_to<T, R(*)(Args...)>;

}

template<typename T>
concept stateless_lambda = requires {
    { T{} };
    { &T::operator() };
    requires detail::is_stateless_lambda<decltype(&T::operator())>;
};

}

#endif

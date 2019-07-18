#ifndef NNGN_UTILS_CONCEPTS_H
#define NNGN_UTILS_CONCEPTS_H

#include <type_traits>

namespace nngn {

template<typename T> concept arithmetic = std::is_arithmetic_v<T>;

template<typename T>
concept member_pointer = std::is_member_object_pointer_v<T>;

template<typename T>
concept function_pointer =
    std::is_pointer_v<T>
    && std::is_function_v<std::remove_pointer_t<T>>;

template<typename T>
concept member_function_pointer = std::is_member_function_pointer_v<T>;

template<typename T>
concept any_function_pointer =
    function_pointer<T> || member_function_pointer<T>;

}

#endif

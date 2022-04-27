#ifndef NNGN_UTILS_ALLOC_REALLOC_H
#define NNGN_UTILS_ALLOC_REALLOC_H

#include <cstdlib>

#include "utils/concepts.h"

#include "base.h"

namespace nngn {

/**
 * Simple allocator which supports reallocation.
 * Implemented in terms of `malloc(3)`, `realloc(3)`, and `free(3)` (which is
 * why \p T must be \ref nngn::trivial).  Can be nested in other allocators that
 * provide an optional `reallocate` method.
 */
template<trivial T = char>
struct reallocator : stateless_allocator<reallocator<T>> {
    using value_type = T;
    using pointer = std::add_pointer_t<value_type>;
    pointer allocate(std::size_t n) noexcept;
    pointer reallocate(pointer p, std::size_t n) noexcept;
    void deallocate(pointer p, std::size_t) noexcept { free(p); }
};

template<typename T>
auto reallocator<T>::allocate(std::size_t n) noexcept -> pointer {
    return static_cast<pointer>(malloc(n * sizeof(T)));
}

template<typename T>
auto reallocator<T>::reallocate(pointer p, std::size_t n) noexcept -> pointer {
    return static_cast<pointer>(realloc(p, n * sizeof(T)));
}

}

#endif

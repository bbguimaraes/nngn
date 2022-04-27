/**
 * \dir src/utils/alloc
 * \brief Custom memory allocators.
 *
 * This module contains custom memory allocators and allocator utilities used by
 * various other modules.
 *
 * The following are available as base classes:
 *
 * - \ref nngn::allocator_base provides default implementations of basic
 *   operations.
 * - \ref nngn::stateless_allocator is a base class for allocators which have no
 *   state and should all be considered equal.
 * - \ref nngn::stateful_allocator is a base class for allocators with state
 *   that should never be considered equal.
 *
 * The following allocators are implemented:
 *
 * - \ref nngn::reallocator is a `realloc(3)`-based allocator (thus only useable
 *   for trivial types) which supports reallocation.
 * - \ref nngn::tagging_allocator transparently adds a header to every allocated
 *   object, similar to common `malloc(3)` implementations.
 * - \ref nngn::tracking_allocator keeps an account of the amount of memory it
 *   allocates and deallocates.
 *
 * In addition, the following helpers are available:
 *
 * - \ref nngn::alloc_block is a simple type which can be used with some of the
 *   above or independently to allocate storage for a type and an associated
 *   header.
 */
#ifndef NNGN_UTILS_ALLOC_BASE_H
#define NNGN_UTILS_ALLOC_BASE_H

#include <cstddef>

namespace nngn {

/** Configuration object for \ref allocator_base. */
struct allocator_opts {
    bool is_always_equal;
};

/**
 * Base class for allocators, implements a few basic operations.
 * \tparam T The allocated type.
 * \tparam o Allocator configuration.
 */
template<typename T, allocator_opts o> struct allocator_base {};

template<typename T, allocator_opts o>
bool operator==(const allocator_base<T, o>&, const allocator_base<T, o>&) {
    return o.is_always_equal;
}

template<typename T, allocator_opts o>
bool operator!=(const allocator_base<T, o>&, const allocator_base<T, o>&) {
    return !o.is_always_equal;
}

/**
 * Base class for allocators that have no state.
 * Automatically implements an equality operator which always returns `true`.
 */
template<typename T>
using stateless_allocator = allocator_base<T, /*XXX*/allocator_opts{.is_always_equal = true}>;

/**
 * Base class for allocators that have state.
 * Automatically implements an equality operator which always returns `false`.
 */
template<typename T>
using stateful_allocator = allocator_base<T, /*XXX*/allocator_opts{.is_always_equal = false}>;

namespace detail {

/**
 * Checks whether an allocator supports memory relocation.
 * Allocators use this check to conditionally support this operation themselves
 * when the base allocator does.
 */
template<typename A>
static constexpr bool has_realloc =
    requires(A a, A::pointer p, std::size_t n) {
        a.reallocate(p, n);
    };

/**
 * Checks whether an allocator supports typed memory allocations.
 * Allocators use this check to conditionally support this operation themselves
 * when the base allocator does.
 */
template<typename A, typename T>
static constexpr bool has_typed_alloc =
    requires(A a, A::pointer p, std::size_t n, T t) {
        a.allocate(p, n, t);
    };

}

}

#endif

#ifndef NNGN_UTILS_ALLOC_BLOCK_H
#define NNGN_UTILS_ALLOC_BLOCK_H

#include <cstddef>
#include <cstdlib>
#include <type_traits>

#include "utils/concepts.h"
#include "utils/utils.h"

namespace nngn {

/**
 * Non-owning handle to an aggregate header and data block.
 * Primarily (but not necessarily) intended to be used in allocators.  The
 * underlying object is allocated as one block of memory.  The handle provides
 * accessors to both the header and data regions, as well as methods to
 * reallocate and free the block.
 *
 * Allocation functions are available if both types are trivial (since `malloc`,
 * `realloc`, and `free` are used).
 *
 * \tparam H The type stored in the header region.
 * \tparam T
 *     The type stored in the data region.  The default is `char`, which can
 *     either be a single byte or (more likely) used with `alloc(size)` to
 *     allocate a block of bytes.  `data()` will the point to the beginning of
 *     the block.
 */
template<typename H, typename T = char>
class alloc_block {
    static constexpr bool both_trivial = trivial<H> && trivial<T>;
public:
    using header_type = H;
    using value_type = T;
    /** Underlying storage type. */
    struct storage {
        // Needed because types can never be deduced from nested classes.
        // http://open-std.org/JTC1/SC22/WG21/docs/papers/2016/p0293r0.pdf
        using parent = alloc_block;
        header_type header = {};
        value_type data = {};
    };
    /** Total size of an allocation for `n` elements. */
    static constexpr std::size_t data_offset = [] {
        if constexpr(std::is_standard_layout_v<alloc_block::storage>)
            return offsetof(alloc_block::storage, data);
        else {
            struct S {
                alignas(header_type) char header[sizeof(header_type)];
                alignas(value_type) char data[sizeof(value_type)];
            };
            return offsetof(S, data);
        }
    }();
    // Static helpers
    /** Total size of an allocation for `n` elements. */
    static constexpr std::size_t alloc_size(std::size_t n);
    // Constructors
    /** Constructs a handle to a pre-existing storage block. */
    static constexpr auto from_storage_ptr(void *p) { return alloc_block{p}; }
    /** Offsets \p p to the beginning of the entire block. */
    static constexpr alloc_block from_ptr(T *p);
    /** Initializes a handle with no associated block. */
    alloc_block(void) noexcept = default;
    // Accessors
    /** Pointer to the block. */
    constexpr storage *data(void) { return this->p; }
    /** Pointer to the block. */
    constexpr const storage *data(void) const { return this->p; }
    /** Pointer to the block header. */
    constexpr header_type *header(void) { return &this->p->header; }
    /** Pointer to the block header. */
    constexpr const header_type *header(void) const { return this->p->header; }
    /** Pointer to the data block. */
    constexpr value_type *get(void) { return &this->p->data; }
    /** Pointer to the data block. */
    constexpr const value_type *get(void) const { return &this->p->data; }
    // Allocation functions
    /** Allocates a block to contain exactly a `T`. */
    static alloc_block alloc(void) requires both_trivial
        { return alloc_block::alloc(sizeof(alloc_block::value_type)); }
    /** Allocates a block to contain `s` bytes of data. */
    static alloc_block alloc(std::size_t s) requires both_trivial
        { alloc_block ret; ret.realloc(s); return ret; }
    /** Reallocates the entire block. */
    void realloc(std::size_t s) requires both_trivial { // TODO clang
        *this = alloc_block{::realloc(this->p, s + offsetof(storage, data))};
    }
    /** Frees the block's memory.  Must be called explicitly. */
    void free(void) requires both_trivial { ::free(this->p); }
private:
    /** Constructs from a pointer to the block. */
    explicit alloc_block(void *p_) : p{static_cast<storage*>(p_)} {}
    /** Pointer to the underlying storage, non-owning. */
    storage *p = nullptr;
};

template<typename H, typename T>
inline constexpr std::size_t alloc_block<H, T>::alloc_size(std::size_t n) {
    return alloc_block::data_offset + n * sizeof(alloc_block::value_type);
}

template<typename H, typename T>
inline constexpr auto alloc_block<H, T>::from_ptr(T *p) -> alloc_block {
    using S = alloc_block::storage;
    if constexpr(std::is_standard_layout_v<S>)
        return alloc_block{NNGN_CONTAINER_OF(storage, data, p)};
    else
        return alloc_block{byte_cast<char*>(p) - alloc_block::data_offset};
}

}

#endif

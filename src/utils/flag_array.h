#ifndef NNGN_UTILS_FLAG_ARRAY_H
#define NNGN_UTILS_FLAG_ARRAY_H

#include <algorithm>
#include <bit>
#include <cassert>
#include <cstddef>
#include <climits>

#include "math/math.h"
#include "utils/utils.h"

namespace nngn {

/**
 * An array of contiguous flags (bit values) optimized for tests.
 * Values are stored in a compact format so that each bit (including for the
 * extra layers) maps directly to single bits in the raw byte storage.  Extra
 * logarithmically-sized layers are stored so that entire areas of the flag
 * array can be checked quickly.
 * Given an initial size `n` (which is rounded up to the next power of 2), the
 * flag storage occupies `n` bits, and each successive layer occupies <tt>n /
 * 2</tt> bits.  The total storage size is then <tt>std::bit_ceil(n) - 1</tt>.
 */
class FlagArray {
public:
    /** Initialize an empty array of size zero. */
    FlagArray() = default;
    /** Initialize an array of \c n flags. */
    FlagArray(std::size_t n)
        : v(std::bit_ceil(n) >> (FlagArray::char_bit_exp - 1)) {}
    /** Retrieves the value of a single flag. */
    bool operator[](std::size_t i) const;
    /**
     * The number of flags stored in the array.
     * Equivalent to the initial size rounded up to the next power of 2.
     */
    std::size_t size() const;
    /** Total size, including the extra layers used for optimization. */
    std::size_t total_size() const;
    /** Storage required for flags. */
    std::size_t size_bytes() const;
    /** Total storage, including the space for layers used for optimization. */
    std::size_t total_size_bytes() const { return this->v.size(); }
    /** Number of logarithmic layers. */
    std::size_t n_layers() const;
    /** Retrieves the value of an element in one of the layers. */
    bool get(std::size_t li, std::size_t i) const;
    /** Sets the value of a single flag. */
    void set(std::size_t i);
    /** Clears all flags. */
    void clear() { std::ranges::fill(this->v, std::byte{}); }
private:
    /** Calculates the initial size (modulo rounding) from storage size. */
    static constexpr std::size_t size_bytes_to_size(std::size_t n);
    /** Calculates the offset into the raw byte storage for bit \c i. */
    std::size_t storage_index(std::size_t i) const;
    /** Calculates the mask for the raw storage byte for bit \c i. */
    unsigned char storage_mask(std::size_t i) const;
    /** Calculates the storage offset for layer \c i. */
    std::size_t layer_offset(std::size_t i) const;
    /** Calculates the offset and mask to retrieve a flag value from a layer. */
    std::tuple<std::size_t, unsigned char> layer_idx(
        std::size_t li, std::size_t i) const;
    /** Updates parent layers after a flag is set. */
    void propagate(std::size_t i);
    unsigned char m_get(std::size_t i, unsigned char mask = UCHAR_MAX) const;
    static constexpr auto char_bit_exp =
        std::countr_zero(static_cast<std::size_t>(CHAR_BIT));
    /**
     * Raw byte storage for flags.
     * Each flag is stored in a single bit in the first \ref storage_size bytes.
     * The optimization layers are stored as a binary tree after the first \ref
     * storage_size byes.  Each successive layer \c i is stored in the next
     * <tt>n >> (i + 1)</tt> bytes (i.e. each decreasing in size by half down to
     * a single-bit layer), starting at <tt>layer_offset(n, i)</tt>.
     */
    std::vector<std::byte> v = {};
};

inline std::size_t FlagArray::size() const
    { return this->size_bytes() << FlagArray::char_bit_exp; }
inline std::size_t FlagArray::total_size() const
    { return this->size() << 1; }
inline std::size_t FlagArray::size_bytes() const
    { return this->total_size_bytes() >> 1; }
inline std::size_t FlagArray::n_layers() const
    { return static_cast<std::size_t>(std::countr_zero(this->size())); }

inline std::size_t FlagArray::storage_index(std::size_t i) const {
    const auto ret = i >> FlagArray::char_bit_exp;
    assert(ret < this->v.size());
    return ret;
}

inline unsigned char FlagArray::storage_mask(std::size_t i) const {
    constexpr auto mask = static_cast<unsigned char>(CHAR_BIT - 1);
    return static_cast<unsigned char>(1u << static_cast<unsigned>(i & mask));
}

inline std::size_t FlagArray::layer_offset(std::size_t i) const {
    assert(i < this->n_layers());
    const auto ret = (this->size() << 1) - (1 << (this->n_layers() - i));
    assert(ret < this->total_size());
    return ret;
}

inline std::tuple<std::size_t, unsigned char> FlagArray::layer_idx(
    std::size_t li, std::size_t i
) const {
    const auto bi = this->layer_offset(li) + i;
    assert(bi < this->total_size());
    return {bi >> FlagArray::char_bit_exp, FlagArray::storage_mask(bi)};
}

inline unsigned char FlagArray::m_get(std::size_t i, unsigned char mask) const {
    assert(i < this->v.size());
    return static_cast<unsigned char>(this->v[i]) & mask;
}

inline bool FlagArray::operator[](std::size_t i) const
    { return this->m_get(this->storage_index(i), this->storage_mask(i)); }

inline bool FlagArray::get(std::size_t li, std::size_t i) const {
    const auto [vi, mask] = this->layer_idx(li, i);
    return this->m_get(vi, mask);
}

inline void FlagArray::set(std::size_t i) {
    const auto vi = this->storage_index(i);
    this->v[vi] = static_cast<std::byte>(
        this->m_get(vi) | this->storage_mask(i));
    this->propagate(i);
}

inline void FlagArray::propagate(std::size_t i) {
    const auto n = this->n_layers();
    for(std::size_t li = 0; li < n; ++li) {
        i >>= 1;
        const auto [vi, mask] = this->layer_idx(li, i);
        const auto x = this->m_get(vi);
        if(x & mask)
            return;
        this->v[vi] = static_cast<std::byte>(x | mask);
    }
}

}

#endif

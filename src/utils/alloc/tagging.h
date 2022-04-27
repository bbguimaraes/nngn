#ifndef NNGN_UTILS_ALLOC_TAGGED_H
#define NNGN_UTILS_ALLOC_TAGGED_H

#include "utils/utils.h"

#include "base.h"

namespace nngn {

/**
 * Associates a block type with an allocator.
 * \see nngn::alloc_block
 * \see nngn::tagging_allocator
 */
template<typename T>
concept tagging_descriptor = requires(T t) {
    // Type exposed in the allocator interface.
    typename T::value_type;
    // Allocated type.
    typename T::block_type;
};

/**
 * Generic implementation of a `malloc(3)`-style allocator with headers.
 * Objects are allocated using `T::block_type` (e.g. \ref nngn::alloc_block),
 * meaning a header is added to each allocation.  This tagging is transparent:
 * after template instantiation, the interface deals only with
 * `T::block_type::value_type` objects.
 *
 * The following are conditionally supported based on whether they are supported
 * in the upstream allocator `A`:
 *
 * - reallocations (see \ref nngn::reallocator)
 * - typed allocations (see \ref nngn::tracking_allocator)
 *
 * \see nngn::tagging_descriptor
 * \see nngn::alloc_block
 */
template<
    tagging_descriptor D,
    typename A = std::allocator<typename D::block_type::value_type>>
class tagging_allocator :
    public stateless_allocator<tagging_allocator<D, A>>
{
public:
    using block_type = typename D::block_type;
    using value_type = typename D::value_type;
    using pointer = value_type*;
    using allocator =
        typename std::allocator_traits<A>::template rebind_alloc<char>;
    template<typename U>
    struct rebind {
        using other =
            tagging_allocator<
                typename D::template rebind<U>::other,
                typename std::allocator_traits<allocator>
                    ::template rebind_alloc<U>>;
    };
    // Construction, assignment
    tagging_allocator(void) = default;
    tagging_allocator(const tagging_allocator &rhs) : alloc{rhs.alloc} {}
    tagging_allocator &operator=(const tagging_allocator &rhs);
    template<tagging_descriptor D1, typename A1>
    tagging_allocator(const tagging_allocator<D1, A1> &rhs);
    explicit tagging_allocator(const allocator &a) : alloc{a} {}
    tagging_allocator(tagging_allocator&&) noexcept = default;
    tagging_allocator &operator=(tagging_allocator&&) noexcept = default;
    ~tagging_allocator(void) = default;
    // Allocator API
    /** Allocates \p n objects (n.b.: only one header is added). */
    pointer allocate(std::size_t n) noexcept;
    /** Typed allocation (see nngn::tracking_allocator). */
    template<typename I>
    pointer allocate(std::size_t n, I i) noexcept
        requires(detail::has_typed_alloc<allocator, I>);
    pointer reallocate(pointer p, std::size_t n) noexcept
        requires(detail::has_realloc<A>);
    void deallocate(pointer p, std::size_t n) noexcept;
    const allocator &get_allocator(void) const { return this->alloc; }
private:
    static pointer to_ptr(char *p);
    static char *raw_from_ptr(pointer p);
    static auto alloc_size(std::size_t n) { return block_type::alloc_size(n); }
    [[no_unique_address]] allocator alloc = {};
};

template<tagging_descriptor D, typename A>
auto tagging_allocator<D, A>::to_ptr(char *p) -> pointer {
    auto b = block_type::from_storage_ptr(p);
    new (b.header()) typename block_type::header_type;
    return b.get();
}

template<tagging_descriptor D, typename A>
char *tagging_allocator<D, A>::raw_from_ptr(pointer p) {
    return nngn::byte_cast<char*>(block_type::from_ptr(p).data());
}

template<tagging_descriptor D, typename A>
auto tagging_allocator<D, A>::operator=(const tagging_allocator &rhs)
    -> tagging_allocator&
{
    this->alloc = rhs.alloc;
    return *this;
}

template<tagging_descriptor D, typename A>
template<tagging_descriptor D1, typename A1>
tagging_allocator<D, A>::tagging_allocator(const tagging_allocator<D1, A1> &rhs)
    : alloc{rhs.get_allocator()} {}

template<tagging_descriptor D, typename A>
auto tagging_allocator<D, A>::allocate(std::size_t n) noexcept -> pointer {
    return tagging_allocator::to_ptr(
        this->alloc.allocate(this->alloc_size(n)));
}

template<tagging_descriptor D, typename A>
template<typename I>
auto tagging_allocator<D, A>::allocate(std::size_t n, I i)
    noexcept -> pointer
    requires(detail::has_typed_alloc<allocator, I>)
{
    return tagging_allocator::to_ptr(
        this->alloc.allocate(this->alloc_size(n), i));
}

template<tagging_descriptor D, typename A>
auto tagging_allocator<D, A>::reallocate(pointer p, std::size_t n)
    noexcept -> pointer requires(detail::has_realloc<A>)
{
    return block_type::from_storage_ptr(
        this->alloc.reallocate(this->raw_from_ptr(p), this->alloc_size(n))
    ).get();
}

template<tagging_descriptor D, typename A>
void tagging_allocator<D, A>::deallocate(pointer p, std::size_t n) noexcept {
    this->alloc.deallocate(this->raw_from_ptr(p), this->alloc_size(n));
}

}

#endif

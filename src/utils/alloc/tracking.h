#ifndef NNGN_UTILS_ALLOC_TRACKING_H
#define NNGN_UTILS_ALLOC_TRACKING_H

#include <memory>

#include "utils/utils.h"

#include "base.h"

namespace nngn {

namespace detail {

/**
 * Checks whether the tracker object tracks memory relocation.
 * The allocator must obviously also support this operation (see \ref
 * nngn::detail::has_realloc).
 * \see nngn::alloc_tracker
 */
template<typename T>
concept tracker_with_realloc =
    requires(T t, typename T::pointer p, std::size_t n) {
        t.reallocate_pre(p, n);
        t.reallocate(p, n);
    };

}

/**
 * Describes how allocations of a particular type are tracked.
 * \see nngn::tracking_allocator
 * \see nngn::alloc_block
 */
template<typename T>
concept alloc_tracker =
    requires(T t, typename T::pointer p, std::size_t n) {
        // Type used in allocator interface.
        typename T::value_type;
        // `value_type` as a pointer.
        typename T::pointer;
        typename T::template rebind<empty>;
        t.allocate(p, n);
        t.deallocate(p, n);
    }
    && (!requires { T::reallocate; } || detail::tracker_with_realloc<T>);

/**
 * Allocator which tracks the amount of memory allocated.
 * This class exists mostly to conform to the allocator interface, the actual
 * operations are delegated to the two template parameters, as described below.
 *
 * The \ref alloc_tracker type \p T should be the one which actually implements
 * the tracking operations.  An object of this type is stored internally --- so
 * it should be some kind of reference if the allocator is frequently copied ---
 * and one of its corresponding methods is called for each operation:
 *
 * - \ref allocate(n): <tt>allocate(p, n)</tt>, where `p` is the pointer just
 *   allocated.
 * - \ref deallocate(p, n): <tt>deallocate(p, n)</tt>.
 *
 * In addition, the following methods are called if the base allocator supports
 * them:
 *
 * - \ref allocate(n, t): <tt>allocate(p, n, t)</tt>, as above but with the
 *   allocation type.
 * - \ref reallocate(p, n): <tt>reallocate(p, n)</tt>.
 *
 * \tparam T
 *     Type which contains tracking data and to which tracking operations are
 *     delegated.
 * \tparam A Base allocator to which all memory allocations are delegated.
 */
template<alloc_tracker T, typename A = std::allocator<typename T::value_type>>
class tracking_allocator :
    public stateful_allocator<tracking_allocator<T, A>>
{
public:
    using value_type = typename T::value_type;
    using pointer = typename T::pointer;
    using allocator =
        typename std::allocator_traits<A>::template rebind_alloc<value_type>;
private:
    template<typename I>
    static bool constexpr has_typed_alloc =
        requires(T t, pointer p, std::size_t n, I i) {
            t.allocate(p, n, i);
        };
public:
    template<typename U>
    struct rebind {
        using other = tracking_allocator<
            typename T::template rebind<U>::other,
            allocator>;
    };
    // Construction, assignment
    tracking_allocator(void) = default;
    explicit tracking_allocator(T t) : m_tracker{std::move(t)} {}
    tracking_allocator(T t, const tracking_allocator &rhs);
    tracking_allocator(const tracking_allocator &rhs);
    template<alloc_tracker T1, typename A1>
    tracking_allocator(const tracking_allocator<T1, A1> &rhs);
    tracking_allocator &operator=(const tracking_allocator &rhs);
    tracking_allocator(tracking_allocator&&) noexcept = default;
    tracking_allocator &operator=(tracking_allocator&&) noexcept = default;
    ~tracking_allocator(void) = default;
    // Accessors
    const allocator &get_allocator(void) const { return this->alloc; }
    T &tracker(void) { return this->m_tracker; }
    const T &tracker(void) const { return this->m_tracker; }
    // Allocator API
    pointer allocate(std::size_t n) noexcept;
    void deallocate(pointer p, std::size_t n) noexcept;
    // Conditional allocator API
    pointer reallocate(pointer p, std::size_t n) noexcept
        requires detail::has_realloc<allocator>
        /*XXX clang*/ {
            this->m_tracker.reallocate_pre(p, n);
            p = this->alloc.reallocate(p, n);
            this->m_tracker.reallocate(p, n);
            return p;
        }
    template<typename I>
    pointer allocate(std::size_t n, I i) noexcept
        requires has_typed_alloc<I>;
private:
    [[no_unique_address]] T m_tracker = {};
    [[no_unique_address]] allocator alloc = {};
};

template<alloc_tracker T, typename A>
tracking_allocator<T, A>::tracking_allocator(T t, const tracking_allocator &rhs)
    : m_tracker{std::move(t)}, alloc{rhs.alloc} {}

template<alloc_tracker T, typename A>
tracking_allocator<T, A>::tracking_allocator(const tracking_allocator &rhs)
    : m_tracker{rhs.tracker()}, alloc{rhs.alloc} {}

template<alloc_tracker T, typename A>
template<alloc_tracker T1, typename A1>
tracking_allocator<T, A>::tracking_allocator(
    const tracking_allocator<T1, A1> &rhs)
    : m_tracker{rhs.tracker()}, alloc{rhs.get_allocator()} {}

template<alloc_tracker T, typename A>
auto tracking_allocator<T, A>::operator=(const tracking_allocator &rhs)
    -> tracking_allocator&
{
    this->alloc = rhs.alloc;
    return *this;
}

template<alloc_tracker T, typename A>
auto tracking_allocator<T, A>::allocate(std::size_t n) noexcept -> pointer {
    auto *const ret = this->alloc.allocate(n);
    this->m_tracker.allocate(ret, n);
    return ret;
}

template<alloc_tracker T, typename A>
template<typename I>
auto tracking_allocator<T, A>::allocate(std::size_t n, I ti)
    noexcept -> pointer
    requires has_typed_alloc<I>
{
    auto *const ret = this->alloc.allocate(n);
    this->m_tracker.allocate(ret, n, ti);
    return ret;
}

template<alloc_tracker T, typename A>
void tracking_allocator<T, A>::deallocate(pointer p, std::size_t n)
    noexcept
{
    this->m_tracker.deallocate(p, n);
    this->alloc.deallocate(p, n);
}

}

#endif

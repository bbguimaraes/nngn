#ifndef NNGN_UTILS_STATIC_VECTOR_H
#define NNGN_UTILS_STATIC_VECTOR_H

#include <cassert>
#include <cstddef>
#include <vector>

#include "utils/ranges.h"
#include "utils/utils.h"

namespace nngn {

/**
 * Fixed-size vector with an embedded free list.
 * Provides constant time insertion and removal.  The free list is kept in the
 * first \c sizeof(std::uintptr_t) bytes of \c T.  Adding elements when the
 * vector is full results in undefined behavior, but the size can be changed
 * using \c set_capacity.
 */
template<typename T>
class static_vector : private std::vector<T> {
    union entry { entry *next_free; T t; std::uintptr_t align; };
    static_assert(sizeof(entry*) <= sizeof(T));
    static_assert(alignof(entry*) <= alignof(T));
    using vector_type = std::vector<T>;
    /** Pointer to the first entry in the free list. */
    entry *free_head = {};
public:
    using typename vector_type::iterator;
    using typename vector_type::const_iterator;
    using vector_type::back;
    using vector_type::begin;
    using vector_type::cbegin;
    using vector_type::cend;
    using vector_type::data;
    using vector_type::capacity;
    using vector_type::empty;
    using vector_type::end;
    using vector_type::front;
    using vector_type::operator[];
    NNGN_MOVE_ONLY(static_vector)
    ~static_vector(void) = default;
    /**
     * Constructs an empty vector.
     * \c set_capacity must be called before items are added.
     */
    static_vector(void) = default;
    /** Constructs an empty vector with capacity \c n. */
    explicit static_vector(std::size_t n) { this->set_capacity(n); }
    /**
     * Calculates the true size (excluding the entries in the free list).
     * Complexity: \c O(n_free()).
     */
    std::size_t size(void) const;
    /** Calculates the number of entries in the free list. */
    std::size_t n_free(void) const;
    /** Checks if the vector is full (i.e. no more items can be added). */
    bool full(void) const;
    /**
     * Changes the total number of items of the vector can hold.
     * Previous data are lost.
     */
    void set_capacity(std::size_t c);
    /**
     * Inserts an element in the first free position.
     * Complexity: O(1).
     */
    void insert(T t) { this->emplace(std::move(t)); }
    /**
     * Emplace an element in the first free position.
     * Complexity: O(1).
     */
    template<typename ...Ts> T &emplace(Ts &&...ts);
    /**
     * Removes element \c p from the vector.
     * Complexity: O(1).
     */
    void erase(T *p);
    /**
     * Removes the element at \c *it from the vector.
     * Complexity: O(1).
     */
    void erase(iterator it);
};

template<typename T>
std::size_t static_vector<T>::size(void) const {
    return vector_type::size() - this->n_free();
}

template<typename T>
bool static_vector<T>::full(void) const {
    return !this->free_head && vector_type::size() == this->capacity();
}

template<typename T>
std::size_t static_vector<T>::n_free(void) const {
    std::size_t ret = {};
    for(auto *p = this->free_head; p; p = p->next_free)
        ++ret;
    return ret;
}

template<typename T>
void static_vector<T>::set_capacity(std::size_t c) {
    nngn::set_capacity(static_cast<vector_type*>(this), c);
    this->free_head = {};
}

template<typename T>
template<typename ...Ts>
T &static_vector<T>::emplace(Ts &&...ts) {
    assert(!this->full());
    T *ret = {};
    if(entry **next = &this->free_head; *next)
        ret = &std::exchange(*next, (*next)->next_free)->t;
    else
        ret = &this->emplace_back();
    return *ret = T(FWD(ts)...);
}

template<typename T>
void static_vector<T>::erase(T *p) {
    assert(&this->front() <= p);
    assert(p <= &this->back());
    return this->erase(this->begin() + (p - this->data()));
}

template<typename T>
void static_vector<T>::erase(iterator it) {
    assert(this->begin() <= it);
    assert(it < this->end());
    if(&*it == &this->back())
        return this->pop_back();
    auto &e = reinterpret_cast<entry&>(*it);
    e.t = {};
    auto **const next = &this->free_head;
    e.next_free = *next;
    *next = &e;
}

}

#endif

#ifndef NNGN_UTILS_POINTER_FLAG_H
#define NNGN_UTILS_POINTER_FLAG_H

#include <cassert>
#include <cstdint>

namespace nngn {

template<typename T> class pointer_flag {
    static constexpr uintptr_t MASK = 0b1;
    T *p = nullptr;
    auto to_uint() const { return reinterpret_cast<uintptr_t>(this->p); }
public:
    pointer_flag() = default;
    explicit constexpr pointer_flag(T *p);
    T *get();
    bool flag() { return this->to_uint() & pointer_flag::MASK; }
    void set_flag(bool b);
};

template<typename T>
inline constexpr pointer_flag<T>::pointer_flag(T *p_p)
    : p(p_p) { assert(!this->flag()); }

template<typename T>
T *pointer_flag<T>::get() {
    const auto u = this->to_uint();
    const auto off = u - (u & ~MASK);
    auto *const cp = static_cast<char*>(static_cast<void*>(this->p));
    return static_cast<T*>(static_cast<void*>(cp - off));
}

template<typename T>
inline void pointer_flag<T>::set_flag(bool b) {
    if(!b) {
        this->p = this->get();
        return;
    }
    const auto u = this->to_uint();
    const auto off = (u | pointer_flag::MASK) - u;
    auto *const cp = static_cast<char*>(static_cast<void*>(this->p));
    this->p = static_cast<T*>(static_cast<void*>(cp + off));
}

}

#endif

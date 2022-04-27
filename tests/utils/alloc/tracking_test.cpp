#include "tracking_test.h"

#include "utils/alloc/block.h"
#include "utils/alloc/realloc.h"
#include "utils/alloc/tagging.h"
#include "utils/alloc/tracking.h"
#include "utils/scoped.h"

using namespace std::string_literals;

namespace {

enum class alloc_type : int {};

struct allocation {
    alloc_type type = {};
    std::size_t size = {};
};

using T = int;
using alloc_block = nngn::alloc_block<allocation, T>;

struct info {
    struct data { std::size_t n, bytes, n_total, bytes_total; };
    std::array<data, 4> v;
};

template<typename T>
struct tracker {
    using value_type = T;
    using pointer = std::add_pointer_t<T>;
    using block_type = nngn::alloc_block<allocation, value_type>;
    template<typename U> struct rebind { using other = tracker<U>; };
    tracker(void) = default;
    explicit tracker(info *i_) : i{i_} {}
    template<typename U>
    tracker(const tracker<U> &rhs) : i{rhs.i} {}
    void allocate(pointer p, std::size_t n);
    void deallocate(pointer p, std::size_t n);
    info *i;
};

template<typename T>
void tracker<T>::allocate(pointer, std::size_t n) {
    this->i->v[0].n += n;
    this->i->v[0].n_total += n;
    this->i->v[0].bytes += n * sizeof(value_type);
    this->i->v[0].bytes_total += n * sizeof(value_type);
}

template<typename T>
void tracker<T>::deallocate(pointer, std::size_t n) {
    this->i->v[0].n -= n;
    this->i->v[0].bytes -= n * sizeof(value_type);
}

template<typename T>
struct nested_tracker : tracker<T> {
    using base_type = tracker<T>;
    using value_type = typename base_type::value_type;
    using pointer = typename base_type::pointer;
    using block_type = typename base_type::block_type;
    template<typename U>
    struct rebind { using other = nested_tracker<U>; };
    using base_type::base_type;
    void allocate(pointer p, std::size_t n);
    void allocate(pointer p, std::size_t n, alloc_type t);
    void deallocate(pointer p, std::size_t n);
};

template<typename T>
void nested_tracker<T>::allocate(pointer p, std::size_t n) {
    base_type::allocate(p, n);
    *block_type::from_ptr(p).header() = {.size = n};
}

template<typename T>
void nested_tracker<T>::allocate(pointer p, std::size_t n, alloc_type t) {
    nested_tracker::allocate(p, n);
    *block_type::from_ptr(p).header() = {.type = t, .size = n};
    const auto ti = static_cast<std::size_t>(t);
    if(!ti)
        return;
    this->i->v[ti].n += n;
    this->i->v[ti].n_total += n;
    this->i->v[ti].bytes += n * sizeof(value_type);
    this->i->v[ti].bytes_total += n * sizeof(value_type);
}

template<typename T>
void nested_tracker<T>::deallocate(pointer p, std::size_t n) {
    base_type::deallocate(p, n);
    const auto t = static_cast<std::size_t>(
        block_type::from_ptr(p).header()->type);
    if(!t)
        return;
    this->i->v[t].n -= n;
    this->i->v[t].bytes -= n * sizeof(value_type);
}

template<typename T>
struct realloc_tracker : nested_tracker<T> {
    using base_type = nested_tracker<T>;
    using value_type = typename base_type::value_type;
    using pointer = typename base_type::pointer;
    using block_type = typename base_type::block_type;
    template<typename U>
    struct rebind { using other = nested_tracker<U>; };
    using base_type::base_type;
    void reallocate_pre(pointer p, std::size_t n);
    void reallocate(pointer p, std::size_t n);
};

template<typename T>
void realloc_tracker<T>::reallocate_pre(pointer p, std::size_t) {
    this->deallocate(p, block_type::from_ptr(p).header()->size);
}

template<typename T>
void realloc_tracker<T>::reallocate(pointer p, std::size_t n) {
    this->allocate(p, n, block_type::from_ptr(p).header()->type);
}

auto deleter(auto &&a, auto *p, std::size_t n) {
    return nngn::make_scoped([&a, p, n] { a.deallocate(p, n); });
}

template<template<typename, typename> typename C>
void test(void) {
    using A = nngn::tracking_allocator<tracker<T>>;
    using CT = C<T, A>;
    info i = {};
    std::size_t nt1 = 0, bt1 = 0;
    {
        auto c = CT{A{tracker<T>{&i}}};
        QCOMPARE(i.v[0].n, 0);
        QCOMPARE(i.v[0].bytes, 0);
        c.emplace_back();
        QCOMPARE(i.v[0].n, 1);
        QCOMPARE(i.v[0].n_total, 1);
        const std::size_t b0 = i.v[0].bytes;
        const std::size_t bt0 = i.v[0].bytes_total;
        QVERIFY(b0);
        QVERIFY(bt0);
        c.emplace_back();
        nt1 = i.v[0].n_total;
        QVERIFY(i.v[0].n >= 1);
        QVERIFY(nt1 >= 1);
        const std::size_t b1 = i.v[0].bytes;
        bt1 = i.v[0].bytes_total;
        QVERIFY(b1 >= b0);
        QVERIFY(bt1 >= bt0);
    }
    QCOMPARE(i.v[0].n, 0);
    QCOMPARE(i.v[0].n_total, nt1);
    QCOMPARE(i.v[0].bytes, 0);
    QCOMPARE(i.v[0].bytes_total, bt1);
}

template<template<typename, typename> typename C>
void test_nested(void) {
    using A0 = nngn::tagging_allocator<tracker<T>>;
    using A1 = nngn::tracking_allocator<nested_tracker<T>, A0>;
    using CT = C<T, A1>;
    info i = {};
    std::size_t nt1 = 0, bt1 = 0;
    {
        auto c = CT{A1{nested_tracker<T>{&i}}};
        QCOMPARE(i.v[0].n, 0);
        QCOMPARE(i.v[0].bytes, 0);
        c.emplace_back();
        QCOMPARE(i.v[0].n, 1);
        QCOMPARE(i.v[0].n_total, 1);
        const std::size_t b0 = i.v[0].bytes;
        const std::size_t bt0 = i.v[0].bytes_total;
        QVERIFY(b0);
        QVERIFY(bt0);
        c.emplace_back();
        nt1 = i.v[0].n_total;
        QVERIFY(i.v[0].n >= 1);
        QVERIFY(nt1 >= 1);
        const std::size_t b1 = i.v[0].bytes;
        bt1 = i.v[0].bytes_total;
        QVERIFY(b1 >= b0);
        QVERIFY(bt1 >= bt0);
    }
    QCOMPARE(i.v[0].n, 0);
    QCOMPARE(i.v[0].n_total, nt1);
    QCOMPARE(i.v[0].bytes, 0);
    QCOMPARE(i.v[0].bytes_total, bt1);
}

}

void TrackingTest::vector(void) { test<std::vector>(); }
void TrackingTest::vector_nested(void) { test_nested<std::vector>(); }
void TrackingTest::list(void) { test<std::list>(); }
void TrackingTest::list_nested(void) { test_nested<std::list>(); }

void TrackingTest::typed(void) {
    using A = nngn::tracking_allocator<tracker<T>>;
    info i = {};
    auto a = A{tracker<T>{&i}};
    QCOMPARE(i.v[0].n, 0);
    QCOMPARE(i.v[0].n_total, 0);
    QCOMPARE(i.v[0].bytes, 0);
    QCOMPARE(i.v[0].bytes_total, 0);
    constexpr std::size_t n = 8;
    T *const p0 = a.allocate(n);
    NNGN_ANON_DECL(deleter(a, p0, n));
    memset(p0, 0, n * sizeof(*p0));
    QCOMPARE(i.v[0].n, n);
    QCOMPARE(i.v[0].n_total, n);
    QCOMPARE(i.v[0].bytes, n * sizeof(T));
    QCOMPARE(i.v[0].bytes_total, n * sizeof(T));
    T *const p1 = a.allocate(n);
    NNGN_ANON_DECL(deleter(a, p1, n));
    memset(p1, 0, n * sizeof(*p1));
    QCOMPARE(i.v[0].n, 2 * n);
    QCOMPARE(i.v[0].n_total, 2 * n);
    QCOMPARE(i.v[0].bytes, 2 * n * sizeof(T));
    QCOMPARE(i.v[0].bytes_total, 2 * n * sizeof(T));
}

void TrackingTest::nested(void) {
    using A0 = nngn::reallocator<>;
    using A1 = nngn::tagging_allocator<tracker<T>, A0>;
    using A2 = nngn::tracking_allocator<nested_tracker<T>, A1>;
    info i = {};
    constexpr std::size_t n = 8;
    {
        auto a = A2{nested_tracker<T>{&i}};
        QCOMPARE(i.v[0].n, 0);
        QCOMPARE(i.v[0].n_total, 0);
        QCOMPARE(i.v[0].bytes, 0);
        QCOMPARE(i.v[0].bytes_total, 0);
        T *const p0 = a.allocate(n);
        NNGN_ANON_DECL(deleter(a, p0, n));
        QCOMPARE(i.v[0].n, n);
        QCOMPARE(i.v[0].n_total, n);
        QCOMPARE(i.v[0].bytes, n * sizeof(T));
        QCOMPARE(i.v[0].bytes_total, n * sizeof(T));
        T *const p1 = a.allocate(n, alloc_type{1});
        NNGN_ANON_DECL(deleter(a, p1, n));
        QCOMPARE(i.v[0].n, 2 * n);
        QCOMPARE(i.v[0].n_total, 2 * n);
        QCOMPARE(i.v[0].bytes, 2 * n * sizeof(T));
        QCOMPARE(i.v[0].bytes_total, 2 * n * sizeof(T));
        QCOMPARE(i.v[1].n, n);
        QCOMPARE(i.v[1].n_total, n);
        QCOMPARE(i.v[1].bytes, n * sizeof(T));
        QCOMPARE(i.v[1].bytes_total, n * sizeof(T));
        T *const p2 = a.allocate(n, alloc_type{2});
        NNGN_ANON_DECL(deleter(a, p2, n));
        QCOMPARE(i.v[0].n, 3 * n);
        QCOMPARE(i.v[0].n_total, 3 * n);
        QCOMPARE(i.v[0].bytes, 3 * n * sizeof(T));
        QCOMPARE(i.v[0].bytes_total, 3 * n * sizeof(T));
        QCOMPARE(i.v[1].n, n);
        QCOMPARE(i.v[1].n_total, n);
        QCOMPARE(i.v[1].bytes, n * sizeof(T));
        QCOMPARE(i.v[1].bytes_total, n * sizeof(T));
        QCOMPARE(i.v[2].n, n);
        QCOMPARE(i.v[2].n_total, n);
        QCOMPARE(i.v[2].bytes, n * sizeof(T));
        QCOMPARE(i.v[2].bytes_total, n * sizeof(T));
    }
    QCOMPARE(i.v[0].n, 0);
    QCOMPARE(i.v[0].n_total, 3 * n);
    QCOMPARE(i.v[0].bytes, 0);
    QCOMPARE(i.v[0].bytes_total, 3 * n * sizeof(T));
    QCOMPARE(i.v[1].n, 0);
    QCOMPARE(i.v[1].n_total, n);
    QCOMPARE(i.v[1].bytes, 0);
    QCOMPARE(i.v[1].bytes_total, n * sizeof(T));
    QCOMPARE(i.v[2].n, 0);
    QCOMPARE(i.v[2].n_total, n);
    QCOMPARE(i.v[2].bytes, 0);
    QCOMPARE(i.v[2].bytes_total, n * sizeof(T));
}

void TrackingTest::realloc(void) {
    using A0 = nngn::reallocator<>;
    using A1 = nngn::tagging_allocator<realloc_tracker<T>, A0>;
    using A2 = nngn::tracking_allocator<realloc_tracker<T>, A1>;
    info i = {};
    constexpr std::size_t n = 8;
    {
        auto a = A2{realloc_tracker<T>{&i}};
        auto del_n = n;
        auto *p = a.allocate(del_n);
        NNGN_ANON_DECL(
            nngn::make_scoped([&a, &p, &del_n] { a.deallocate(p, del_n); }));
        QCOMPARE(i.v[0].n, n);
        QCOMPARE(i.v[0].bytes, n * sizeof(T));
        QCOMPARE(i.v[0].n_total, n);
        QCOMPARE(i.v[0].bytes_total, n * sizeof(T));
        p = a.reallocate(p, del_n = 2 * n);
        QCOMPARE(i.v[0].n, 2 * n);
        QCOMPARE(i.v[0].bytes, 2 * n * sizeof(T));
        QCOMPARE(i.v[0].n_total, 3 * n);
        QCOMPARE(i.v[0].bytes_total, 3 * n * sizeof(T));
        p = a.reallocate(p, del_n = 3 * n);
        QCOMPARE(i.v[0].n, 3 * n);
        QCOMPARE(i.v[0].bytes, 3 * n * sizeof(T));
        QCOMPARE(i.v[0].n_total, 6 * n);
        QCOMPARE(i.v[0].bytes_total, 6 * n * sizeof(T));
    }
    QCOMPARE(i.v[0].n, 0);
    QCOMPARE(i.v[0].bytes, 0);
    QCOMPARE(i.v[0].n_total, 6 * n);
    QCOMPARE(i.v[0].bytes_total, 6 * n * sizeof(T));
}

void TrackingTest::typed_realloc(void) {
    using A0 = nngn::reallocator<>;
    using A1 = nngn::tagging_allocator<realloc_tracker<T>, A0>;
    using A2 = nngn::tracking_allocator<realloc_tracker<T>, A1>;
    info i = {};
    constexpr std::size_t n = 8;
    {
        auto a = A2{realloc_tracker<T>{&i}};
        auto del_n = n;
        auto *p = a.allocate(del_n, alloc_type{1});
        NNGN_ANON_DECL(
            nngn::make_scoped([&a, &p, &del_n] { a.deallocate(p, del_n); }));
        QCOMPARE(i.v[0].n, n);
        QCOMPARE(i.v[0].bytes, n * sizeof(T));
        QCOMPARE(i.v[0].n_total, n);
        QCOMPARE(i.v[0].bytes_total, n * sizeof(T));
        QCOMPARE(i.v[1].n, n);
        QCOMPARE(i.v[1].bytes, n * sizeof(T));
        QCOMPARE(i.v[1].n_total, n);
        QCOMPARE(i.v[1].bytes_total, n * sizeof(T));
        p = a.reallocate(p, del_n = 2 * n);
        QCOMPARE(i.v[0].n, 2 * n);
        QCOMPARE(i.v[0].bytes, 2 * n * sizeof(T));
        QCOMPARE(i.v[0].n_total, 3 * n);
        QCOMPARE(i.v[0].bytes_total, 3 * n * sizeof(T));
        QCOMPARE(i.v[1].n, 2 * n);
        QCOMPARE(i.v[1].bytes, 2 * n * sizeof(T));
        QCOMPARE(i.v[1].n_total, 3 * n);
        QCOMPARE(i.v[1].bytes_total, 3 * n * sizeof(T));
        p = a.reallocate(p, del_n = 3 * n);
        QCOMPARE(i.v[0].n, 3 * n);
        QCOMPARE(i.v[0].bytes, 3 * n * sizeof(T));
        QCOMPARE(i.v[0].n_total, 6 * n);
        QCOMPARE(i.v[0].bytes_total, 6 * n * sizeof(T));
        QCOMPARE(i.v[1].n, 3 * n);
        QCOMPARE(i.v[1].bytes, 3 * n * sizeof(T));
        QCOMPARE(i.v[1].n_total, 6 * n);
        QCOMPARE(i.v[1].bytes_total, 6 * n * sizeof(T));
    }
    QCOMPARE(i.v[0].n, 0);
    QCOMPARE(i.v[0].bytes, 0);
    QCOMPARE(i.v[0].n_total, 6 * n);
    QCOMPARE(i.v[0].bytes_total, 6 * n * sizeof(T));
}

QTEST_MAIN(TrackingTest)

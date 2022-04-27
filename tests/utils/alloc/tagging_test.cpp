#include "tagging_test.h"

#include "utils/alloc/block.h"
#include "utils/alloc/tagging.h"
#include "utils/scoped.h"

namespace {

struct allocation { std::size_t i = 42; };
using T = int;

template<typename T>
struct tracking {
    using value_type = T;
    using block_type = nngn::alloc_block<allocation, T>;
    template<typename U> struct rebind { using other = tracking<U>; };
};

using A = nngn::tagging_allocator<tracking<T>>;

template<template<typename, typename> typename C>
void test_container(void) {
    auto v = C<T, A>{};
    v.emplace_back(0);
    v.emplace_back(1);
}

auto deleter(auto &&a, auto *p, std::size_t n) {
    return nngn::make_scoped([&a, p, n] { a.deallocate(p, n); });
}

}

void TaggingTest::vector(void) { test_container<std::vector>(); }
void TaggingTest::list(void) { test_container<std::list>(); }

void TaggingTest::alloc(void) {
    A a = {};
    constexpr std::size_t n = 8;
    T *const p0 = a.allocate(n);
    T *const p1 = a.allocate(n);
    NNGN_ANON_DECL(deleter(a, p0, n));
    NNGN_ANON_DECL(deleter(a, p1, n));
    using block = tracking<T>::block_type;
    auto b0 = block::from_ptr(p0);
    auto b1 = block::from_ptr(p1);
    QCOMPARE(static_cast<void*>(b0.get()), static_cast<void*>(p0));
    QCOMPARE(static_cast<void*>(b1.get()), static_cast<void*>(p1));
    QCOMPARE(b0.header()->i, 42);
    QCOMPARE(b1.header()->i, 42);
    ++b0.header()->i;
    ++b1.header()->i;
    QCOMPARE(b0.header()->i, 43);
    QCOMPARE(b1.header()->i, 43);
}

QTEST_MAIN(TaggingTest)

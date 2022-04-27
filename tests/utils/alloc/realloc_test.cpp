#include "realloc_test.h"

#include "utils/literals.h"
#include "utils/alloc/realloc.h"
#include "utils/scoped.h"

using namespace nngn::literals;

namespace {

constexpr auto N = 1024_z;

auto deleter(auto &&a, auto *p, std::size_t n) {
    return nngn::make_scoped([&a, p, n] { a.deallocate(p, n); });
}

}

void ReallocTest::alloc(void) {
    nngn::reallocator a = {};
    char *const p0 = a.allocate(N);
    memset(p0, 0, N);
    char *const p1 = a.allocate(N);
    memset(p1, 0, N);
    a.deallocate(p0, N);
    a.deallocate(p1, N);
}

void ReallocTest::realloc(void) {
    nngn::reallocator a = {};
    std::size_t n = N;
    char *const p0 = a.allocate(n);
    memset(p0, 0, n);
    n = N / 2;
    char *const p1 = a.reallocate(p0, n);
    memset(p1, 0, n);
    n = n * 2;
    char *const p2 = a.reallocate(p1, n);
    memset(p2, 0, n);
    a.deallocate(p2, n);
}

void ReallocTest::typed_alloc(void) {
    struct S { std::array<char, N> a; };
    nngn::reallocator<S> a = {};
    S *const p0 = a.allocate(N);
    new (p0) S{};
    S *const p1 = a.allocate(N);
    new (p1) S{};
    a.deallocate(p0, N);
    a.deallocate(p1, N);
}

void ReallocTest::typed_realloc(void) {
    struct S { std::array<char, N> a; };
    nngn::reallocator<S> a = {};
    S *const p0 = a.allocate(2);
    new (p0 + 0) S{};
    new (p0 + 1) S{};
    S *const p1 = a.reallocate(p0, 1);
    new (p1 + 0) S{};
    S *const p2 = a.reallocate(p1, 4);
    new (p2 + 0) S{};
    new (p2 + 1) S{};
    new (p2 + 2) S{};
    new (p2 + 3) S{};
    a.deallocate(p2, 4);
}

QTEST_MAIN(ReallocTest)

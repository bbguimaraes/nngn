#include "block_test.h"

#include "utils/alloc/block.h"
#include "utils/scoped.h"

namespace {

template<typename T>
auto make_scoped(T &&t) {
    return nngn::make_scoped_obj(FWD(t), nngn::delegate_fn<&T::free>{});
}

}

void BlockTest::bytes(void) {
    struct allocation { std::size_t i; };
    constexpr auto off = sizeof(allocation);
    constexpr std::size_t n = 1024;
    using alloc_block = nngn::alloc_block<allocation>;
    auto b = make_scoped(alloc_block::alloc(n));
    void *p0 = b->data();
    void *p1 = b->get();
    QCOMPARE(nngn::ptr_diff(p1, p0), off);
    *b = alloc_block::from_ptr(b->get());
    p0 = b->data();
    p1 = b->get();
    QCOMPARE(b->data(), p0);
    QCOMPARE(nngn::ptr_diff(p1, p0), off);
    b->realloc(2 * n);
    p0 = b->data();
    p1 = b->get();
    QCOMPARE(nngn::ptr_diff(p1, p0), off);
}

void BlockTest::type(void) {
    struct allocation { std::size_t i; };
    struct data { alignas(64) std::size_t i; };
    constexpr auto off = alignof(data);
    using alloc_block = nngn::alloc_block<allocation, data>;
    auto b = make_scoped(alloc_block::alloc());
    void *p0 = b->data();
    auto *p1 = b->get();
    QCOMPARE(nngn::ptr_diff(p1, p0), off);
    *p1 = {.i = 42};
    *b = alloc_block::from_ptr(b->get());
    p0 = b->data();
    p1 = b->get();
    QCOMPARE(b->data(), p0);
    QCOMPARE(nngn::ptr_diff(p1, p0), off);
    QCOMPARE(b->get()->i, 42);
    b->realloc(2048);
    p0 = b->data();
    p1 = b->get();
    QCOMPARE(p0, b->data());
    QCOMPARE(nngn::ptr_diff(p1, p0), off);
    QCOMPARE(b->get()->i, 42);
}

QTEST_MAIN(BlockTest)

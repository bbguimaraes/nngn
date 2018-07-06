#include "utils/scoped.h"

#include "scoped_test.h"

namespace {

struct Called {
    bool *b = {};
    void test() { *this->b = true; }
    void test_arg(bool c) { *this->b = c; }
};

bool g_called = false;
void delegate_test() { g_called = true; }
void delegate_test_arg(bool b) { g_called = b; }

}

static void delegate_base_test(bool *b) { *b = true; }

void ScopedTest::delegate_base() {
    bool called = false;
    auto f = nngn::delegate_fn<delegate_base_test>{};
    f(&called);
    QVERIFY(called);
}

void ScopedTest::constructor() {
    constexpr auto f = [](int) {};
    using S = nngn::scoped<int, decltype(f)>;
    S{42};
    S{f};
}

void ScopedTest::lambda() {
    g_called = false;
    {
        auto s = nngn::make_scoped([] { g_called = true; });
        static_assert(sizeof(s) == 1);
    }
    QVERIFY(g_called);
}

void ScopedTest::lambda_with_storage() {
    bool called = false;
    {
        auto s = nngn::make_scoped([&called] { called = true; });
        static_assert(sizeof(s) == sizeof(bool*));
    }
    QVERIFY(called);
}

void ScopedTest::lambda_obj() {
    bool called = false;
    {
        auto s = nngn::make_scoped_obj(
            Called{&called},
            [b = true](auto &&c) { c.test_arg(b); });
        static_assert(sizeof(s) == 2 * sizeof(Called));
    }
    QVERIFY(called);
}

void ScopedTest::delegate() {
    g_called = false;
    {
        auto s = nngn::make_scoped(nngn::delegate_fn<delegate_test>{});
        static_assert(sizeof(s) == 1);
    }
    QVERIFY(g_called);
}

void ScopedTest::delegate_arg() {
    g_called = false;
    {
        auto s = nngn::make_scoped(
            nngn::delegate_fn<delegate_test_arg>{}, true);
        static_assert(sizeof(s) == sizeof(bool));
    }
    QVERIFY(g_called);
}

void ScopedTest::delegate_obj() {
    g_called = false;
    {
        auto s = nngn::make_scoped_obj(
            true, nngn::delegate_fn<delegate_test_arg>{});
        static_assert(sizeof(s) == sizeof(bool));
    }
    QVERIFY(g_called);
}

void ScopedTest::obj_member() {
    bool called = false;
    {
        auto s = nngn::make_scoped_obj(
            Called{&called}, nngn::delegate_fn<&Called::test>{});
        static_assert(sizeof(s) == sizeof(Called));
    }
    QVERIFY(called);
}

void ScopedTest::obj_member_arg() {
    bool called = false;
    {
        auto s = nngn::make_scoped_obj(
            Called{&called}, nngn::delegate_fn<&Called::test_arg>{}, true);
        static_assert(sizeof(s) == 2 * sizeof(bool*));
    }
    QVERIFY(called);
}

QTEST_MAIN(ScopedTest)

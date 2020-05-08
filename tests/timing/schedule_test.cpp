#include <chrono>
#include <cstring>

#include "timing/schedule.h"
#include "timing/timing.h"
#include "utils/scoped.h"

#include "schedule_test.h"

using namespace std::chrono_literals;

const auto CB = [](void *p) { return **static_cast<bool**>(p) = true; };

static std::vector<std::byte> gen_data(bool *p) {
    constexpr auto n = sizeof(p);
    std::vector<std::byte> ret(n);
    new (ret.data()) bool*{p};
    return ret;
}

void ScheduleTest::next(void) {
    nngn::Timing t;
    nngn::Schedule s;
    s.init(&t);
    bool called = false;
    s.next({CB, nullptr, gen_data(&called), {}});
    QVERIFY(!called);
    QVERIFY(s.update());
    QVERIFY(called);
    called = false;
    QVERIFY(s.update());
    QVERIFY(!called);
}

void ScheduleTest::in(void) {
    nngn::Timing t;
    nngn::Schedule s;
    s.init(&t);
    auto called = false;
    s.in(1s, {CB, nullptr, gen_data(&called), {}});
    QVERIFY(!called);
    t.now += 500ms;
    QVERIFY(s.update());
    QVERIFY(!called);
    t.now += 500ms;
    QVERIFY(s.update());
    QVERIFY(called);
    called = false;
    t.now += 500ms;
    QVERIFY(s.update());
    QVERIFY(!called);
}

void ScheduleTest::at(void) {
    nngn::Timing t;
    nngn::Schedule s;
    s.init(&t);
    bool called = false;
    s.at(t.now + 1s, {CB, nullptr, gen_data(&called), {}});
    QVERIFY(!called);
    t.now += 500ms;
    QVERIFY(s.update());
    QVERIFY(!called);
    t.now += 500ms;
    QVERIFY(s.update());
    QVERIFY(called);
    called = false;
    t.now += 500ms;
    QVERIFY(s.update());
    QVERIFY(!called);
}

void ScheduleTest::frame(void) {
    nngn::Timing t;
    nngn::Schedule s;
    s.init(&t);
    bool called = false;
    s.frame(3, {CB, nullptr, gen_data(&called), {}});
    QVERIFY(!called);
    QVERIFY(s.update());
    QVERIFY(!called);
    t.frame = 1;
    QVERIFY(s.update());
    QVERIFY(!called);
    t.frame = 2;
    QVERIFY(s.update());
    QVERIFY(!called);
    t.frame = 3;
    QVERIFY(s.update());
    QVERIFY(called);
    called = false;
    t.frame = 4;
    QVERIFY(s.update());
    QVERIFY(!called);
}

void ScheduleTest::atexit(void) {
    nngn::Timing t;
    nngn::Schedule s;
    s.init(&t);
    bool atexit_called = false, next_called = false;
    s.atexit({CB, nullptr, gen_data(&atexit_called), {}});
    QVERIFY(!atexit_called);
    QVERIFY(s.update());
    QVERIFY(!atexit_called);
    s.next({CB, nullptr, gen_data(&next_called), {}});
    QVERIFY(s.exit());
    QVERIFY(atexit_called);
    QVERIFY(!next_called);
    atexit_called = false;
    QVERIFY(s.exit());
    QVERIFY(!atexit_called);
    QVERIFY(!next_called);
}

void ScheduleTest::ignore_failures(void) {
    nngn::Timing t;
    nngn::Schedule s;
    s.init(&t);
    s.next({
        .f = [](auto) { return false; },
        .flags = nngn::Schedule::Flag::IGNORE_FAILURES,
    });
    QVERIFY(s.update());
    s.next({.f = [](auto) { return false; }});
    QVERIFY(!s.update());
}

void ScheduleTest::heartbeat(void) {
    nngn::Timing t;
    nngn::Schedule s;
    s.init(&t);
    bool called = false;
    s.next({CB, nullptr, gen_data(&called), nngn::Schedule::Flag::HEARTBEAT});
    QVERIFY(!called);
    QVERIFY(s.update());
    QVERIFY(called);
    called = false;
    QVERIFY(s.update());
    QVERIFY(called);
    called = false;
    QVERIFY(s.update());
    QVERIFY(called);
}

void ScheduleTest::cancel(void) {
    constexpr auto H = nngn::Schedule::Flag::HEARTBEAT;
    nngn::Timing t;
    nngn::Schedule s;
    s.init(&t);
    bool called0 = false, called1 = false;
    const auto i = s.next({CB, nullptr, gen_data(&called0), H});
    s.next({CB, nullptr, gen_data(&called1), H});
    QVERIFY(!called0);
    QVERIFY(!called1);
    QVERIFY(s.update());
    QVERIFY(called0);
    QVERIFY(called1);
    s.cancel(i);
    called0 = called1 = false;
    QVERIFY(s.update());
    QVERIFY(!called0);
    QVERIFY(called1);
    bool called2 = false;
    s.next({CB, nullptr, gen_data(&called2), H});
    QVERIFY(s.update());
    QVERIFY(called2);
}

void ScheduleTest::recursive(void) {
    nngn::Timing t;
    nngn::Schedule s;
    s.init(&t);
    bool called = false;
    struct data { nngn::Schedule *s; bool *p; };
    std::vector<std::byte> v(sizeof(data));
    new (v.data()) data{&s, &called};
    s.next({
        [](void *p) {
            auto inner_d = static_cast<data*>(p);
            inner_d->s->next({CB, nullptr, gen_data(inner_d->p), {}});
            return true;
        }, nullptr, v, {}});
    QVERIFY(!called);
    QVERIFY(s.update());
    QVERIFY(!called);
    QVERIFY(s.update());
    QVERIFY(called);
    called = false;
    QVERIFY(s.update());
    QVERIFY(!called);
}

void ScheduleTest::recursive_remove(void) {
    nngn::Timing t;
    nngn::Schedule s;
    s.init(&t);
    size_t i = 0;
    struct data { nngn::Schedule *s; size_t *i; };
    std::vector<std::byte> v(sizeof(data));
    new (v.data()) data{&s, &i};
    i = s.next({
        [](auto *p) {
            auto inner_d = static_cast<data*>(p);
            inner_d->s->cancel(*inner_d->i);
            return true;
        }, nullptr, v, {}});
    QVERIFY(s.update());
}

void ScheduleTest::destructor(void) {
    nngn::Timing t;
    nngn::Schedule s;
    s.init(&t);
    bool called0 = false, called1 = false;
    const auto nop = [](void*){ return true; };
    s.next({nop, CB, gen_data(&called0), {}});
    s.in(1s, {nop, CB, gen_data(&called1), {}});
    QVERIFY(!called0);
    QVERIFY(!called1);
    QVERIFY(s.update());
    QVERIFY(called0);
    QVERIFY(!called1);
    t.now += 1s;
    called0 = false;
    QVERIFY(s.update());
    QVERIFY(!called0);
    QVERIFY(called1);
    called1 = false;
    QVERIFY(s.update());
    QVERIFY(!called0);
    QVERIFY(!called1);
}

void ScheduleTest::map(void) {
    nngn::Timing t;
    nngn::Schedule s;
    struct d0 {
        nngn::Schedule *s;
        nngn::Schedule::Fn next;
        size_t *next_i;
        std::vector<std::byte> d;
    };
    struct d1 {
        nngn::Schedule *s;
        nngn::Schedule::Fn next;
        std::vector<std::byte> d;
        size_t *i;
    };
    struct d2 { nngn::Schedule *s; size_t *f1; size_t *i; };
    const auto f0 = [](auto *p) {
        auto d = static_cast<d0*>(p);
        *d->next_i = d->s->next({
            d->next, nullptr, d->d, nngn::Schedule::Flag::HEARTBEAT});
        return true;
    };
    const auto f1 = [](auto *p) {
        auto d = static_cast<d1*>(p);
        if((*d->i)++ == 1)
            d->s->next({d->next, nullptr, d->d, {}});
        return true;
    };
    const auto f2 = [](auto *p) {
        auto d = static_cast<d2*>(p);
        d->s->cancel(*d->f1);
        *d->i = 0;
        return true;
    };
    size_t i = 0, f1_idx = 0;
    auto v0 = std::vector<std::byte>(sizeof(d0));
    auto v1 = std::vector<std::byte>(sizeof(d1));
    std::vector<std::byte> v2(sizeof(d2));
    new (v2.data()) d2{&s, &f1_idx, &i};
    auto destroy_v1 = nngn::make_scoped_obj(
        new (v1.data()) d1{&s, f2, v2, &i},
        [](auto *x) { static_cast<d1*>(static_cast<void*>(x))->d1::~d1(); });
    auto destroy_v0 = nngn::make_scoped_obj(
        new (v0.data()) d0{&s, f1, &f1_idx, v1},
        [](auto *x) { static_cast<d0*>(static_cast<void*>(x))->d0::~d0(); });
    s.init(&t);
    s.next({f0, nullptr, v0, {}});
    QVERIFY(s.update());
    QCOMPARE(i, 0);
    QVERIFY(s.update());
    QCOMPARE(i, 1);
    QVERIFY(s.update());
    QCOMPARE(i, 2);
    QVERIFY(s.update());
    QCOMPARE(i, 0);
}

QTEST_MAIN(ScheduleTest)

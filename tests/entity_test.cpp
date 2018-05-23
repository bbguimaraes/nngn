#include "entity_test.h"

#include <cstring>

#include "entity.h"

#include "collision/colliders.h"
#include "timing/timing.h"

#include "tests/tests.h"

using ParentTestFn = void(Entity*, Entity*, nngn::Collider*);

Q_DECLARE_METATYPE(nngn::vec3)
Q_DECLARE_METATYPE(ParentTestFn*)

void EntityTest::max_v_data() {
    QTest::addColumn<float>("max_v");
    QTest::addColumn<nngn::vec3>("v");
    QTest::newRow("no max") << 0.0f << nngn::vec3{1, 0, 0};
    QTest::newRow("below") << 2.0f << nngn::vec3{1, 0, 0};
    QTest::newRow("above") << 0.5f << nngn::vec3{0.5f, 0, 0};
}

void EntityTest::max_v() {
    QFETCH(const float, max_v);
    QFETCH(const nngn::vec3, v);
    Entities es;
    es.set_max(1);
    auto *const e = es.add();
    e->v = {1, 0, 0};
    e->max_v = max_v;
    nngn::Timing t;
    t.dt = std::chrono::milliseconds(16);
    es.update(t);
    QCOMPARE(e->v, v);
}

void EntityTest::add_remove() {
    Entities e;
    e.set_max(3);
    Entity *e0 = e.add();
    e.set_name(e0, "e0");
    Entity *e1 = e.add();
    e.set_name(e1, "e1");
    Entity *e2 = e.add();
    e.set_name(e2, "e2");
    QCOMPARE(e.n(), 3);
    QVERIFY(std::strcmp(e.name(*e0).data(), "e0") == 0);
    QVERIFY(std::strcmp(e.name(*e1).data(), "e1") == 0);
    QVERIFY(std::strcmp(e.name(*e2).data(), "e2") == 0);
    e.remove(e1);
    QCOMPARE(e.n(), 2);
    QVERIFY(std::strcmp(e.name(*e0).data(), "e0") == 0);
    QVERIFY(std::strcmp(e.name(*e2).data(), "e2") == 0);
    e.remove(e0);
    QCOMPARE(e.n(), 1);
    QVERIFY(std::strcmp(e.name(*e2).data(), "e2") == 0);
    e1 = e.add();
    e.set_name(e1, "e1");
    QCOMPARE(e.n(), 2);
    QVERIFY(std::strcmp(e.name(*e1).data(), "e1") == 0);
    QVERIFY(std::strcmp(e.name(*e2).data(), "e2") == 0);
    e0 = e.add();
    e.set_name(e0, "e0");
    QCOMPARE(e.n(), 3);
    QVERIFY(std::strcmp(e.name(*e0).data(), "e0") == 0);
    QVERIFY(std::strcmp(e.name(*e1).data(), "e1") == 0);
    QVERIFY(std::strcmp(e.name(*e2).data(), "e2") == 0);
}

void EntityTest::parent_data() {
    constexpr auto pos = [](Entity*, Entity *e, nngn::Collider*) {
        e->set_pos({3, 4, 5});
    };
    constexpr auto col = [](Entity*, Entity *e, nngn::Collider *c) {
        e->set_collider(c);
    };
    constexpr auto par = [](Entity *p, Entity *e, nngn::Collider*) {
        e->set_parent(p);
    };
    QTest::addColumn<ParentTestFn*>("f0");
    QTest::addColumn<ParentTestFn*>("f1");
    QTest::addColumn<ParentTestFn*>("f2");
#define T(f0, f1, f2) QTest::newRow(#f0 ", " #f1 ", " #f2) << +f0 << +f1 << +f2;
    T(pos, col, par);
    T(pos, par, col);
    T(col, pos, par);
    T(col, par, pos);
    T(par, pos, col);
    T(par, col, pos);
#undef T
}

void EntityTest::parent() {
    constexpr nngn::vec3 parent_pos = {0, 1, 2}, child_pos = {3, 4, 5};
    QFETCH(ParentTestFn*, f0);
    QFETCH(ParentTestFn*, f1);
    QFETCH(ParentTestFn*, f2);
    Entities e = {};
    e.set_max(2);
    nngn::Collider c = {};
    Entity &parent = *e.add(), &child = *e.add();
    parent.set_pos(parent_pos);
    f0(&parent, &child, &c);
    f1(&parent, &child, &c);
    f2(&parent, &child, &c);
    e.update_children();
    QCOMPARE(child.p, child_pos);
    QCOMPARE(c.pos, parent_pos + child_pos);
}

QTEST_MAIN(EntityTest)

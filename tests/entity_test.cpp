#include "entity_test.h"

#include <cstring>

#include "entity.h"

#include "timing/timing.h"

#include "tests/tests.h"

Q_DECLARE_METATYPE(nngn::vec3)

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

QTEST_MAIN(EntityTest)

#include <cstring>

#include "entity.h"

#include "entity_test.h"

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

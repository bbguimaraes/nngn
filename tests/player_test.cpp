#include "entity.h"
#include "player.h"

#include "player_test.h"

using data = std::tuple<
    std::array<Entity, 3>,
    Players>;

static data gen_data() {
    data ret;
    auto &[e, p] = ret;
    for(auto &x : e)
        p.add(&x);
    return ret;
}

void PlayerTest::add() {
    const auto [entities, players] = gen_data();
    QCOMPARE(players.cur(), players.get(0));
    QCOMPARE(players.cur()->e, &entities[0]);
    QCOMPARE(players.get(0)->e, &entities[0]);
    QCOMPARE(players.get(1)->e, &entities[1]);
    QCOMPARE(players.get(2)->e, &entities[2]);
}

void PlayerTest::remove_data() {
    QTest::addColumn<size_t>("cur");
    QTest::addColumn<size_t>("i");
    QTest::addColumn<size_t>("next_cur");
    QTest::addColumn<size_t>("entity");
    const auto t = [](size_t c, size_t i, size_t n, size_t e) {
        const auto name = "c" + std::to_string(c) + " r" + std::to_string(i);
        QTest::newRow(name.c_str()) << c << i << n << e;
    };
    t(0, 0, 0, 1);
    t(0, 1, 0, 0);
    t(0, 2, 0, 0);
    t(1, 0, 0, 1);
    t(1, 1, 1, 2);
    t(1, 2, 1, 1);
    t(2, 0, 1, 2);
    t(2, 1, 1, 2);
    t(2, 2, 1, 1);
}

void PlayerTest::remove() {
    QFETCH(const size_t, cur);
    QFETCH(const size_t, i);
    QFETCH(const size_t, next_cur);
    QFETCH(const size_t, entity);
    auto [entities, players] = gen_data();
    players.set_cur(players.get(cur));
    players.remove(players.get(i));
    QCOMPARE(players.cur(), players.get(next_cur));
    QCOMPARE(players.cur()->e, &entities[entity]);
}

QTEST_MAIN(PlayerTest)

#include "collision_test.h"

#include "collision/collision.h"
#include "timing/timing.h"

#include "tests/tests.h"

Q_DECLARE_METATYPE(std::optional<nngn::vec3>)
Q_DECLARE_METATYPE(nngn::AABBCollider)
Q_DECLARE_METATYPE(nngn::BBCollider)
Q_DECLARE_METATYPE(nngn::SphereCollider)

namespace {

constexpr auto no_coll = std::optional<nngn::vec3>(std::nullopt);
std::optional<nngn::vec3> coll(float x, float y, float z = 0)
    { return {{x, y, z}}; }

}

void CollisionTest::aabb_collision_data() {
    const auto aabb = [](float x, float y)
        { return nngn::AABBCollider({x - .5f, y - .5f}, {x + .5f, y + .5f}); };
    QTest::addColumn<std::optional<nngn::vec3>>("coll");
    QTest::addColumn<nngn::AABBCollider>("c");
    QTest::newRow("n, l") << no_coll << aabb(-0.5f,  0.5f);
    QTest::newRow("n, r") << no_coll << aabb( 1.5f,  0.5f);
    QTest::newRow("n, b") << no_coll << aabb( 0.5f, -0.5f);
    QTest::newRow("n, t") << no_coll << aabb( 0.5f,  1.5f);
    QTest::newRow("y, l") << coll( 0.5f,  0.0f) << aabb(0.0f, 0.5f);
    QTest::newRow("y, r") << coll(-0.5f,  0.0f) << aabb(1.0f, 0.5f);
    QTest::newRow("y, b") << coll( 0.0f,  0.5f) << aabb(0.5f, 0.0f);
    QTest::newRow("y, t") << coll( 0.0f, -0.5f) << aabb(0.5f, 1.0f);
    QTest::newRow("n, l, bl") << no_coll << aabb(0.0f, -0.5f);
    QTest::newRow("n, l, tl") << no_coll << aabb(0.0f,  1.5f);
    QTest::newRow("n, r, br") << no_coll << aabb(1.0f, -0.5f);
    QTest::newRow("n, r, tr") << no_coll << aabb(1.0f,  1.5f);
    QTest::newRow("n, b, bl") << no_coll << aabb(-0.5f, 0.0f);
    QTest::newRow("n, b, br") << no_coll << aabb( 1.5f, 0.0f);
    QTest::newRow("n, t, tl") << no_coll << aabb(-0.5f, 1.0f);
    QTest::newRow("n, t, tr") << no_coll << aabb( 1.5f, 1.0f);
    QTest::newRow("y, l, bl") << coll( 0.25f, 0.0f) << aabb(-0.25f, 0.0f);
    QTest::newRow("y, l, tl") << coll( 0.25f, 0.0f) << aabb(-0.25f, 1.0f);
    QTest::newRow("y, r, br") << coll(-0.25f, 0.0f) << aabb( 1.25f, 0.0f);
    QTest::newRow("y, r, tr") << coll(-0.25f, 0.0f) << aabb( 1.25f, 1.0f);
    QTest::newRow("y, b, bl") << coll(0.0f,  0.25f) << aabb(0.0f, -0.25f);
    QTest::newRow("y, b, br") << coll(0.0f,  0.25f) << aabb(1.0f, -0.25f);
    QTest::newRow("y, t, tl") << coll(0.0f, -0.25f) << aabb(0.0f,  1.25f);
    QTest::newRow("y, t, tr") << coll(0.0f, -0.25f) << aabb(1.0f,  1.25f);
}

void CollisionTest::aabb_collision() {
    QFETCH(const nngn::AABBCollider, c);
    QFETCH(const std::optional<nngn::vec3>, coll);
    this->colliders.clear();
    this->colliders.set_max_colliders(2);
    this->colliders.set_max_collisions(1);
    this->colliders.add(nngn::AABBCollider({}, {1, 1}));
    this->colliders.add(c);
    QVERIFY(this->colliders.check_collisions({}));
    const auto ret = this->colliders.collisions();
    if(coll) {
        QVERIFY(!ret.empty());
        if(const auto v = ret[0].force; !fuzzy_eq(v, *coll))
            QCOMPARE(v, *coll);
    } else if(!ret.empty())
        QFAIL(toString(ret[0].force));
}

void CollisionTest::bb_collision_data() {
    const auto bb = [](float x, float y) {
        return nngn::BBCollider(
            {x - .5f, y - 1},
            {x + .5f, y + 1},
            0, 1);
    };
    QTest::addColumn<std::optional<nngn::vec3>>("coll");
    QTest::addColumn<nngn::BBCollider>("c");
    QTest::newRow("n, l") << no_coll << bb(-1.0f,  0.5f);
    QTest::newRow("n, r") << no_coll << bb( 3.0f,  0.5f);
    QTest::newRow("n, b") << no_coll << bb( 1.0f, -0.5f);
    QTest::newRow("n, t") << no_coll << bb( 1.0f,  1.5f);
    QTest::newRow("y, l") << coll( 0.5f,  0.0f) << bb(-0.5f, 0.5f);
    QTest::newRow("y, r") << coll(-0.5f,  0.0f) << bb( 2.5f, 0.5f);
    QTest::newRow("y, b") << coll( 0.0f,  0.5f) << bb(1.0f, 0.0f);
    QTest::newRow("y, t") << coll( 0.0f, -0.5f) << bb(1.0f, 1.0f);
    QTest::newRow("n, l, bl") << no_coll << bb(0.0f, -0.5f);
    QTest::newRow("n, l, tl") << no_coll << bb(0.0f,  1.5f);
    QTest::newRow("n, r, br") << no_coll << bb(2.0f, -0.5f);
    QTest::newRow("n, r, tr") << no_coll << bb(2.0f,  1.5f);
    QTest::newRow("n, b, bl") << no_coll << bb(-1.0f, 0.0f);
    QTest::newRow("n, b, br") << no_coll << bb( 3.0f, 0.0f);
    QTest::newRow("n, t, tl") << no_coll << bb(-1.0f, 1.0f);
    QTest::newRow("n, t, tr") << no_coll << bb( 3.0f, 1.0f);
    QTest::newRow("y, l, bl") << coll( 0.25f, 0.0f) << bb(-0.75f, 0.0f);
    QTest::newRow("y, l, tl") << coll( 0.25f, 0.0f) << bb(-0.75f, 1.0f);
    QTest::newRow("y, r, br") << coll(-0.25f, 0.0f) << bb( 2.75f, 0.0f);
    QTest::newRow("y, r, tr") << coll(-0.25f, 0.0f) << bb( 2.75f, 1.0f);
    QTest::newRow("y, b, bl") << coll(0.0f,  0.25f) << bb(0.0f, -0.25f);
    QTest::newRow("y, b, br") << coll(0.0f,  0.25f) << bb(2.0f, -0.25f);
    QTest::newRow("y, t, tl") << coll(0.0f, -0.25f) << bb(0.0f,  1.25f);
    QTest::newRow("y, t, tr") << coll(0.0f, -0.25f) << bb(2.0f,  1.25f);
}

void CollisionTest::bb_collision() {
    QFETCH(const nngn::BBCollider, c);
    QFETCH(const std::optional<nngn::vec3>, coll);
    this->colliders.clear();
    this->colliders.set_max_colliders(2);
    this->colliders.set_max_collisions(1);
    this->colliders.add(nngn::BBCollider({0.5f, -0.5f}, {1.5f, 1.5f}, 0, 1));
    this->colliders.add(c);
    QVERIFY(this->colliders.check_collisions(nngn::Timing{}));
    const auto ret = this->colliders.collisions();
    if(coll) {
        QVERIFY(!ret.empty());
        if(const auto v = ret[0].force; !fuzzy_eq(v, *coll))
            QCOMPARE(v, *coll);
    } else if(!ret.empty())
        QFAIL(toString(ret[0].force));
}

void CollisionTest::sphere_sphere_collision_data() {
    const auto sphere = [](float x, float y)
        { return nngn::SphereCollider({x, y, 0}, 0.5f); };
    QTest::addColumn<std::optional<nngn::vec3>>("coll");
    QTest::addColumn<nngn::SphereCollider>("c");
    QTest::newRow("n, l") << no_coll << sphere(0.5f, 1.5f);
    QTest::newRow("n, r") << no_coll << sphere(2.5f, 1.5f);
    QTest::newRow("n, b") << no_coll << sphere(1.5f, 0.5f);
    QTest::newRow("n, t") << no_coll << sphere(1.5f, 2.5f);
    QTest::newRow("y, l") << coll( 0.5f,  0.0f) << sphere(1.0f, 1.5f);
    QTest::newRow("y, r") << coll(-0.5f,  0.0f) << sphere(2.0f, 1.5f);
    QTest::newRow("y, b") << coll( 0.0f,  0.5f) << sphere(1.5f, 1.0f);
    QTest::newRow("y, t") << coll( 0.0f, -0.5f) << sphere(1.5f, 2.0f);
    const auto diag = std::pow(2.0f, -1.5f);
    QTest::newRow("y, bl")
        << coll( diag,  diag) << sphere(1.5f - diag, 1.5f - diag);
    QTest::newRow("y, tr")
        << coll(-diag, -diag) << sphere(1.5f + diag, 1.5f + diag);
    QTest::newRow("y, br")
        << coll( diag, -diag) << sphere(1.5f - diag, 1.5f + diag);
    QTest::newRow("y, tl")
        << coll(-diag,  diag) << sphere(1.5f + diag, 1.5f - diag);
}

void CollisionTest::sphere_sphere_collision() {
    QFETCH(const std::optional<nngn::vec3>, coll);
    QFETCH(const nngn::SphereCollider, c);
    this->colliders.clear();
    this->colliders.set_max_colliders(2);
    this->colliders.set_max_collisions(1);
    this->colliders.add(nngn::SphereCollider{{1.5f, 1.5f, 0}, .5f});
    this->colliders.add(c);
    QVERIFY(this->colliders.check_collisions(nngn::Timing{}));
    const auto ret = this->colliders.collisions();
    if(coll) {
        QVERIFY(!ret.empty());
        if(const auto v = ret[0].force; !fuzzy_eq(v, *coll))
            QCOMPARE(v, *coll);
    } else if(!ret.empty())
        QFAIL(toString(ret[0].force));
}

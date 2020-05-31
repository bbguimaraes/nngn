#include "collision_test.h"

#include "collision/collision.h"
#include "timing/timing.h"

#include "tests/tests.h"

Q_DECLARE_METATYPE(nngn::vec4)
Q_DECLARE_METATYPE(std::optional<nngn::vec3>)
Q_DECLARE_METATYPE(nngn::AABBCollider)
Q_DECLARE_METATYPE(nngn::BBCollider)
Q_DECLARE_METATYPE(nngn::SphereCollider)
Q_DECLARE_METATYPE(nngn::PlaneCollider)

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
        if(const auto v = ret[0].length * ret[0].normal; !fuzzy_eq(v, *coll))
            QCOMPARE(v, *coll);
    } else if(!ret.empty())
        QFAIL(toString(ret[0].length * ret[0].normal));
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
        if(const auto v = ret[0].length * ret[0].normal; !fuzzy_eq(v, *coll))
            QCOMPARE(v, *coll);
    } else if(!ret.empty())
        QFAIL(toString(ret[0].length * ret[0].normal));
}

void CollisionTest::sphere_sphere_collision_data() {
    const auto sphere = [](float x, float y)
        { return nngn::SphereCollider({x, y, 0}, 0.5f); };
    QTest::addColumn<std::optional<nngn::vec3>>("coll");
    QTest::addColumn<nngn::SphereCollider>("c");
    QTest::newRow("n") << no_coll << sphere(1.5f, 1.5f);
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
    // XXX
    this->colliders.set_max_colliders(4);
    this->colliders.set_max_collisions(8);
    this->colliders.add(nngn::SphereCollider{{1.5f, 1.5f, 0}, .5f});
    this->colliders.add(c);
    this->colliders.add(nngn::SphereCollider{});
    this->colliders.add(nngn::SphereCollider{});
    QVERIFY(this->colliders.check_collisions(nngn::Timing{}));
    const auto ret = this->colliders.collisions();
    if(coll) {
        QVERIFY(!ret.empty());
        if(const auto v = ret[0].length * ret[0].normal; !fuzzy_eq(v, *coll))
            QCOMPARE(v, *coll);
    } else if(!ret.empty())
        QFAIL(toString(ret[0].length * ret[0].normal));
}

// TODO
//void CollisionTest::plane_collision_data() {
//    QTest::addColumn<vec4>("v");
//    QTest::addColumn<bool>("coll");
//    QTest::addColumn<vec3>("p");
//    QTest::addColumn<vec3>("d");
//    QTest::newRow("equal")
//        << vec4(0, 1, 0, -1) << false << vec3(0) << vec3(0);
//    QTest::newRow("parallel")
//        << vec4(0, 1, 0, 0) << false << vec3(0) << vec3(0);
//    QTest::newRow("coll0")
//        << vec4(-1, 0, 0, 1) << true
//        << vec3(1, 1, 0) << vec3(0, 0, 1);
//    QTest::newRow("coll1")
//        << vec4(0, 0, 1, -1) << true
//        << vec3(0, 1, 1) << vec3(1, 0, 0);
//}
//
//void CollisionTest::plane_collision() {
//    const nngn::PlaneCollider c({0, 1, 0, -1});
//    QFETCH(const vec4, v);
//    QFETCH(const bool, coll);
//    QFETCH(const vec3, p);
//    QFETCH(const vec3, d);
//    this->colliders.clear();
//    this->colliders.set_max_colliders(2);
//    this->colliders.set_max_collisions(1);
//    this->colliders.add(c);
//    this->colliders.add(nngn::PlaneCollider(v));
//    QVERIFY(this->colliders.check_collisions(nngn::Timing{}));
//    vec3 tp = {}, td = {};
//    const auto ret = c.collision(, &tp, &td);
//    QCOMPARE(coll, ret);
//    QCOMPARE(tp, p);
//    QCOMPARE(td, d);
//}

void CollisionTest::plane_sphere_collision_data() {
    constexpr float r = .5;
    const auto diag_1 = nngn::vec2(1.0f / std::sqrt(2.0f));
    const auto diag_quarter = nngn::vec2(1.0f / std::sqrt(32.0f));
    const std::array p = {
        nngn::PlaneCollider({0,  1, 0}, {0,  1, 0, -1}),
        nngn::PlaneCollider({0, -1, 0}, {0, -1, 0, -1}),
        nngn::PlaneCollider({0, -1, 0}, {0,  1, 0,  1}),
        nngn::PlaneCollider({0,  1, 0}, {0, -1, 0,  1}),
        nngn::PlaneCollider({ diag_1, 0}, {diag_1, 0, -1}),
        nngn::PlaneCollider({-diag_1, 0}, {diag_1, 0,  1})};
    const std::array s = {
        nngn::SphereCollider({0, -1.50, 0}, r),
        nngn::SphereCollider({0, -1.25, 0}, r),
        nngn::SphereCollider({0, -0.75, 0}, r),
        nngn::SphereCollider({0, -0.50, 0}, r),
        nngn::SphereCollider({0,  0.00, 0}, r),
        nngn::SphereCollider({0,  0.50, 0}, r),
        nngn::SphereCollider({0,  0.75, 0}, r),
        nngn::SphereCollider({0,  1.25, 0}, r),
        nngn::SphereCollider({0,  1.50, 0}, r),
        nngn::SphereCollider({diag_1 * -1.50f, 0}, r),
        nngn::SphereCollider({diag_1 * -1.25f, 0}, r),
        nngn::SphereCollider({diag_1 * -0.75f, 0}, r),
        nngn::SphereCollider({diag_1 * -0.50f, 0}, r),
        nngn::SphereCollider({diag_1 * 1.50f, 0}, r),
        nngn::SphereCollider({diag_1 * 1.25f, 0}, r),
        nngn::SphereCollider({diag_1 * 0.75f, 0}, r),
        nngn::SphereCollider({diag_1 * 0.50f, 0}, r)};
    const auto
        no = nngn::vec4(),
        p25 = nngn::vec4(1, 0, .25f, 0),
        p75 = nngn::vec4(1, 0, .75f, 0),
        p1 = nngn::vec4(1, 0, 1, 0),
        n25 = nngn::vec4(1, 0, -.25f, 0),
        n75 = nngn::vec4(1, 0, -.75f, 0),
        n1 = nngn::vec4(1, 0, -1, 0),
        pd25 = nngn::vec4(1, diag_quarter, 0),
        pd75 = nngn::vec4(1, diag_1 - diag_quarter, 0),
        pd1 = nngn::vec4(1, diag_1, 0);
    QTest::addColumn<nngn::PlaneCollider>("c1");
    QTest::addColumn<nngn::SphereCollider>("c0");
    QTest::addColumn<nngn::vec4>("coll");
    QTest::newRow("p u-1 s +1.50") << p[0] << s[8] << no;
    QTest::newRow("p u-1 s +1.25") << p[0] << s[7] << p25;
    QTest::newRow("p u-1 s +0.75") << p[0] << s[6] << p75;
    QTest::newRow("p u-1 s +0.50") << p[0] << s[5] << p1;
    QTest::newRow("p d-1 s -1.50") << p[1] << s[0] << no;
    QTest::newRow("p d-1 s -1.25") << p[1] << s[1] << n25;
    QTest::newRow("p d-1 s -0.75") << p[1] << s[2] << n75;
    QTest::newRow("p d-1 s -0.50") << p[1] << s[3] << n1;
    QTest::newRow("p u+1 s -0.50") << p[2] << s[3] << no;
    QTest::newRow("p u+1 s -0.75") << p[2] << s[2] << p25;
    QTest::newRow("p u+1 s -1.25") << p[2] << s[1] << p75;
    QTest::newRow("p u+1 s -1.50") << p[2] << s[0] << p1;
    QTest::newRow("p d+1 s +0.50") << p[3] << s[5] << no;
    QTest::newRow("p d+1 s +0.75") << p[3] << s[6] << n25;
    QTest::newRow("p d+1 s +1.25") << p[3] << s[7] << n75;
    QTest::newRow("p d+1 s +1.50") << p[3] << s[8] << n1;
    QTest::newRow("p r-1 s d*1.50") << p[4] << s[13] << no;
    QTest::newRow("p r-1 s d*1.25") << p[4] << s[14] << pd25;
    QTest::newRow("p r-1 s d*0.75") << p[4] << s[15] << pd75;
    QTest::newRow("p r-1 s d*0.50") << p[4] << s[16] << pd1;
    QTest::newRow("p r+1 s d*-0.50") << p[5] << s[12] << no;
    QTest::newRow("p r+1 s d*-0.75") << p[5] << s[11] << pd25;
    QTest::newRow("p r+1 s d*-1.25") << p[5] << s[10] << pd75;
    QTest::newRow("p r+1 s d*-1.50") << p[5] << s[ 9] << pd1;
}

void CollisionTest::plane_sphere_collision() {
    QFETCH(const nngn::PlaneCollider, c1);
    QFETCH(const nngn::SphereCollider, c0);
    QFETCH(const nngn::vec4, coll);
    this->colliders.clear();
    this->colliders.set_max_colliders(2);
    this->colliders.set_max_collisions(1);
    this->colliders.add(c0);
    this->colliders.add(c1);
    QVERIFY(this->colliders.check_collisions(nngn::Timing{}));
    const auto ret = this->colliders.collisions();
    nngn::vec4 cmp = {};
    if(!ret.empty())
        cmp = {1, ret[0].length * ret[0].normal};
    if(!(qFuzzyCompare(cmp[0], coll[0])
            && qFuzzyCompare(cmp[1], coll[1])
            && qFuzzyCompare(cmp[2], coll[2])))
        QCOMPARE(cmp, coll);
}

void CollisionTest::gravity_collision_data() {
    const auto s = [](const nngn::vec3 &p, float m = 1.0f) {
        nngn::SphereCollider ret(p, .5f);
        ret.m = m;
        ret.flags.set(nngn::Collider::Flag::SOLID);
        return ret;
    };
    constexpr auto G = nngn::GravityCollider::G;
    QTest::addColumn<nngn::SphereCollider>("c0");
    QTest::addColumn<nngn::vec4>("coll");
    QTest::newRow("+0")   << s({ 2,  1, 0}) << nngn::vec4(1, -G, 0, 0);
    QTest::newRow("-0")   << s({ 0,  1, 0}) << nngn::vec4(1,  G, 0, 0);
    QTest::newRow("0+")   << s({ 1,  2, 0}) << nngn::vec4(1, 0, -G, 0);
    QTest::newRow("0-")   << s({ 1,  0, 0}) << nngn::vec4(1, 0,  G, 0);
    QTest::newRow("++0")  << s({ 3,  1, 0}) << nngn::vec4();
    QTest::newRow("--0")  << s({-1,  1, 0}) << nngn::vec4();
    QTest::newRow("0++")  << s({ 1,  3, 0}) << nngn::vec4();
    QTest::newRow("0--")  << s({ 1, -1, 0}) << nngn::vec4();
    QTest::newRow("m") << s({ 2,  1, 0}, 2.0f) << nngn::vec4(1, -2 * G, 0, 0);
}

void CollisionTest::gravity_collision() {
    QFETCH(const nngn::SphereCollider, c0);
    QFETCH(const nngn::vec4, coll);
    this->colliders.clear();
    this->colliders.set_max_colliders(2);
    this->colliders.set_max_collisions(1);
    auto c1 = nngn::GravityCollider({1, 1, 0}, 1.0f, 1.0f);
    c1.flags.set(nngn::Collider::Flag::SOLID);
    this->colliders.add(c0);
    this->colliders.add(c1);
    QVERIFY(this->colliders.check_collisions(nngn::Timing{}));
    const auto ret = this->colliders.collisions();
    nngn::vec4 cmp = {};
    if(!ret.empty())
        cmp = {1, ret[0].length * ret[0].normal};
    if(!(qFuzzyCompare(cmp[0], coll[0])
            && qFuzzyCompare(cmp[1], coll[1])
            && qFuzzyCompare(cmp[2], coll[2])))
        QCOMPARE(cmp, coll);
}

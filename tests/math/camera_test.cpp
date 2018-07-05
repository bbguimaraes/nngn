#include "math/camera.h"
#include "timing/timing.h"

#include "tests/tests.h"

#include "camera_test.h"

using namespace std::chrono_literals;

using nngn::mat4;
using nngn::vec3;

Q_DECLARE_METATYPE(vec3)

void CameraTest::look_at_data() {
    constexpr auto pi_4 = nngn::Math::pi<float>() / 4.0f;
    QTest::addColumn<vec3>("eye");
    QTest::addColumn<vec3>("up");
    QTest::addColumn<vec3>("rot");
    QTest::newRow("no") << vec3(0, 0, 1) << vec3(0, 1, 0) << vec3();
    QTest::newRow("x") << vec3(0, -1, 1) << vec3(0, 1, 0) << vec3(pi_4, 0, 0);
    QTest::newRow("y") << vec3(1, 0, 1) << vec3(0, 1, 0) << vec3(0, pi_4, 0);
}

void CameraTest::look_at() {
    QFETCH(const vec3, eye);
    QFETCH(const vec3, up);
    QFETCH(const vec3, rot);
    nngn::Camera c;
    c.look_at({}, eye, up);
    if(!fuzzy_eq(c.r, rot))
        QCOMPARE(c.r, rot);
}

void CameraTest::view() {
    nngn::Camera c;
    c.flags.clear(nngn::Camera::Flag::PERSPECTIVE);
    c.screen = {800, 600};
    constexpr auto far = -nngn::Camera::FAR / 2;
    auto m = nngn::Math::translate(mat4(1), {0, 0, far});
    QCOMPARE(c.gen_view(), m);
    c.p = {1, 2, 3};
    m = nngn::Math::translate(mat4(1), {-1, -2, -3 + far});
    QCOMPARE(c.gen_view(), m);
    c.zoom = 2;
    m = nngn::Math::translate(
        nngn::Math::scale(mat4(1), vec3(c.zoom, c.zoom, 1)),
        {-1, -2, -3 + far});
    QCOMPARE(c.gen_view(), m);
}

void CameraTest::view_perspective() {
    nngn::Camera c;
    c.flags |= nngn::Camera::Flag::PERSPECTIVE;
    c.screen = {800, 600};
    auto m = nngn::Math::translate(mat4(1), {0, 0, 0});
    QCOMPARE(c.gen_view(), m);
    c.p = {1, 2, 3};
    m = nngn::Math::translate(mat4(1), {-1, -2, -3});
    QCOMPARE(c.gen_view(), m);
    c.zoom = 2;
    m = nngn::Math::translate(
        nngn::Math::scale(mat4(1), vec3(c.zoom, c.zoom, 1)),
        {-1, -2, -3});
    QCOMPARE(c.gen_view(), m);
}

void CameraTest::update() {
    nngn::Timing t;
    t.dt = 100ms;
    nngn::Camera c;
    c.a = {2, 2, 2};
    for(unsigned int i = 0; i < 10; ++i)
        c.update(t);
    const auto cmp = [](const auto &l, const auto &r) {
        return qFuzzyCompare(l[0], r[0])
            && qFuzzyCompare(l[1], r[1])
            && qFuzzyCompare(l[2], r[2]);
    };
    if(!cmp(c.v, vec3(2, 2, 2)))
        QCOMPARE(c.v, vec3(2, 2, 2));
    if(!cmp(c.p, vec3(1, 1, 1)))
        QCOMPARE(c.p, vec3(1, 1, 1));
    c.v = {0, 0, 0};
    c.p = {0, 0, 0};
    t.dt = 1000ms;
    c.update(t);
    QCOMPARE(c.v, vec3(2, 2, 2));
    QCOMPARE(c.p, vec3(1, 1, 1));
    c.update(t);
    QCOMPARE(c.v, vec3(4, 4, 4));
    QCOMPARE(c.p, vec3(4, 4, 4));
}

void CameraTest::update_max_vel() {
    nngn::Timing t;
    t.dt = 1000ms;
    nngn::Camera c;
    c.max_v = 5;
    c.a = {0, 3, 4};
    c.update(t);
    QCOMPARE(c.v, vec3(0, 3, 4));
    c.update(t);
    QCOMPARE(c.v, vec3(0, 3, 4));
}

void CameraTest::damp() {
    nngn::Timing t;
    t.dt = 500ms;
    nngn::Camera c;
    c.max_v = 1;
    c.v = {1, 0, 0};
    c.damp = 1;
    c.update(t);
    QCOMPARE(c.v, vec3(0.5f, 0, 0));
    t.dt = 1000ms;
    c.update(t);
    QCOMPARE(c.v, vec3(0, 0, 0));
}

void CameraTest::proj() {
    constexpr float w = 800, h = 600;
    constexpr float n = nngn::Camera::NEAR, f = nngn::Camera::FAR;
    constexpr float zd = f - n, zs = f + n;
    nngn::Camera c;
    c.flags.clear(nngn::Camera::Flag::PERSPECTIVE);
    c.screen = static_cast<nngn::uvec2>(nngn::vec2{w, h});
    const auto proj = c.gen_proj();
    const auto m = nngn::Math::transpose(mat4({
        {2.0f/w,      0,     0,      0},
        {     0, 2.0f/h,     0,      0},
        {     0,      0, -2/zd, -zs/zd},
        {     0,      0,     0,      1},
    }));
    for(int i = 0; i < 16; ++i) {
        const auto x = i % 4, y = i / 4;
        if(!qFuzzyCompare(proj[y][x] + 1, m[y][x] + 1))
            QCOMPARE(proj, m);
    }
}

void CameraTest::proj_perspective() {
    nngn::Camera c;
    c.flags |= nngn::Camera::Flag::PERSPECTIVE;
    c.screen = {};
    QCOMPARE(c.gen_proj(), mat4(1));
    c.screen = {800, 600};
    const float n = nngn::Camera::NEAR, f = nngn::Camera::FAR;
    const float uh = 1.0f / std::tan(c.fov_y / 2.0f);
    const float uw = uh
        * static_cast<float>(c.screen.y)
        / static_cast<float>(c.screen.x);
    const auto proj = c.gen_proj();
    const auto m = nngn::Math::transpose(mat4({
        {uw,  0,           0,           0},
        { 0, uh,           0,           0},
        { 0,  0, (f+n)/(n-f), 2*f*n/(n-f)},
        { 0,  0,          -1,           0},
    }));
    for(int i = 0; i < 16; ++i) {
        const auto x = i % 4, y = i / 4;
        if(!qFuzzyCompare(proj[y][x] + 1, m[y][x] + 1))
            QCOMPARE(proj, m);
    }
}

void CameraTest::perspective() {
    nngn::Camera c;
    c.screen = {800, 600};
    c.set_perspective(true);
    const float t = std::tan(c.fov_y / 2.0f);
    QCOMPARE(c.zoom, 1.0f);
    QCOMPARE(c.p.z, 300 / t);
    c.set_perspective(false);
    QCOMPARE(c.zoom, 1.0f);
    QCOMPARE(c.p.z, 0.0f);
    c.set_perspective(true);
    QCOMPARE(c.zoom, 1.0f);
    QCOMPARE(c.p.z, 300 / t);
}

void CameraTest::perspective_zoom() {
    nngn::Camera c;
    c.screen = {800, 600};
    c.zoom = 2;
    c.set_perspective(true);
    const float t = std::tan(c.fov_y / 2.0f);
    QCOMPARE(c.zoom, 1.0f);
    QCOMPARE(c.p.z, 150 / t);
    c.set_perspective(false);
    QCOMPARE(c.zoom, 2.0f);
    QCOMPARE(c.p.z, 0.0f);
    c.set_perspective(true);
    QCOMPARE(c.zoom, 1.0f);
    QCOMPARE(c.p.z, 150 / t);
}

void CameraTest::eye() {
    vec3 eye = {1, 2, 3};
    nngn::Camera c;
    c.screen = {800, 600};
    c.set_perspective(true);
    c.p = eye;
    QCOMPARE(c.eye(), eye);
    c.set_perspective(false);
    QCOMPARE(c.eye(), eye);
}

QTEST_MAIN(CameraTest)

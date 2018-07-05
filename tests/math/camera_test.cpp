#include "math/camera.h"
#include "timing/timing.h"

#include "tests/tests.h"

#include "camera_test.h"

using namespace std::chrono_literals;

using nngn::uvec2, nngn::vec2, nngn::vec3, nngn::vec4, nngn::mat4;
using nngn::Camera, nngn::Math;
using Flag = nngn::Camera::Flag;

Q_DECLARE_METATYPE(vec2)
Q_DECLARE_METATYPE(vec3)
Q_DECLARE_METATYPE(mat4)

namespace {

constexpr unsigned SCREEN_W = 800;
constexpr unsigned SCREEN_H = 600;
constexpr float Z = 519.615242270663f; // SCREEN_H / 2 * tan(Camera::FOVY / 2)

}

void CameraTest::fov_data(void) {
    QTest::addColumn<float>("w");
    QTest::addColumn<float>("z");
    QTest::newRow("h") << static_cast<float>(SCREEN_H) << Z;
}

void CameraTest::fov(void) {
    QFETCH(const float, w);
    QFETCH(const float, z);
    QCOMPARE(Camera::fov(z, w), Camera::FOVY);
}

void CameraTest::center_up_data(void) {
    constexpr float pi = Math::pi<float>();
    constexpr float pi_4 = pi / 4;
    constexpr float sq2 = Math::sq2<float>();
    QTest::addColumn<vec3>("rot");
    QTest::addColumn<vec3>("center");
    QTest::addColumn<vec3>("up");
    QTest::newRow("-z")
        << vec3{}
        << vec3{1, 1, 0}
        << vec3{0, 1, 0};
    QTest::newRow("z")
        << vec3{0, pi, 0}
        << vec3{1, 1, 2}
        << vec3{0, 1, 0};
    QTest::newRow("x rotation")
        << vec3{pi_4, 0, 0}
        << vec3{1, 1 + sq2 / 2, 1 + sq2 / -2}
        << vec3{0,     sq2 / 2,     sq2 /  2};
    QTest::newRow("xy")
        << vec3{pi_4, -pi_4, 0}
        << vec3{ 1.5, 1 + sq2 / 2, 0.5}
        << vec3{-0.5,     sq2 / 2, 0.5};
}

void CameraTest::center_up(void) {
    QFETCH(const vec3, rot);
    QFETCH(const vec3, center);
    QFETCH(const vec3, up);
    Camera c = {.p = {1, 1, 1}, .rot = rot};
    c.set_perspective(true);
    if(const auto ret = c.center(); !fuzzy_eq(ret, center))
        QCOMPARE(ret, center);
    if(const auto ret = c.up(); !fuzzy_eq(ret, up))
        QCOMPARE(ret, up);
    c.set_perspective(false);
    if(const auto ret = c.center(); !fuzzy_eq(ret, center))
        QCOMPARE(ret, center);
    if(const auto ret = c.up(); !fuzzy_eq(ret, up))
        QCOMPARE(ret, up);
}

void CameraTest::world_to_view(void) {
    Camera c = {
        .p = {0, 0, -1},
        .rot = {0, Math::pi<float>(), 0},
        .screen = {1, 1},
    };
    c.flags.set(Flag::UPDATED),
    c.update({});
    constexpr vec3 world = {0, 0, -1};
    constexpr vec3 view = {};
    if(const auto ret = c.world_to_view(world); !fuzzy_eq(ret, view))
        QCOMPARE(ret, view);
    if(const auto ret = c.view_to_world(view); !fuzzy_eq(ret, world))
        QCOMPARE(ret, world);
}

void CameraTest::view_to_clip(void) {
    Camera c = {.screen = {SCREEN_W, SCREEN_H}, .flags = Flag::UPDATED};
    c.update({});
    constexpr vec3 view = {200, 150, Camera::FAR / -4};
    constexpr vec3 clip = {0.5, 0.5, 0.5};
    if(const auto ret = c.view_to_clip(view); !fuzzy_eq(ret, clip))
        QCOMPARE(ret, clip);
    if(const auto ret = c.clip_to_view(clip); !fuzzy_eq(ret, view))
        QCOMPARE(ret, view);
}

void CameraTest::clip_to_screen(void) {
    Camera c = {.screen = {SCREEN_W, SCREEN_H}, .flags = Flag::UPDATED};
    c.update({});
    constexpr vec3 clip = {};
    constexpr vec2 screen = {SCREEN_W / 2, SCREEN_H / 2};
    if(const auto ret = c.clip_to_screen(clip); !fuzzy_eq(ret, screen))
        QCOMPARE(ret, screen);
    if(const auto ret = c.screen_to_clip(screen); !fuzzy_eq(ret, clip))
        QCOMPARE(ret, clip);
}

void CameraTest::screen_clip_view_world_data(void) {
    QTest::addColumn<vec2>("pos");
    QTest::addColumn<vec2>("screen");
    QTest::addColumn<vec3>("clip");
    QTest::addColumn<vec3>("view");
    QTest::addColumn<vec3>("world");
    constexpr float far_z = Camera::FAR / -2;
    constexpr float world_z = Z + far_z;
    constexpr vec3 o = {};
    constexpr vec2 screen_bl = o.xy();
    constexpr vec2 screen_o = {SCREEN_W / 2, SCREEN_H / 2};
    constexpr vec2 screen_tr = {SCREEN_W, SCREEN_H};
    constexpr vec2 camera_bl = -screen_o.xy();
    constexpr vec2 camera_tr =  screen_o.xy();
    constexpr vec3 clip_bl = {-1, -1, 0};
    constexpr vec3 clip_tr = { 1,  1, 0};
    constexpr vec3 view_bl = {-screen_o.xy(), -Z};
    constexpr vec3 view_o = {0, 0, -Z};
    constexpr vec3 view_tr = { screen_o.xy(), -Z};
    constexpr float camera_bl_view_z = -Z - screen_o.y;
    constexpr float camera_tr_view_z = -Z + screen_o.y;
    constexpr vec3 world_bl = {-screen_o.xy(), 0};
    constexpr vec3 world_tr = {screen_o.xy(), 0};
    constexpr vec3 world_bl2 = {-2.0f * screen_o.xy(), world_z};
    constexpr vec3 world_tr2 = { 2.0f * screen_o.xy(), world_z};
    constexpr float camera_bl_world_z = screen_o.y;
    constexpr float camera_tr_world_z = -screen_o.y;
    QTest::newRow("camera: o, point: c")
        << o.xy() << screen_o << o << view_o << o;
    QTest::newRow("camera: o, point: bl")
        << o.xy() << screen_bl << clip_bl << view_bl << world_bl;
    QTest::newRow("camera: o, point: tr")
        << o.xy() << screen_tr << clip_tr << view_tr << world_tr;
    QTest::newRow("camera: bl, point: c")
        << camera_bl << screen_o << o
        << vec3{view_o.xy(), camera_tr_view_z}
        << vec3{-screen_o.xy(), camera_bl_world_z};
    QTest::newRow("camera: bl, point: bl")
        << camera_bl << screen_bl << clip_bl
        << vec3{view_bl.xy(), camera_tr_view_z}
        << vec3{world_bl2.xy(), camera_bl_world_z};
    QTest::newRow("camera: bl, point: tr")
        << camera_bl << screen_tr << clip_tr
        << vec3{view_tr.xy(), camera_tr_view_z}
        << vec3{o.xy(), camera_bl_world_z};
    QTest::newRow("camera: tr, point: c")
        << camera_tr << screen_o << o
        << vec3{view_o.xy(), camera_bl_view_z}
        << vec3{world_tr.xy(), camera_tr_world_z};
    QTest::newRow("camera: tr, point: bl")
        << camera_tr << screen_bl << clip_bl
        << vec3{view_bl.xy(), camera_bl_view_z}
        << vec3{o.xy(), camera_tr_world_z};
    QTest::newRow("camera: tr, point: tr")
        << camera_tr << screen_tr << clip_tr
        << vec3{view_tr.xy(), camera_bl_view_z}
        << vec3{world_tr2.xy(), camera_tr_world_z};
}

void CameraTest::screen_clip_view_world(void) {
    QFETCH(const vec2, pos);
    QFETCH(const vec2, screen);
    QFETCH(const vec3, clip);
    QFETCH(const vec3, view);
    QFETCH(const vec3, world);
    Camera c;
    c.set_screen({SCREEN_W, SCREEN_H});
    c.set_pos({pos, c.z_for_fov()});
    c.update({});
    const auto screen_to_clip = c.screen_to_clip(screen);
    if(!fuzzy_eq(screen_to_clip, clip))
        QCOMPARE(screen_to_clip, clip);
    const auto clip_to_view = c.clip_to_view(screen_to_clip);
    if(!fuzzy_eq(clip_to_view, view))
        QCOMPARE(clip_to_view, view);
    const auto view_to_world = c.view_to_world(clip_to_view);
    if(!fuzzy_eq(view_to_world, world, 1_u32 << 11))
        QCOMPARE(view_to_world, world);
    const auto world_to_view = c.world_to_view(view_to_world);
    if(!fuzzy_eq(world_to_view, view))
        QCOMPARE(world_to_view, view);
    const auto view_to_clip = c.view_to_clip(world_to_view);
    if(!fuzzy_eq(view_to_clip, clip))
        QCOMPARE(view_to_clip, clip);
    const auto clip_to_screen = c.clip_to_screen(view_to_clip);
    if(!fuzzy_eq(clip_to_screen, screen))
        QCOMPARE(clip_to_screen, screen);
}

void CameraTest::z_for_fov(void) {
    constexpr Camera c = {.screen = {SCREEN_W, SCREEN_H}};
    if(const auto ret = c.z_for_fov(); !fuzzy_eq(ret, Z))
        QCOMPARE(ret, Z);
}

void CameraTest::scale_for_fov(void) {
    constexpr Camera c = {
        .p = {0, 0, 2.0f * Z},
        .screen = {SCREEN_W, SCREEN_H},
    };
    constexpr float scale = 0.5f;
    if(const auto ret = c.scale_for_fov(); !fuzzy_eq(ret, scale))
        QCOMPARE(ret, scale);
}

void CameraTest::fov_z(void) {
    Camera c;
    c.fov_y = Math::pi<float>() / 4;
    c.set_screen({8, 8});
    c.set_perspective(true);
    c.p.z = 8;
    c.set_fov_z(8);
    c.update({});
    const auto proj = [&c] {
        return Math::perspective_transform(c.proj * c.view, {2, 2, 0}).xy();
    };
    constexpr vec2 clip = {0.5, 0.5};
    if(const auto ret = proj(); !fuzzy_eq(ret, clip))
        QCOMPARE(ret, clip);
    c.set_screen({4, 4});
    if(const auto ret = proj(); !fuzzy_eq(ret, clip))
        QCOMPARE(ret, clip);
}

void CameraTest::look_at_data(void) {
    constexpr auto pi_4 = Math::pi<float>() / 4.0f;
    constexpr auto sq2_2 = Math::sq2_2<float>();
    QTest::addColumn<vec3>("pos");
    QTest::addColumn<vec3>("up");
    QTest::addColumn<vec3>("rot");
    QTest::newRow("no") << vec3{0, 0, 1} << vec3{0, 1, 0} << vec3{};
    QTest::newRow("x") << vec3{0, -1, 1} << vec3{0, 1, 0} << vec3{pi_4, 0, 0};
    QTest::newRow("y") << vec3{1, 0, 1} << vec3{0, 1, 0} << vec3{0, pi_4, 0};
    QTest::newRow("z")
        << vec3{0, 0, 1} << vec3{-sq2_2, sq2_2, 0} << vec3{0, 0, pi_4};
}

void CameraTest::look_at(void) {
    QFETCH(const vec3, pos);
    QFETCH(const vec3, up);
    QFETCH(const vec3, rot);
    Camera c;
    c.look_at({}, pos, up);
    if(!fuzzy_eq(c.rot, rot))
        QCOMPARE(c.rot, rot);
}

void CameraTest::proj(void) {
    Camera c;
    c.set_screen({SCREEN_W, SCREEN_H});
    c.update({});
    constexpr float x = SCREEN_W / 2;
    constexpr float y = SCREEN_H / 2;
    constexpr float z = Camera::FAR / 2;
    const auto ortho = Math::ortho(-x, x, -y, y, -z, z);
    if(!fuzzy_eq(c.proj, ortho))
        QCOMPARE(c.proj, ortho);
}

void CameraTest::proj_perspective(void) {
    Camera c;
    c.set_perspective(true);
    c.set_screen({});
    c.update({});
    QCOMPARE(c.proj, mat4{1});
    c.set_screen({SCREEN_W, SCREEN_H});
    c.update({});
    const auto persp = Math::perspective(
        Camera::FOVY,
        static_cast<float>(SCREEN_W) / static_cast<float>(SCREEN_H),
        Camera::NEAR, Camera::FAR);
    if(!fuzzy_eq(c.proj, persp))
        QCOMPARE(c.proj, persp);
}

void CameraTest::view_data(void) {
    QTest::addColumn<vec3>("pos");
    QTest::addColumn<float>("zoom");
    QTest::addColumn<mat4>("view");
    QTest::newRow("zoom")
        << vec3{0, 0, Z} << 2.0f
        << Math::translate(
            Math::scale(mat4{1}, {2, 2, 1}),
            {0, 0, -Z});
    QTest::newRow("pos zoom")
        << vec3{1, 2, 3} << 2.0f
        << Math::translate(
            Math::scale(mat4{1}, {2 * Z / 3, 2 * Z / 3, 1}),
            {-1, -2, -3});
    QTest::newRow("pos")
        << vec3{1, 2, 3} << 1.0f
        << Math::translate(
            Math::scale(mat4{1}, {Z / 3, Z / 3, 1}),
            {-1, -2, -3});
}

void CameraTest::view(void) {
    QFETCH(const vec3, pos);
    QFETCH(const float, zoom);
    QFETCH(const mat4, view);
    Camera c;
    c.set_zoom(zoom);
    c.set_screen({SCREEN_W, SCREEN_H});
    c.set_pos(pos);
    c.update({});
    if(!fuzzy_eq(c.view, view))
        QCOMPARE(c.view, view);
}

void CameraTest::view_perspective_data(void) {
    QTest::addColumn<vec3>("pos");
    QTest::addColumn<float>("zoom");
    QTest::addColumn<mat4>("view");
    QTest::newRow("zoom")
        << vec3{0, 0, Z} << 2.0f
        << Math::translate(
            Math::scale(mat4{1}, {2, 2, 1}),
            {0, 0, -Z});
    QTest::newRow("pos zoom")
        << vec3{1, 2, 3} << 2.0f
        << Math::translate(
            Math::scale(mat4{1}, {2, 2, 1}),
            {-1, -2, -3});
    QTest::newRow("pos")
        << vec3{1, 2, 3} << 1.0f
        << Math::translate(mat4{1}, {-1, -2, -3});
}

void CameraTest::view_perspective(void) {
    QFETCH(const vec3, pos);
    QFETCH(const float, zoom);
    QFETCH(const mat4, view);
    Camera c;
    c.set_zoom(zoom);
    c.set_screen({SCREEN_W, SCREEN_H});
    c.set_pos(pos);
    c.set_perspective(true);
    c.update({});
    if(!fuzzy_eq(c.view, view))
        QCOMPARE(c.view, view);
}

void CameraTest::ortho_to_persp(void) {
    constexpr unsigned width = 8;
    constexpr float zoom = 2;
    Camera c = {.zoom = zoom, .screen = {width, width}};
    const auto z = c.z_for_fov();
    c.p.z = z;
    c.flags.set(Flag::UPDATED),
    c.update({});
    constexpr auto mul = [](auto proj, auto view) {
        return Math::perspective_transform(proj * view, vec3{2, 2, 0}).xy();
    };
    constexpr vec2 ortho = {1, 1};
    if(const auto ret = mul(c.proj, c.view); !fuzzy_eq(ret, ortho))
        QCOMPARE(ret, ortho);
    c.set_perspective(true);
    c.update({});
    constexpr vec2 persp = {1, 1};
    if(const auto ret = mul(c.proj, c.view); !fuzzy_eq(ret, persp))
        QCOMPARE(ret, persp);
}

void CameraTest::update(void) {
    nngn::Timing t;
    t.dt = 100ms;
    Camera c = {.max_v = 8};
    c.a = {2, 2, 2};
    for(unsigned i = 0; i < 10; ++i)
        c.update(t);
    constexpr vec3 v = {2, 2, 2}, p = {1, 1, 1};
    if(!fuzzy_eq(c.v, v))
        QCOMPARE(c.v, v);
    if(!fuzzy_eq(c.p, p))
        QCOMPARE(c.p, p);
    c.v = {0, 0, 0};
    c.p = {0, 0, 0};
    t.dt = 1000ms;
    c.update(t);
    QCOMPARE(c.v, v);
    QCOMPARE(c.p, p);
    c.update(t);
    constexpr vec3 v2 = {4, 4, 4}, p2 = {4, 4, 4};
    QCOMPARE(c.v, v2);
    QCOMPARE(c.p, p2);
}

void CameraTest::update_max_vel(void) {
    nngn::Timing t;
    t.dt = 1000ms;
    Camera c;
    c.max_v = 5;
    c.a = {0, 3, 4};
    c.update(t);
    QCOMPARE(c.v, (vec3{0, 3, 4}));
    c.update(t);
    QCOMPARE(c.v, (vec3{0, 3, 4}));
}

void CameraTest::damp(void) {
    nngn::Timing t;
    t.dt = 500ms;
    Camera c;
    c.max_v = 1;
    c.v = {1, 0, 0};
    c.damp = 1;
    c.update(t);
    QCOMPARE(c.v, (vec3{0.5f, 0, 0}));
    t.dt = 1000ms;
    c.update(t);
    QCOMPARE(c.v, (vec3{0, 0, 0}));
}

QTEST_MAIN(CameraTest)

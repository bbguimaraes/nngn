#include "entity.h"

#include "render/light.h"
#include "timing/timing.h"
#include "utils/log.h"

#include "light_test.h"

using namespace std::chrono_literals;

using nngn::vec3;
using nngn::vec4;

void LightTest::add_light() {
    nngn::Lighting l;
    size_t n_dir = 0, n_point = 0;
    for(size_t i = 0, n = nngn::Lighting::MAX_LIGHTS * 2; i < n; ++i) {
        if(i % 2) {
            nngn::Light *x = l.add_light(nngn::Light::Type::DIR);
            QCOMPARE(&l.dir_lights()[n_dir++], x);
        } else {
            nngn::Light *x = l.add_light(nngn::Light::Type::POINT);
            QCOMPARE(&l.point_lights()[n_point++], x);
        }
        QCOMPARE(l.dir_lights().size(), n_dir);
        QCOMPARE(l.point_lights().size(), n_point);
    }
}

void LightTest::remove_light() {
    constexpr auto max = nngn::Lighting::MAX_LIGHTS;
    nngn::Lighting l;
    for(size_t i = 0, n = max * 2; i < n; ++i)
        l.add_light(i % 2 ? nngn::Light::Type::POINT : nngn::Light::Type::DIR);
    size_t n_dir = max, n_point = max;
    for(size_t i = max * 2; i; --i) {
        if(i % 2)
            l.remove_light(&l.point_lights()[--n_point]);
        else
            l.remove_light(&l.dir_lights()[--n_dir]);
        QCOMPARE(l.dir_lights().size(), n_dir);
        QCOMPARE(l.point_lights().size(), n_point);
    }
}

void LightTest::remove_add_light() {
    nngn::Lighting l;
    l.add_light(nngn::Light::Type::DIR)->color = {0, 0, 0, 1};
    l.add_light(nngn::Light::Type::DIR)->color = {1, 1, 1, 1};
    l.add_light(nngn::Light::Type::DIR)->color = {2, 2, 2, 1};
    l.remove_light(&l.dir_lights()[1]);
    QCOMPARE(l.dir_lights()[0].color, vec4(0, 0, 0, 1));
    QCOMPARE(l.dir_lights()[1].color, vec4(2, 2, 2, 1));
    l.add_light(nngn::Light::Type::DIR)->color = {3, 3, 3, 1};
    QCOMPARE(l.dir_lights()[0].color, vec4(0, 0, 0, 1));
    QCOMPARE(l.dir_lights()[1].color, vec4(2, 2, 2, 1));
    QCOMPARE(l.dir_lights()[2].color, vec4(3, 3, 3, 1));
    for(size_t i = 3; i < nngn::Lighting::MAX_LIGHTS; ++i)
        QCOMPARE(l.dir_lights()[i].color, vec4(0, 0, 0, 0));
}

void LightTest::max_lights() {
    nngn::Lighting l;
    for(size_t i = 0; i < nngn::Lighting::MAX_LIGHTS; ++i)
        QVERIFY(l.add_light(nngn::Light::Type::DIR));
    const auto log = nngn::Log::capture([&l]() {
        QCOMPARE(l.add_light(nngn::Light::Type::DIR), nullptr); });
    QCOMPARE(l.dir_lights().size(), nngn::Lighting::MAX_LIGHTS);
    QCOMPARE(
        log.c_str(),
        "Lighting::add_light: cannot add more dir lights\n");
}

void LightTest::ubo_ambient_light() {
    nngn::Timing t;
    nngn::Lighting l;
    l.update(t);
    QCOMPARE(l.ubo().ambient, vec3(1.0f));
    constexpr vec4 v = {1, 2, 3, 4};
    l.set_ambient_light(v);
    l.update(t);
    QCOMPARE(l.ubo().ambient, v.xyz() * 4.0f);
}

void LightTest::ubo_n() {
    nngn::Timing t;
    nngn::Lighting l;
    l.update(t);
    QCOMPARE(l.ubo().n_dir, 0);
    QCOMPARE(l.ubo().n_point, 0);
}

void LightTest::update_dir() {
    nngn::Timing t;
    nngn::Lighting l;
    l.update(t);
    nngn::Light *x = l.add_light(nngn::Light::Type::DIR);
    constexpr vec3 dir = {1, 2, 3};
    constexpr vec4 color = {4, 5, 6, 7};
    constexpr float spec = 8;
    constexpr vec4 color_spec = {4*7, 5*7, 6*7, spec};
    x->dir = dir;
    x->color = color;
    x->spec = spec;
    QVERIFY(x->updated);
    l.update(t);
    QVERIFY(!x->updated);
    QCOMPARE(l.ubo().n_dir, 1);
    auto d = l.ubo().dir;
    QCOMPARE(d.dir[0], vec4(dir, 0));
    QCOMPARE(d.color_spec[0], color_spec);
}

void LightTest::update_point() {
    nngn::Timing t;
    nngn::Lighting l;
    l.update(t);
    nngn::Light *x = l.add_light(nngn::Light::Type::POINT);
    constexpr vec3 dir = {1, 2, 3}, pos = {4, 5, 6};
    constexpr vec4 color = {7, 8, 9, 10};
    constexpr vec3 att = {11, 12, 13};
    constexpr float spec = 14, cutoff = 15;
    constexpr vec4 color_spec = {7*10, 8*10, 9*10, spec};
    x->color = color;
    x->pos = pos;
    x->dir = dir;
    x->att = att;
    x->spec = spec;
    x->cutoff = cutoff;
    QVERIFY(x->updated);
    l.update(t);
    QVERIFY(!x->updated);
    QCOMPARE(l.ubo().n_point, 1);
    auto p = l.ubo().point;
    QCOMPARE(p.pos[0], vec4(pos, 0));
    QCOMPARE(p.dir[0], vec4(dir, 0));
    QCOMPARE(p.color_spec[0], color_spec);
    QCOMPARE(p.att_cutoff[0], vec4(att, cutoff));
}

void LightTest::updated() {
    nngn::Timing t;
    nngn::Lighting l;
    nngn::Light *x = l.add_light(nngn::Light::Type::DIR);
    constexpr vec4 color = {1, 2, 3, 4};
    constexpr float spec = 8;
    x->dir = {5, 6, 7};
    x->color = color;
    x->spec = spec;
    l.update(t);
    x->color = {9, 10, 11, 12};
    l.update(t);
    constexpr vec4 color_spec = {1*4, 2*4, 3*4, spec};
    QCOMPARE(l.ubo().n_dir, 1);
    QCOMPARE(l.ubo().dir.color_spec[0], color_spec);
}

void LightTest::entity() {
    nngn::Lighting l;
    Entity v[4];
    for(size_t i = 0; i < 4; ++i) {
        v[i].set_light(l.add_light(nngn::Light::Type::DIR));
        v[i].light->color = vec4(static_cast<float>(i));
    }
    l.remove_light(&l.dir_lights()[1]);
    for(auto i : {0, 2, 3})
        QCOMPARE(v[i].light->color, vec4(static_cast<float>(i)));
    l.remove_light(&l.dir_lights()[2]);
    for(auto i : {0, 3})
        QCOMPARE(v[i].light->color, vec4(static_cast<float>(i)));
    l.remove_light(&l.dir_lights()[0]);
    QCOMPARE(v[3].light->color, vec4(3));
}

QTEST_MAIN(LightTest)

#include "render/render.h"

#include "tests/registry.h"

#include "render_test.h"

NNGN_TEST(RenderTest)

Q_DECLARE_METATYPE(nngn::uvec2)
Q_DECLARE_METATYPE(nngn::vec2)

void RenderTest::uv_coords_data() {
    QTest::addColumn<nngn::uvec2>("scale");
    QTest::addColumn<nngn::uvec2>("coords0");
    QTest::addColumn<nngn::uvec2>("coords1");
    QTest::addColumn<nngn::vec2>("uv0");
    QTest::addColumn<nngn::vec2>("uv1");
    const nngn::uvec2 scale1(1, 1), scale2(2, 2), scale3(3, 3);
    const auto half = 1.0f / 2.0f, third = 1.0f / 3.0f;
    QTest::newRow("whole")
        << scale1 << nngn::uvec2(0, 0) << nngn::uvec2(1, 1)
        << nngn::vec2(0, 1) << nngn::vec2(1, 0);
    QTest::newRow("half")
        << scale2 << nngn::uvec2(0, 0) << nngn::uvec2(2, 2)
        << nngn::vec2(0, 1) << nngn::vec2(1, 0);
    QTest::newRow("half bl")
        << scale2 << nngn::uvec2(0, 0) << nngn::uvec2(1, 1)
        << nngn::vec2(0, 1) << nngn::vec2(half, half);
    QTest::newRow("half br")
        << scale2 << nngn::uvec2(1, 0) << nngn::uvec2(2, 1)
        << nngn::vec2(half, 1) << nngn::vec2(1, half);
    QTest::newRow("half tl")
        << scale2 << nngn::uvec2(0, 1) << nngn::uvec2(1, 2)
        << nngn::vec2(0, half) << nngn::vec2(half, 0);
    QTest::newRow("half tr")
        << scale2 << nngn::uvec2(1, 1) << nngn::uvec2(2, 2)
        << nngn::vec2(half, half) << nngn::vec2(1, 0);
    QTest::newRow("third")
        << scale3 << nngn::uvec2(0, 0) << nngn::uvec2(3, 3)
        << nngn::vec2(0, 1) << nngn::vec2(1, 0);
    QTest::newRow("third bl")
        << scale3 << nngn::uvec2(0, 0) << nngn::uvec2(1, 1)
        << nngn::vec2(0, 1) << nngn::vec2(third, 2*third);
    QTest::newRow("third bc")
        << scale3 << nngn::uvec2(1, 0) << nngn::uvec2(2, 1)
        << nngn::vec2(third, 1) << nngn::vec2(2*third, 2*third);
    QTest::newRow("third br")
        << scale3 << nngn::uvec2(2, 0) << nngn::uvec2(3, 1)
        << nngn::vec2(2*third, 1) << nngn::vec2(1, 2*third);
    QTest::newRow("third cl")
        << scale3 << nngn::uvec2(0, 1) << nngn::uvec2(1, 2)
        << nngn::vec2(0, 2*third) << nngn::vec2(third, third);
    QTest::newRow("third cc")
        << scale3 << nngn::uvec2(1, 1) << nngn::uvec2(2, 2)
        << nngn::vec2(third, 2*third) << nngn::vec2(2*third, third);
    QTest::newRow("third cr")
        << scale3 << nngn::uvec2(2, 1) << nngn::uvec2(3, 2)
        << nngn::vec2(2*third, 2*third) << nngn::vec2(1, third);
    QTest::newRow("third tl")
        << scale3 << nngn::uvec2(0, 2) << nngn::uvec2(1, 3)
        << nngn::vec2(0, third) << nngn::vec2(third, 0);
    QTest::newRow("third tc")
        << scale3 << nngn::uvec2(1, 2) << nngn::uvec2(2, 3)
        << nngn::vec2(third, third) << nngn::vec2(2*third, 0);
    QTest::newRow("third tr")
        << scale3 << nngn::uvec2(2, 2) << nngn::uvec2(3, 3)
        << nngn::vec2(2*third, third) << nngn::vec2(1, 0);
}

void RenderTest::uv_coords() {
    QFETCH(nngn::uvec2, scale);
    QFETCH(nngn::uvec2, coords0);
    QFETCH(nngn::uvec2, coords1);
    QFETCH(nngn::vec2, uv0);
    QFETCH(nngn::vec2, uv1);
    std::array<nngn::uvec2, 2> coords = {coords0, coords1};
    nngn::SpriteRenderer r;
    nngn::SpriteRenderer::uv_coords(coords[0], coords[1], scale, &r.uv0);
    nngn::vec4 v = {r.uv0, r.uv1}, c = {uv0, uv1};
    for(int i = 0; i < 4; ++i)
        if(!qFuzzyCompare(v[i], c[i]))
            QCOMPARE(v, c);
}

QTEST_MAIN(RenderTest)

#include "graphics/shaders.h"

#include "shaders_test.h"

extern const uint8_t *NNGN_GLSL_VK_TRIANGLE_VERT;
extern const size_t NNGN_GLSL_VK_TRIANGLE_VERT_LEN;

void ShadersTest::load() {
    const auto src =
        reinterpret_cast<const char*>(NNGN_GLSL_VK_TRIANGLE_VERT);
    const auto ret = nngn::Shaders().load(
        nngn::Shaders::Name::VK_TRIANGLE_VERT);
    QCOMPARE(ret.size(), NNGN_GLSL_VK_TRIANGLE_VERT_LEN);
    QCOMPARE(ret.data(), src);
}

QTEST_MAIN(ShadersTest)

#include "terminal_test.h"

#include "graphics/terminal/frame_buffer.h"
#include "graphics/terminal/texture.h"

#include "tests/tests.h"

using namespace std::literals;
using namespace nngn::literals;
using nngn::uvec2;
using nngn::term::FrameBuffer, nngn::term::Texture;
using texel4 = Texture::texel4;

Q_DECLARE_METATYPE(texel4);

void TerminalTest::texture_sample(void) {
    constexpr auto n = 2_z, bytes = 4 * n * n;
    const uvec2 size = {n, n};
    auto t = Texture{size};
    QCOMPARE(t.size(), size);
    QCOMPARE(t.data().size(), bytes);
    t.copy([] {
        std::array<unsigned char, bytes> ret = {};
        std::iota(begin(ret), end(ret), static_cast<unsigned char>('a'));
        return ret;
    }().data());
    QCOMPARE((t.sample({0.0f, 0.0f})), (texel4{'a', 'b', 'c', 'd'}));
    QCOMPARE((t.sample({1.0f, 0.0f})), (texel4{'e', 'f', 'g', 'h'}));
    QCOMPARE((t.sample({0.0f, 1.0f})), (texel4{'i', 'j', 'k', 'l'}));
    QCOMPARE((t.sample({1.0f, 1.0f})), (texel4{'m', 'n', 'o', 'p'}));
}

void TerminalTest::empty(void) {
    auto f = FrameBuffer{{}};
    f.resize_and_clear({2, 2});
    const std::string_view s = {begin(f.span()), end(f.span())};
    const auto expected_empty = "    "sv;
    QCOMPARE(s, expected_empty);
}

void TerminalTest::write(void) {
    auto f = FrameBuffer{{}};
    f.resize_and_clear({2, 2});
    const std::string_view s = {begin(f.span()), end(f.span())};
    f.write(0, 0, {'a', 'b', 'c', 'd'});
    f.write(1, 0, {'e', 'f', 'g', 'h'});
    f.write(0, 1, {'i', 'j', 'k', 'l'});
    f.write(1, 1, {'m', 'n', 'o', 'p'});
    const auto expected = "!i~+"sv;
    QCOMPARE(s, expected);
}

QTEST_MAIN(TerminalTest)

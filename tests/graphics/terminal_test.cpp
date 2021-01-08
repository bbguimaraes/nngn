#include "terminal_test.h"

#include "graphics/terminal/frame_buffer.h"
#include "graphics/terminal/texture.h"

#include "tests/tests.h"

using namespace std::literals;
using namespace nngn::literals;
using nngn::uvec2;
using nngn::VT100EscapeCode;
using nngn::term::FrameBuffer, nngn::term::Texture;
using texel4 = Texture::texel4;
using Flag = nngn::Graphics::TerminalFlag;
using Mode = nngn::Graphics::TerminalMode;

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

void TerminalTest::ascii_empty(void) {
    auto f = FrameBuffer{{}, Mode::ASCII};
    f.resize_and_clear({2, 2});
    const std::string_view s = {begin(f.span()), end(f.span())};
    const auto expected_empty = "    "sv;
    QCOMPARE(s, expected_empty);
}

void TerminalTest::ascii_write(void) {
    auto f = FrameBuffer{{}, Mode::ASCII};
    f.resize_and_clear({2, 2});
    const std::string_view s = {begin(f.span()), end(f.span())};
    f.write_ascii(0, 0, {'a', 'b', 'c', 'd'});
    f.write_ascii(1, 0, {'e', 'f', 'g', 'h'});
    f.write_ascii(0, 1, {'i', 'j', 'k', 'l'});
    f.write_ascii(1, 1, {'m', 'n', 'o', 'p'});
    const auto expected = "!i~+"sv;
    QCOMPARE(s, expected);
}

void TerminalTest::colored_empty(void) {
    auto f = FrameBuffer{{}, Mode::COLORED};
    f.resize_and_clear({2, 2});
    const std::string_view s = {begin(f.span()), end(f.span())};
    const auto expected_empty =
        "\x1b[48;2;000;000;000m \x1b[48;2;000;000;000m "
            "\x1b[48;2;000;000;000m \x1b[48;2;000;000;000m "sv;
    QCOMPARE(s, expected_empty);
}

void TerminalTest::colored_write(void) {
    auto f = FrameBuffer{{}, Mode::COLORED};
    f.resize_and_clear({2, 2});
    const std::string_view s = {begin(f.span()), end(f.span())};
    f.write_colored(0, 0, {'a', 'b', 'c', 255});
    f.write_colored(1, 0, {'d', 'e', 'f', 255});
    f.write_colored(0, 1, {'g', 'h', 'i', 255});
    f.write_colored(1, 1, {'j', 'k', 'l', 255});
    constexpr auto expected =
        "\x1b[48;2;097;098;099m \x1b[48;2;100;101;102m "
            "\x1b[48;2;103;104;105m \x1b[48;2;106;107;108m "sv;
    QCOMPARE(s, expected);
}

void TerminalTest::dedup(void) {
    auto f = FrameBuffer{Flag::DEDUPLICATE, Mode::COLORED};
    f.resize_and_clear({2, 2});
    const std::string_view s = {begin(f.span()), end(f.span())};
    f.write_colored(0, 0, {'a', 'b', 'c', 255});
    f.write_colored(1, 0, {'a', 'b', 'c', 255});
    f.write_colored(0, 1, {'d', 'e', 'f', 255});
    f.write_colored(1, 1, {'d', 'e', 'f', 255});
    constexpr auto expected =
        "\x1b[48;2;097;098;099m  \x1b[48;2;100;101;102m  "sv;
    QCOMPARE((s.substr(0, f.dedup())), expected);
}

QTEST_MAIN(TerminalTest)

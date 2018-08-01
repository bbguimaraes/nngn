#include <iomanip>

#include "graphics/pseudo.h"
#include "graphics/texture.h"
#include "utils/log.h"

#include "texture_test.h"

struct TextureTestGraphics : public nngn::Pseudograph {
    bool called = false;
    std::uint32_t i = 0, n = 0;
    const std::byte *p = nullptr;
    std::vector<std::byte> d = {};
    bool load_textures(std::uint32_t, std::uint32_t, const std::byte*) override;
};

bool TextureTestGraphics::load_textures(
    std::uint32_t i_, std::uint32_t n_, const std::byte *p_
) {
    this->called = true;
    this->i = i_;
    this->n = n_;
    this->p = p_;
    this->d = std::vector(p_, p_ + n_ * nngn::Graphics::TEXTURE_SIZE);
    return true;
}

std::filesystem::path TextureTest::data_file, TextureTest::data_alpha_file;
std::byte TextureTest::data[nngn::Graphics::TEXTURE_SIZE];
std::byte TextureTest::data_alpha[nngn::Graphics::TEXTURE_SIZE];

template<typename T, typename U> auto img_diff(T &&t, U &&u) {
    const auto b = std::begin(t);
    auto tp = b;
    auto up = std::begin(u);
    if(std::memcmp(&*b, &*up, nngn::Graphics::TEXTURE_SIZE) == 0)
        return nngn::Graphics::TEXTURE_SIZE;
    for(const auto e = std::end(t); tp != e && *tp++ == *up++;);
    return static_cast<std::uint32_t>(tp - b - 1);
}

template<typename T> QString fmt_img(T &&t, size_t off) {
    constexpr std::ptrdiff_t max = 64;
    const auto b = std::begin(t) + static_cast<std::ptrdiff_t>(off);
    const auto e = std::end(t);
    const auto n = e - b;
    const bool extra = n > max;
    std::stringstream s;
    s << "offset " << off << ":";
    for(auto it = b, ie = it + std::min(n, max); it != ie; ++it)
        s << ' ' << static_cast<unsigned int>(*it);
    if(extra)
        s << "...";
    return QString::fromStdString(s.str());
}

void TextureTest::initTestCase() {
    this->data_file = "texture_test.png";
    this->data_alpha_file = "texture_test_alpha.png";
    if(const char *d = std::getenv("srcdir")) {
        const std::filesystem::path p = d;
        this->data_file =
            p / "tests" / "graphics" / this->data_file;
        this->data_alpha_file =
            p / "tests" / "graphics" / this->data_alpha_file;
    }
    std::uint8_t i = 0, v = 0;
    for(auto &x : this->data)
        x = static_cast<std::byte>(++v % 4 ? i++ : 255);
    i = 0;
    for(auto &x : this->data_alpha)
        x = static_cast<std::byte>(i++);
}

void TextureTest::read() {
    nngn::Textures t;
    t.set_max(1);
    const auto v = t.read(this->data_file.c_str());
    QVERIFY(!v.empty());
    const auto off = img_diff(v, this->data);
    if(off != nngn::Graphics::TEXTURE_SIZE)
        QCOMPARE(fmt_img(v, off), fmt_img(this->data, off));
}

void TextureTest::load_data_test() {
    TextureTestGraphics g;
    nngn::Textures t;
    t.set_max(2);
    t.set_graphics(&g);
    QVERIFY(t.load_data(this->data_file.c_str(), this->data));
    QCOMPARE(t.n(), 2);
    QVERIFY(g.called);
    QCOMPARE(g.i, 1);
    QCOMPARE(g.n, 1);
    QCOMPARE(g.p, this->data);
}

void TextureTest::load() {
    TextureTestGraphics g;
    nngn::Textures t;
    t.set_max(2);
    t.set_graphics(&g);
    QVERIFY(t.load(this->data_file.c_str()));
    QCOMPARE(t.n(), 2);
    QVERIFY(g.called);
    QCOMPARE(g.i, 1);
    QCOMPARE(g.n, 1);
    QVERIFY(!g.d.empty());
    const auto off = img_diff(g.d, this->data);
    if(off != nngn::Graphics::TEXTURE_SIZE)
        QCOMPARE(fmt_img(g.d, off), fmt_img(this->data, off));
}

void TextureTest::load_alpha() {
    TextureTestGraphics g;
    nngn::Textures t;
    t.set_max(2);
    t.set_graphics(&g);
    QVERIFY(t.load(this->data_alpha_file.c_str()));
    QCOMPARE(t.n(), 2);
    QVERIFY(g.called);
    QCOMPARE(g.i, 1);
    QCOMPARE(g.n, 1);
    QVERIFY(!g.d.empty());
    const auto off = img_diff(g.d, this->data_alpha);
    if(off != nngn::Graphics::TEXTURE_SIZE)
        QCOMPARE(fmt_img(g.d, off), fmt_img(this->data, off));
}

void TextureTest::load_err() {
    auto s = nngn::Log::capture([]() {
        TextureTestGraphics g;
        nngn::Textures t;
        t.set_max(2);
        t.set_graphics(&g);
        QCOMPARE(t.load("/dev/null"), 0u);
    });
    constexpr auto cmp =
        "Textures::load: /dev/null: Textures::read: /dev/null: Read Error\n";
    QCOMPARE(s.c_str(), cmp);
}

void TextureTest::load_max() {
    constexpr std::size_t n = 16;
    TextureTestGraphics g;
    nngn::Textures t;
    t.set_max(n);
    t.set_graphics(&g);
    for(std::size_t i = 1; i < n; ++i)
        QVERIFY(t.load_data(std::to_string(i).c_str(), this->data));
    const auto m = std::to_string(n);
    auto s = nngn::Log::capture([&t, &m]()
        { QVERIFY(!t.load(m.c_str())); });
    const auto c =
        "Textures::load: " + m + ": "
        "cannot load more textures (max = " + m + ")\n";
    QCOMPARE(s.c_str(), c.c_str());
}

QTEST_MAIN(TextureTest)

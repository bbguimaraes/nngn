#include "font_test.h"

#include <cstdio>
#include <sstream>

#include "font/font.h"
#include "utils/log.h"

namespace {

auto find_font(void) {
    std::array<char, PATH_MAX + 1> ret = {};
    FILE *const p = popen("fc-match --format %{file} DejaVuSans", "r");
    if(!p)
        return nngn::Log::perror("popen"), ret;
    const std::size_t n_read = fread(ret.data(), 1, ret.size(), p);
    if(ferror(p))
        return nngn::Log::perror("fread"), ret;
    if(fclose(p) == EOF)
        return nngn::Log::perror("fclose"), ret;
    assert(n_read <= ret.size());
    ret[n_read] = 0;
    return ret;
}

}

void FontTest::from_file() {
    const auto path = find_font();
    QVERIFY(path[0]);
    nngn::Fonts fs;
    QVERIFY(fs.init());
    QVERIFY(fs.load(64, path.data()));
    const auto *f = fs.fonts() + 1;
    QCOMPARE(f->chars.size(), 128);
    const auto &c = f->chars['f'];
    QCOMPARE(c.size, nngn::uvec2(23, 49));
    QCOMPARE(c.bearing, nngn::ivec2(1, 0));
    QCOMPARE(c.advance, 23.0f);
}

void FontTest::from_file_err() {
    nngn::Fonts fs;
    QVERIFY(fs.init());
    auto s = nngn::Log::capture([&fs]()
        { QVERIFY(!fs.load(0, "/dev/null")); });
    QCOMPARE(
        s.c_str(),
        "Fonts::load: FT_New_Face(/dev/null): unknown file format\n");
}

QTEST_MAIN(FontTest)

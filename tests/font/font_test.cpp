#include <sstream>

#include "font/font.h"
#include "utils/log.h"

#include "font_test.h"

static const char *FONT = "/usr/share/fonts/TTF/DejaVuSans.ttf";

void FontTest::from_file() {
    nngn::Fonts fs;
    QVERIFY(fs.init());
    QVERIFY(fs.load(64, FONT));
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

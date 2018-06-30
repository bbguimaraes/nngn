#include "font/font.h"
#include "font/text.h"

#include "text_test.h"

Q_DECLARE_METATYPE(nngn::vec2)

static nngn::Font gen_font() {
    nngn::Font font;
    font.size = 32;
    font.chars['t'].advance = 2.0f;
    font.chars['e'].advance = 3.0f;
    font.chars['s'].advance = 5.0f;
    return font;
}

void TextTest::default_constructor() {
    nngn::Text text;
    QCOMPARE(text.str.c_str(), "");
    QCOMPARE(text.cur, size_t());
    QCOMPARE(text.nlines, 0u);
    QCOMPARE(text.spacing, 0.0f);
    QCOMPARE(text.size, nngn::vec2(0.0f, 0.0f));
}

void TextTest::constructor() {
    nngn::Font font = gen_font();
    nngn::Text text(font, "test");
    QCOMPARE(text.str.c_str(), "test");
    QCOMPARE(text.cur, 4ul);
    QCOMPARE(text.nlines, 1u);
    QCOMPARE(text.spacing, 8.0f);
    QCOMPARE(text.size, nngn::vec2(12.0f, 32.0f));
}

void TextTest::count_lines_data() {
    QTest::addColumn<unsigned int>("n");
    QTest::addColumn<QString>("str");
    QTest::newRow("0") << 1u << "";
    QTest::newRow("1") << 1u << "test";
    QTest::newRow("2") << 2u << "test\ntest";
    QTest::newRow("4") << 4u << "test\ntest\ntest\ntest";
}

void TextTest::count_lines() {
    std::string sstr;
    QFETCH(unsigned int, n);
    { QFETCH(QString, str); sstr = str.toStdString(); }
    QCOMPARE(nngn::Text::count_lines(sstr, sstr.size()), n);
}

void TextTest::size_zero() {
    nngn::Font font;
    font.size = 32;
    QCOMPARE(nngn::Text(font, "", 0).size, nngn::vec2(0.0f, 32.0f));
}

void TextTest::size_data() {
    QTest::addColumn<nngn::vec2>("size");
    QTest::addColumn<unsigned int>("cur");
    QTest::newRow( "0, 1") << nngn::vec2( 0.0f,  32.0f) <<  0u;
    QTest::newRow( "1, 1") << nngn::vec2( 2.0f,  32.0f) <<  1u;
    QTest::newRow( "2, 1") << nngn::vec2( 5.0f,  32.0f) <<  2u;
    QTest::newRow( "3, 1") << nngn::vec2(10.0f,  32.0f) <<  3u;
    QTest::newRow( "4, 1") << nngn::vec2(12.0f,  32.0f) <<  4u;
    QTest::newRow( "5, 2") << nngn::vec2(12.0f,  72.0f) <<  5u;
    QTest::newRow( "9, 2") << nngn::vec2(12.0f,  72.0f) <<  9u;
    QTest::newRow("10, 2") << nngn::vec2(14.0f,  72.0f) << 10u;
    QTest::newRow("13, 2") << nngn::vec2(24.0f,  72.0f) << 13u;
    QTest::newRow("14, 3") << nngn::vec2(24.0f, 112.0f) << 14u;
    QTest::newRow("26, 3") << nngn::vec2(36.0f, 112.0f) << 26u;
}

void TextTest::size() {
    const char *str = "test\ntesttest\ntesttesttest";
    nngn::Font font = gen_font();
    QFETCH(nngn::vec2, size);
    QFETCH(unsigned int, cur);
    nngn::Text text(font, str, cur);
    QCOMPARE(text.size, size);
}

QTEST_MAIN(TextTest)

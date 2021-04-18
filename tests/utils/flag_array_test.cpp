#include "flag_array_test.h"

#include <sstream>

#include "utils/flag_array.h"

namespace {

void to_str(std::stringstream *s, const nngn::FlagArray &f) {
    const auto n = f.size();
    for(std::size_t i = 0; i < n; ++i)
        *s << f[i];
}

auto to_str(const nngn::FlagArray &f) {
    std::stringstream s;
    to_str(&s, f);
    return s.str();
}

}

void FlagArrayTest::constructor_data() {
    QTest::addColumn<std::size_t>("n");
    QTest::addColumn<std::size_t>("size");
    QTest::addColumn<std::size_t>("size_bytes");
    QTest::addColumn<std::size_t>("total_size_bytes");
    QTest::newRow("0")
        << std::size_t{} << std::size_t{} << std::size_t{} << std::size_t{};
    QTest::newRow("32")
        << std::size_t{32}
        << std::size_t{32} << std::size_t{4} << std::size_t{8};
    QTest::newRow("42")
        << std::size_t{42}
        << std::size_t{64} << std::size_t{8} << std::size_t{16};
}

void FlagArrayTest::constructor() {
    QFETCH(const std::size_t, n);
    QFETCH(const std::size_t, size);
    QFETCH(const std::size_t, size_bytes);
    QFETCH(const std::size_t, total_size_bytes);
    const nngn::FlagArray f(n);
    QCOMPARE(f.total_size_bytes(), total_size_bytes);
    QCOMPARE(f.size_bytes(), size_bytes);
    QCOMPARE(f.size(), size);
}

void FlagArrayTest::set() {
    nngn::FlagArray f(8);
    QCOMPARE(to_str(f).c_str(), "00000000");
    f.set(3);
    f.set(5);
    QCOMPARE(to_str(f).c_str(), "00010100");
    QVERIFY(f[3]);
    QVERIFY(f[5]);
    f.clear();
    QCOMPARE(to_str(f).c_str(), "00000000");
}

void FlagArrayTest::layers() {
    constexpr auto to_str = [](const nngn::FlagArray &f) {
        std::stringstream s;
        ::to_str(&s, f);
        const auto n_layers = f.n_layers();
        auto n = f.size();
        for(std::size_t li = 0; li < n_layers; ++li) {
            n >>= 1;
            s << ' ';
            for(std::size_t i = 0; i < n; ++i)
                s << f.get(li, i);
        }
        return s.str();
    };
    nngn::FlagArray f(8);
    QCOMPARE(f.n_layers(), 3);
    f.set(3);
    QCOMPARE(to_str(f).c_str(), "00010000 0100 10 1");
    f.set(5);
    QCOMPARE(to_str(f).c_str(), "00010100 0110 11 1");
    f.clear();
    QCOMPARE(to_str(f).c_str(), "00000000 0000 00 0");
}

QTEST_MAIN(FlagArrayTest)

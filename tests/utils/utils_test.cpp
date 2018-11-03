#include <cstring>
#include <filesystem>

#include "utils/log.h"
#include "utils/pointer_flag.h"
#include "utils/utils.h"

#include "tests/tests.h"

#include "utils_test.h"

using namespace std::string_literals;

void UtilsTest::read_file() {
    std::filesystem::path f = "utils_test.txt";
    if(const char *d = std::getenv("srcdir"))
        f = d / ("tests" / ("utils" / f));
    std::string ret;
    if(!nngn::read_file(f.c_str(), &ret))
        QFAIL(std::strerror(errno));
    QCOMPARE(ret.c_str(), "test\n");
}

void UtilsTest::read_file_err() {
    const auto ret = nngn::Log::capture([]() {
        QVERIFY(!nngn::read_file("\0", static_cast<std::string*>(nullptr)));
    });
    const auto expected = "read_file: : "s + std::strerror(ENOENT) + '\n';
    QCOMPARE(ret, expected);
}

void UtilsTest::pointer_flag() {
    auto p = this;
    auto fp = nngn::pointer_flag{p};
    QCOMPARE(fp.get(), p);
    QCOMPARE(fp.flag(), false);
    fp.set_flag(true);
    QCOMPARE(fp.get(), p);
    QCOMPARE(fp.flag(), true);
    fp.set_flag(false);
    QCOMPARE(fp.get(), p);
    QCOMPARE(fp.flag(), false);
}

QTEST_MAIN(UtilsTest)

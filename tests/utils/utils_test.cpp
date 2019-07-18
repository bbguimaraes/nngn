#include <cstring>
#include <filesystem>

#include "utils/literals.h"
#include "utils/log.h"
#include "utils/pointer_flag.h"
#include "utils/ranges.h"
#include "utils/scoped.h"
#include "utils/utils.h"

#include "tests/tests.h"

#include "utils_test.h"

using namespace std::string_literals;
using namespace nngn::literals;

using nngn::u8, nngn::i32, nngn::u64;

void UtilsTest::offsetof_ptr() {
    struct S { i32 i; u64 u; u8 b; };
    QCOMPARE(nngn::offsetof_ptr(&S::i), offsetof(S, i));
    QCOMPARE(nngn::offsetof_ptr(&S::u), offsetof(S, u));
    QCOMPARE(nngn::offsetof_ptr(&S::b), offsetof(S, b));
}

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

void UtilsTest::set_capacity() {
    std::vector<int> v;
    nngn::set_capacity(&v, 8);
    QCOMPARE(v.capacity(), 8);
    nngn::set_capacity(&v, 4);
    QCOMPARE(v.capacity(), 4);
    nngn::set_capacity(&v, 8);
    QCOMPARE(v.capacity(), 8);
}

void UtilsTest::const_time_erase() {
    std::vector<int> v = {0, 1, 2, 3, 4};
    auto *const p = &v[2];
    nngn::const_time_erase(&v, p);
    QCOMPARE(v[0], 0);
    QCOMPARE(v[1], 1);
    QCOMPARE(v[2], 4);
    QCOMPARE(v[3], 3);
    QCOMPARE(v.size(), 4_z);
    QCOMPARE(p, v.data() + 2);
}

QTEST_MAIN(UtilsTest)

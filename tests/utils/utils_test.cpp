#include <cstring>
#include <filesystem>

#include "utils/log.h"
#include "utils/pointer_flag.h"
#include "utils/scoped.h"
#include "utils/utils.h"
#include "utils/vector.h"

#include "tests/tests.h"

#include "utils_test.h"

void UtilsTest::offsetof_ptr() {
    struct S { std::int32_t i32; std::int64_t i64; std::uint8_t u8; };
    QCOMPARE(nngn::offsetof_ptr(&S::i32), offsetof(S, i32));
    QCOMPARE(nngn::offsetof_ptr(&S::i64), offsetof(S, i64));
    QCOMPARE(nngn::offsetof_ptr(&S::u8), offsetof(S, u8));
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
    const auto ret = nngn::Log::capture(
        []() { QCOMPARE(nngn::read_file("\0", nullptr), false); });
    const auto expected =
        std::string("read_file: : ") + std::strerror(ENOENT) + '\n';
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

void UtilsTest::vector_linear_erase() {
    std::vector<int> v = {0, 1, 2, 3, 4};
    const auto it = nngn::vector_linear_erase(&v, &v[2]);
    QCOMPARE(v[0], 0);
    QCOMPARE(v[1], 1);
    QCOMPARE(v[2], 4);
    QCOMPARE(v[3], 3);
    QCOMPARE(v.size(), 4ul);
    QCOMPARE(it, v.begin() + 2);
}

QTEST_MAIN(UtilsTest)

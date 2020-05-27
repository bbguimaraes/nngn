#include "utils/pointer_flag.h"

#include "utils_test.h"

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

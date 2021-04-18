#include "flag_array.h"

#include "utils/flag_array.h"

void FlagArrayTest::set() {
    constexpr std::size_t n = 1'000'000;
    nngn::FlagArray a = {n};
    QBENCHMARK {
        for(std::size_t i = 0; i < n; ++i)
            a.set(i);
    }
}

QTEST_MAIN(FlagArrayTest)

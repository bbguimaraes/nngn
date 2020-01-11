#include "math/mat4.h"

#include "tests/tests.h"

#include "mat4_test.h"

void Mat4Test::mat4_mat4_mul() {
    constexpr nngn::mat4 m0 = {
         1,  2,  3,  4,
         5,  6,  7,  8,
         9, 10, 11, 12,
        13, 14, 15, 16};
    constexpr nngn::mat4 m1 = {
        17, 18, 19, 20,
        21, 22, 23, 24,
        25, 26, 27, 28,
        29, 30, 31, 31};
    constexpr nngn::mat4 expected = {
        538, 612,  686,  760,
        650, 740,  830,  920,
        762, 868,  974, 1080,
        861, 982, 1103, 1224};
    QCOMPARE(m0 * m1, expected);
}

void Mat4Test::mat4_vec4_mul() {
    constexpr nngn::vec4 v = {1, 2, 3, 4};
    constexpr nngn::mat4 m = {
         5,  6,  7,  8,
         9, 10, 11, 12,
        13, 14, 15, 16,
        17, 18, 19, 20};
    constexpr nngn::vec4 expected = {70, 110, 150, 190};
    QCOMPARE(m * v, expected);
}

QTEST_MAIN(Mat4Test)

#include "mat_test.h"

#include "math/mat3.h"
#include "math/mat4.h"
#include "math/math.h"

#include "tests/tests.h"

using nngn::Math;
using nngn::vec3;
using nngn::vec4;
using nngn::mat3;
using nngn::mat4;

Q_DECLARE_METATYPE(vec3)
Q_DECLARE_METATYPE(vec4)
Q_DECLARE_METATYPE(mat3)
Q_DECLARE_METATYPE(mat4)

void MatTest::mat3_row(void) {
    const auto m = Math::transpose(mat3{
        0, 1, 2,
        3, 4, 5,
        6, 7, 8,
    });
    QCOMPARE(m.row(0), (vec3{0, 1, 2}));
    QCOMPARE(m.row(1), (vec3{3, 4, 5}));
    QCOMPARE(m.row(2), (vec3{6, 7, 8}));
}

void MatTest::mat3_col(void) {
    const auto m = Math::transpose(mat3{
        0, 1, 2,
        3, 4, 5,
        6, 7, 8,
    });
    QCOMPARE(m.col(0), (vec3{0, 3, 6}));
    QCOMPARE(m.col(1), (vec3{1, 4, 7}));
    QCOMPARE(m.col(2), (vec3{2, 5, 8}));
}

void MatTest::mat4_row(void) {
    const auto m = Math::transpose(mat4{
         0,  1,  2,  3,
         4,  5,  6,  7,
         8,  9, 10, 11,
        12, 13, 14, 15,
    });
    QCOMPARE(m.row(0), (vec4{ 0,  1,  2,  3}));
    QCOMPARE(m.row(1), (vec4{ 4,  5,  6,  7}));
    QCOMPARE(m.row(2), (vec4{ 8,  9, 10, 11}));
    QCOMPARE(m.row(3), (vec4{12, 13, 14, 15}));
}

void MatTest::diag(void) {
    const auto m = Math::transpose(mat3{
         0, 1, 2,
         3, 4, 5,
         6, 7, 8,
    });
    QCOMPARE(Math::diag(m, 0), (vec3{0, 4, 8}));
    QCOMPARE(Math::diag(m, 1), (vec3{1, 5, 6}));
    QCOMPARE(Math::diag(m, 2), (vec3{2, 3, 7}));
    QCOMPARE(Math::inv_diag(m, 0), (vec3{0, 5, 7}));
    QCOMPARE(Math::inv_diag(m, 1), (vec3{1, 3, 8}));
    QCOMPARE(Math::inv_diag(m, 2), (vec3{2, 4, 6}));
}

void MatTest::mat4_col(void) {
    const auto m = Math::transpose(mat4{
         0,  1,  2,  3,
         4,  5,  6,  7,
         8,  9, 10, 11,
        12, 13, 14, 15,
    });
    QCOMPARE(m.col(0), (vec4{0, 4,  8, 12}));
    QCOMPARE(m.col(1), (vec4{1, 5,  9, 13}));
    QCOMPARE(m.col(2), (vec4{2, 6, 10, 14}));
    QCOMPARE(m.col(3), (vec4{3, 7, 11, 15}));
}

void MatTest::minor_matrix_data(void) {
    QTest::addColumn<std::size_t>("i");
    QTest::addColumn<std::size_t>("j");
    QTest::addColumn<mat3>("minor");
    QTest::newRow("0, 0") << 0_z << 0_z << Math::transpose(mat3{
        -1,  2,  5,
         2,  3,  4,
         4, -1,  2,
    });
    QTest::newRow("1, 0") << 1_z << 0_z << Math::transpose(mat3{
         2,  2,  5,
         1,  3,  4,
         3, -1,  2,
    });
    QTest::newRow("2, 0") << 2_z << 0_z << Math::transpose(mat3{
         2, -1,  5,
         1,  2,  4,
         3,  4,  2,
    });
    QTest::newRow("3, 0") << 3_z << 0_z << Math::transpose(mat3{
         2, -1,  2,
         1,  2,  3,
         3,  4, -1,
    });
    QTest::newRow("0, 1") << 0_z << 1_z << Math::transpose(mat3{
         1,  4,  2,
         2,  3,  4,
         4, -1,  2,
    });
    QTest::newRow("1, 1") << 1_z << 1_z << Math::transpose(mat3{
        -1,  4,  2,
         1,  3,  4,
         3, -1,  2,
    });
    QTest::newRow("2, 1") << 2_z << 1_z << Math::transpose(mat3{
        -1,  1,  2,
         1,  2,  4,
         3,  4,  2,
    });
    QTest::newRow("3, 1") << 3_z << 1_z << Math::transpose(mat3{
        -1,  1,  4,
         1,  2,  3,
         3,  4, -1,
    });
    QTest::newRow("0, 2") << 0_z << 2_z << Math::transpose(mat3{
         1,  4,  2,
        -1,  2,  5,
         4, -1,  2,
    });
    QTest::newRow("1, 2") << 1_z << 2_z << Math::transpose(mat3{
        -1,  4,  2,
         2,  2,  5,
         3, -1,  2,
    });
    QTest::newRow("2, 2") << 2_z << 2_z << Math::transpose(mat3{
        -1,  1,  2,
         2, -1,  5,
         3,  4,  2,
    });
    QTest::newRow("3, 2") << 3_z << 2_z << Math::transpose(mat3{
        -1,  1,  4,
         2, -1,  2,
         3,  4, -1,
    });
    QTest::newRow("0, 3") << 0_z << 3_z << Math::transpose(mat3{
         1,  4,  2,
        -1,  2,  5,
         2,  3,  4,
    });
    QTest::newRow("1, 3") << 1_z << 3_z << Math::transpose(mat3{
        -1,  4,  2,
         2,  2,  5,
         1,  3,  4,
    });
    QTest::newRow("2, 3") << 2_z << 3_z << Math::transpose(mat3{
        -1,  1,  2,
         2, -1,  5,
         1,  2,  4,
    });
    QTest::newRow("3, 3") << 3_z << 3_z << Math::transpose(mat3{
        -1,  1,  4,
         2, -1,  2,
         1,  2,  3,
    });
}

void MatTest::minor_matrix(void) {
    QFETCH(const std::size_t, i);
    QFETCH(const std::size_t, j);
    QFETCH(const mat3, minor);
    const auto m = Math::transpose(mat4{
        -1,  1,  4,  2,
         2, -1,  2,  5,
         1,  2,  3,  4,
         3,  4, -1,  2,
    });
    QCOMPARE(Math::minor_matrix(m, i, j), minor);
}

void MatTest::mat3_vec_mul(void) {
    const auto m = Math::transpose(mat3{
        0, 1, 2,
        3, 4, 5,
        6, 7, 8,
    });
    constexpr vec3 v = {9, 10, 11};
    QCOMPARE(m * v, (vec3{32, 122, 212}));
}

void MatTest::mat4_vec_mul(void) {
    const auto m = Math::transpose(mat4{
         0,  1,  2,  3,
         4,  5,  6,  7,
         8,  9, 10, 11,
        12, 13, 14, 15,
    });
    constexpr vec4 v = {16, 17, 18, 19};
    QCOMPARE(m * v, (vec4{110, 390, 670, 950}));
}

void MatTest::mat3_mul(void) {
    const auto m0 = Math::transpose(mat3{
        1, 2, 3,
        4, 5, 6,
        7, 8, 9,
    });
    const auto m1 = Math::transpose(mat3{
        10, 11, 12,
        13, 14, 15,
        16, 17, 18,
    });
    const auto m = Math::transpose(mat3{
         84,  90,  96,
        201, 216, 231,
        318, 342, 366,
    });
    QCOMPARE(m0 * m1, m);
}

void MatTest::mat4_mul(void) {
    const auto m0 = Math::transpose(mat4{
         1,  2,  3,  4,
         5,  6,  7,  8,
         9, 10, 11, 12,
        13, 14, 15, 16
    });
    const auto m1 = Math::transpose(mat4{
        17, 18, 19, 20,
        21, 22, 23, 24,
        25, 26, 27, 28,
        29, 30, 31, 32,
    });
    const auto m = Math::transpose(mat4{
         250,  260,  270,  280,
         618,  644,  670,  696,
         986, 1028, 1070, 1112,
        1354, 1412, 1470, 1528,
    });
    QCOMPARE(m0 * m1, m);
}

void MatTest::rotate_data(void) {
    QTest::addColumn<float>("angle");
    QTest::addColumn<vec3>("axis");
    QTest::addColumn<vec4>("expected");
    QTest::newRow("0")    <<   0.0f << vec3{1, 0, 0} << vec4{ 1,  0,  0, 0};
    QTest::newRow("x90")  <<  90.0f << vec3{1, 0, 0} << vec4{ 1,  0,  0, 0};
    QTest::newRow("x180") << 180.0f << vec3{1, 0, 0} << vec4{ 1,  0,  0, 0};
    QTest::newRow("x270") << 270.0f << vec3{1, 0, 0} << vec4{ 1,  0,  0, 0};
    QTest::newRow("y90")  <<  90.0f << vec3{0, 1, 0} << vec4{ 0,  0, -1, 0};
    QTest::newRow("y180") << 180.0f << vec3{0, 1, 0} << vec4{-1,  0,  0, 0};
    QTest::newRow("y270") << 270.0f << vec3{0, 1, 0} << vec4{ 0,  0,  1, 0};
    QTest::newRow("z90")  <<  90.0f << vec3{0, 0, 1} << vec4{ 0,  1,  0, 0};
    QTest::newRow("z180") << 180.0f << vec3{0, 0, 1} << vec4{-1,  0,  0, 0};
    QTest::newRow("z270") << 270.0f << vec3{0, 0, 1} << vec4{ 0, -1,  0, 0};
}

void MatTest::rotate(void) {
    QFETCH(const float, angle);
    QFETCH(const vec3, axis);
    QFETCH(const vec4, expected);
    constexpr vec4 v = {1, 0, 0, 0};
    const auto m = Math::rotate(mat4{1}, Math::radians(angle), axis);
    if(const auto ret = m * v; !fuzzy_eq(ret, expected))
        QCOMPARE(ret, expected);
}

void MatTest::determinant3_data(void) {
    QTest::addColumn<mat3>("m");
    QTest::addColumn<float>("d");
    QTest::newRow("zero") << mat3{} << 0.0f;
    QTest::newRow("identity") << mat3{1} << 1.0f;
    QTest::newRow("m") << Math::transpose(mat3{
         -2, -1,  2,
          2,  1,  4,
         -3,  3, -1,
    }) << 54.0f;
}

void MatTest::determinant3(void) {
    QFETCH(const mat3, m);
    QFETCH(const float, d);
    QCOMPARE(Math::determinant(m), d);
}

void MatTest::determinant4_data(void) {
    QTest::addColumn<mat4>("m");
    QTest::addColumn<float>("d");
    QTest::newRow("zero") << mat4{} << 0.0f;
    QTest::newRow("identity") << mat4{1} << 1.0f;
    QTest::newRow("m") << Math::transpose(mat4{
        -1,  1,  4,  2,
         2, -1,  2,  5,
         1,  2,  3,  4,
         3,  4, -1,  2,
    }) << -26.0f;
}

void MatTest::determinant4(void) {
    QFETCH(const mat4, m);
    QFETCH(const float, d);
    QCOMPARE(Math::determinant(m), d);
}

void MatTest::inverse_data(void) {
    QTest::addColumn<mat4>("m");
    QTest::addColumn<mat4>("inv");
    QTest::addColumn<float>("det");
    QTest::newRow("identity") << mat4{1} << mat4{1} << 1.0f;
    QTest::newRow("m")
        << Math::transpose(mat4{
            -1,  1,  4,  2,
             2, -1,  2,  5,
             1,  2,  3,  4,
             3,  4, -1,  2,
        })
        << Math::transpose(mat4{
             56.0f/26.0f,  30.0f/26.0f, -83.0f/26.0f,  35.0f/26.0f,
            -10.0f/26.0f, -10.0f/26.0f,  19.0f/26.0f,  -3.0f/26.0f,
             44.0f/26.0f,  18.0f/26.0f, -55.0f/26.0f,  21.0f/26.0f,
            -42.0f/26.0f, -16.0f/26.0f,  59.0f/26.0f, -23.0f/26.0f,
        })
        << -26.0f;
}

void MatTest::inverse(void) {
    QFETCH(const mat4, m);
    QFETCH(const mat4, inv);
    QFETCH(const float, det);
    QCOMPARE(Math::determinant(m), det);
    if(!fuzzy_eq(Math::inverse(m), inv))
        QCOMPARE(Math::inverse(m), inv);
    const auto adj = Math::adjugate(m);
    if(!fuzzy_eq(det * inv, adj))
        QCOMPARE(det * inv, adj);
    if(!fuzzy_eq(Math::inverse(inv), m, 1_u32 << 9))
        QCOMPARE(Math::inverse(inv), m);
    if(!fuzzy_eq(inv * m, mat4{1}))
        QCOMPARE(inv * m, mat4{1});
    if(!fuzzy_eq(m * inv, mat4{1}))
        QCOMPARE(m * inv, mat4{1});
}

QTEST_MAIN(MatTest)

#include "math/math.h"

#include "tests/tests.h"

#include "math_test.h"

using nngn::Math;
using nngn::vec2;
using nngn::vec3;
using nngn::vec4;
using nngn::mat4;

Q_DECLARE_METATYPE(vec2)
Q_DECLARE_METATYPE(vec3)
Q_DECLARE_METATYPE(vec4)

namespace {

template<typename T, typename T::type T::*ordinate>
auto rotate_initial() {
    T ret = {};
    ret.*ordinate = 1;
    return ret;
}

template<
    typename T,
    typename T::type T::*abscissa,
    typename T::type T::*ordinate
> auto rotate_gen_vector(const vec2 &r) {
    T ret = {};
    ret.*abscissa = r[0];
    ret.*ordinate = r[1];
    return ret;
}

template<
    typename T,
    T F(const T&, typename T::type),
    typename T::type T::*abscissa,
    typename T::type T::*ordinate
> void rotate_test() {
    QFETCH(const float, a);
    QFETCH(const vec2, r);
    const auto ret = F(rotate_initial<T, ordinate>(), a);
    const auto expected = rotate_gen_vector<T, abscissa, ordinate>(r);
    if(!fuzzy_eq(ret, expected))
        QCOMPARE(ret, expected);
}

template<
    typename T,
    int up_x,
    int up_y,
    int up_z,
    typename T::type T::*abscissa,
    typename T::type T::*ordinate
> void rotate_vec_test() {
    QFETCH(const float, a);
    QFETCH(const vec2, r);
    constexpr vec3 up = {up_x, up_y, up_z};
    T v = {};
    v.*ordinate = 1;
    T r2 = {};
    r2.*abscissa = r[0];
    r2.*ordinate = r[1];
    if(const auto ret = Math::rotate(v, a, up); !fuzzy_eq(ret, r2))
        QCOMPARE(ret, r2);
}

}

void MathTest::avg2_data() {
    QTest::addColumn<vec2>("v");
    QTest::addColumn<float>("avg");
    QTest::newRow("zero") << vec2{} << 0.0f;
    QTest::newRow("x") << vec2{1, 0} << 0.5f;
    QTest::newRow("y") << vec2{0, 1} << 0.5f;
    QTest::newRow("xy") << vec2{1, 1} << 1.0f;
}

void MathTest::avg2() {
    QFETCH(const vec2, v);
    QFETCH(const float, avg);
    QCOMPARE(Math::avg(v), avg);
}

void MathTest::avg3_data() {
    QTest::addColumn<vec3>("v");
    QTest::addColumn<float>("avg");
    QTest::newRow("zero") << vec3{} << 0.0f;
    QTest::newRow("x") << vec3{3, 0, 0} << 1.0f;
    QTest::newRow("y") << vec3{0, 3, 0} << 1.0f;
    QTest::newRow("z") << vec3{0, 0, 3} << 1.0f;
    QTest::newRow("xy") << vec3{3, 3, 0} << 2.0f;
    QTest::newRow("xz") << vec3{3, 0, 3} << 2.0f;
    QTest::newRow("yz") << vec3{0, 3, 3} << 2.0f;
    QTest::newRow("xyz") << vec3{3, 3, 3} << 3.0f;
}

void MathTest::avg3() {
    QFETCH(const vec3, v);
    QFETCH(const float, avg);
    QCOMPARE(Math::avg(v), avg);
}

void MathTest::avg4_data() {
    QTest::addColumn<vec4>("v");
    QTest::addColumn<float>("avg");
    QTest::newRow("zero") << vec4{} << 0.0f;
    QTest::newRow("x") << vec4{4, 0, 0, 0} << 1.0f;
    QTest::newRow("y") << vec4{0, 4, 0, 0} << 1.0f;
    QTest::newRow("z") << vec4{0, 0, 4, 0} << 1.0f;
    QTest::newRow("w") << vec4{0, 0, 0, 4} << 1.0f;
    QTest::newRow("xy") << vec4{4, 4, 0, 0} << 2.0f;
    QTest::newRow("xz") << vec4{4, 0, 4, 0} << 2.0f;
    QTest::newRow("xw") << vec4{4, 0, 0, 4} << 2.0f;
    QTest::newRow("yz") << vec4{0, 4, 4, 0} << 2.0f;
    QTest::newRow("yw") << vec4{0, 4, 0, 4} << 2.0f;
    QTest::newRow("zw") << vec4{0, 0, 4, 4} << 2.0f;
    QTest::newRow("xyz") << vec4{4, 4, 4, 0} << 3.0f;
    QTest::newRow("yzw") << vec4{0, 4, 4, 4} << 3.0f;
    QTest::newRow("xyzw") << vec4{4, 4, 4, 4} << 4.0f;
}

void MathTest::avg4() {
    QFETCH(const vec4, v);
    QFETCH(const float, avg);
    QCOMPARE(Math::avg(v), avg);
}

void MathTest::swizzle2() {
    QCOMPARE((vec2{0, 1}.yx()), (vec2{1, 0}));
    QCOMPARE((vec2{0, 1}.yy()), (vec2{1, 1}));
}

void MathTest::swizzle3() {
    QCOMPARE((vec3{0, 1}.yx()), (vec2{1, 0}));
    QCOMPARE((vec3{0, 1}.yy()), (vec2{1, 1}));
    QCOMPARE((vec3{0, 1, 2}.zyx()), (vec3{2, 1, 0}));
    QCOMPARE((vec3{0, 1, 2}.zzz()), (vec3{2, 2, 2}));
}

void MathTest::swizzle4() {
    QCOMPARE((vec4{0, 1}.yx()), (vec2{1, 0}));
    QCOMPARE((vec4{0, 1}.yy()), (vec2{1, 1}));
    QCOMPARE((vec4{0, 1, 2}.zyx()), (vec3{2, 1, 0}));
    QCOMPARE((vec4{0, 1, 2}.zzz()), (vec3{2, 2, 2}));
    QCOMPARE((vec4{0, 1, 2, 3}.wzyx()), (vec4{3, 2, 1, 0}));
    QCOMPARE((vec4{0, 1, 2, 3}.wwww()), (vec4{3, 3, 3, 3}));
}

void MathTest::clamp_len_vec_data() {
    constexpr auto sq2_2 = Math::sq2_2<float>();
    QTest::addColumn<vec2>("v");
    QTest::addColumn<vec2>("clamped");
    QTest::newRow("zero") << vec2{} << vec2{};
    QTest::newRow("x") << vec2{0.5f, 0} << vec2{0.5f, 0};
    QTest::newRow("x clamp") << vec2{1.5f, 0} << vec2{1, 0};
    QTest::newRow("y") << vec2{0, 0.5f} << vec2{0, 0.5f};
    QTest::newRow("y clamp") << vec2{0, 1.5f} << vec2{0, 1};
    QTest::newRow("xy") << vec2{1, 1} << vec2{sq2_2, sq2_2};
    QTest::newRow("xy clamp") << vec2{0.5f, 0.5f} << vec2{0.5f, 0.5f};
}

void MathTest::clamp_len_vec() {
    QFETCH(const vec2, v);
    QFETCH(const vec2, clamped);
    const vec2 ret = Math::clamp_len(v, 1.0f);
    QCOMPARE(ret, clamped);
}

void MathTest::rotate_data() {
    constexpr auto pi = [](float n, float d)
        { return Math::pi<float>() * n / d; };
    constexpr auto d = Math::sq2_2<float>();
    QTest::addColumn<float>("a");
    QTest::addColumn<vec2>("r");
    QTest::newRow("0")   << pi(0, 4) << vec2( 0,  1);
    QTest::newRow("45")  << pi(1, 4) << vec2(-d,  d);
    QTest::newRow("90")  << pi(2, 4) << vec2(-1,  0);
    QTest::newRow("135") << pi(3, 4) << vec2(-d, -d);
    QTest::newRow("180") << pi(4, 4) << vec2( 0, -1);
    QTest::newRow("225") << pi(5, 4) << vec2( d, -d);
    QTest::newRow("270") << pi(6, 4) << vec2( 1,  0);
    QTest::newRow("315") << pi(7, 4) << vec2( d,  d);
}

#define D(x) void MathTest::x() { MathTest::rotate_data(); }
D(rotate_x_data)
D(rotate_y_data)
D(rotate_z_data)
D(rotate_vec_x_data)
D(rotate_vec_y_data)
D(rotate_vec_z_data)
#undef D

void MathTest::rotate()
    { rotate_test<vec2, Math::rotate, &vec2::x, &vec2::y>(); }

void MathTest::rotate_x()
    { rotate_test<vec3, Math::rotate_x, &vec3::y, &vec3::z>(); }
void MathTest::rotate_y()
    { rotate_test<vec3, Math::rotate_y, &vec3::z, &vec3::x>(); }
void MathTest::rotate_z()
    { rotate_test<vec3, Math::rotate_z, &vec3::x, &vec3::y>(); }

void MathTest::rotate_vec_x()
    { rotate_vec_test<vec3, 1, 0, 0, &vec3::y, &vec3::z>(); }
void MathTest::rotate_vec_y()
    { rotate_vec_test<vec3, 0, 1, 0, &vec3::z, &vec3::x>(); }
void MathTest::rotate_vec_z()
    { rotate_vec_test<vec3, 0, 0, 1, &vec3::x, &vec3::y>(); }

void MathTest::mat4_rotate_data() {
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

void MathTest::mat4_rotate() {
    QFETCH(const float, angle);
    QFETCH(const vec3, axis);
    QFETCH(const vec4, expected);
    constexpr vec4 v = {1, 0, 0, 0};
    const auto m = Math::rotate(mat4{1}, Math::radians(angle), axis);
    if(const auto ret = m * v; !fuzzy_eq(ret, expected))
        QCOMPARE(ret, expected);
}

QTEST_MAIN(MathTest)

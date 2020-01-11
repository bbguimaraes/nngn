#include "math/math.h"

#include "utils/literals.h"

#include "tests/tests.h"

#include "math_test.h"

using namespace nngn::literals;
using nngn::Math, nngn::vec2, nngn::vec3, nngn::vec4, nngn::mat4;

Q_DECLARE_METATYPE(vec2)
Q_DECLARE_METATYPE(vec3)
Q_DECLARE_METATYPE(vec4)

namespace {

template<typename T, typename T::type T::*ordinate>
auto rotate_initial(void) {
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
    T F(T, typename T::type),
    typename T::type T::*abscissa,
    typename T::type T::*ordinate
> void rotate_test(void) {
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
> void rotate_vec_test(void) {
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

void MathTest::round_down_pow2_data(void) {
    QTest::addColumn<unsigned>("n");
    QTest::addColumn<unsigned>("e");
    QTest::newRow("0") << 0u << 0u;
    QTest::newRow("1") << 1u << 0u;
    QTest::newRow("2") << 2u << 0u;
    QTest::newRow("7") << 7u << 0u;
    QTest::newRow("8") << 8u << 8u;
}

void MathTest::round_down_pow2(void) {
    QFETCH(const unsigned, n);
    QFETCH(const unsigned, e);
    QCOMPARE(Math::round_down_pow2(n, 8u), e);
}

void MathTest::round_up_pow2_data(void) {
    QTest::addColumn<unsigned>("n");
    QTest::addColumn<unsigned>("e");
    QTest::newRow("0") << 0u << 0u;
    QTest::newRow("1") << 1u << 8u;
    QTest::newRow("2") << 2u << 8u;
    QTest::newRow("7") << 7u << 8u;
    QTest::newRow("8") << 8u << 8u;
}

void MathTest::round_up_pow2(void) {
    QFETCH(const unsigned, n);
    QFETCH(const unsigned, e);
    QCOMPARE(Math::round_up_pow2(n, 8u), e);
}

void MathTest::round_down_data(void) {
    QTest::addColumn<unsigned>("n");
    QTest::addColumn<unsigned>("e");
    QTest::newRow("0 7") << 0u << 0u;
    QTest::newRow("1 7") << 1u << 0u;
    QTest::newRow("2 7") << 2u << 0u;
    QTest::newRow("6 7") << 6u << 0u;
    QTest::newRow("7 7") << 7u << 7u;
}

void MathTest::round_down(void) {
    QFETCH(const unsigned, n);
    QFETCH(const unsigned, e);
    QCOMPARE(Math::round_down(n, 7u), e);
}

void MathTest::round_up_data(void) {
    QTest::addColumn<unsigned>("n");
    QTest::addColumn<unsigned>("e");
    QTest::newRow("0 7") << 0u << 0u;
    QTest::newRow("1 7") << 1u << 7u;
    QTest::newRow("2 7") << 2u << 7u;
    QTest::newRow("6 7") << 6u << 7u;
    QTest::newRow("7 7") << 7u << 7u;
}

void MathTest::round_up(void) {
    QFETCH(const unsigned, n);
    QFETCH(const unsigned, e);
    QCOMPARE(Math::round_up(n, 7u), e);
}

void MathTest::avg2_data(void) {
    QTest::addColumn<vec2>("v");
    QTest::addColumn<float>("avg");
    QTest::newRow("zero") << vec2{} << 0.0f;
    QTest::newRow("x") << vec2{1, 0} << 0.5f;
    QTest::newRow("y") << vec2{0, 1} << 0.5f;
    QTest::newRow("xy") << vec2{1, 1} << 1.0f;
}

void MathTest::avg2(void) {
    QFETCH(const vec2, v);
    QFETCH(const float, avg);
    QCOMPARE(Math::avg(v), avg);
}

void MathTest::avg3_data(void) {
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

void MathTest::avg3(void) {
    QFETCH(const vec3, v);
    QFETCH(const float, avg);
    QCOMPARE(Math::avg(v), avg);
}

void MathTest::avg4_data(void) {
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

void MathTest::avg4(void) {
    QFETCH(const vec4, v);
    QFETCH(const float, avg);
    QCOMPARE(Math::avg(v), avg);
}

void MathTest::swizzle2(void) {
    QCOMPARE((vec2{0, 1}.yx()), (vec2{1, 0}));
    QCOMPARE((vec2{0, 1}.yy()), (vec2{1, 1}));
}

void MathTest::swizzle3(void) {
    QCOMPARE((vec3{0, 1}.yx()), (vec2{1, 0}));
    QCOMPARE((vec3{0, 1}.yy()), (vec2{1, 1}));
    QCOMPARE((vec3{0, 1, 2}.zyx()), (vec3{2, 1, 0}));
    QCOMPARE((vec3{0, 1, 2}.zzz()), (vec3{2, 2, 2}));
}

void MathTest::swizzle4(void) {
    QCOMPARE((vec4{0, 1}.yx()), (vec2{1, 0}));
    QCOMPARE((vec4{0, 1}.yy()), (vec2{1, 1}));
    QCOMPARE((vec4{0, 1, 2}.zyx()), (vec3{2, 1, 0}));
    QCOMPARE((vec4{0, 1, 2}.zzz()), (vec3{2, 2, 2}));
    QCOMPARE((vec4{0, 1, 2, 3}.wzyx()), (vec4{3, 2, 1, 0}));
    QCOMPARE((vec4{0, 1, 2, 3}.wwww()), (vec4{3, 3, 3, 3}));
}

void MathTest::clamp_len_vec_data(void) {
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

void MathTest::clamp_len_vec(void) {
    QFETCH(const vec2, v);
    QFETCH(const vec2, clamped);
    const vec2 ret = Math::clamp_len(v, 1.0f);
    QCOMPARE(ret, clamped);
}

void MathTest::rotate_data(void) {
    constexpr auto pi = [](float n, float d) {
        return Math::pi<float>() * n / d;
    };
    constexpr auto d = Math::sq2_2<float>();
    QTest::addColumn<float>("a");
    QTest::addColumn<vec2>("r");
    QTest::newRow("0")   << pi(0, 4) << vec2{ 0,  1};
    QTest::newRow("45")  << pi(1, 4) << vec2{-d,  d};
    QTest::newRow("90")  << pi(2, 4) << vec2{-1,  0};
    QTest::newRow("135") << pi(3, 4) << vec2{-d, -d};
    QTest::newRow("180") << pi(4, 4) << vec2{ 0, -1};
    QTest::newRow("225") << pi(5, 4) << vec2{ d, -d};
    QTest::newRow("270") << pi(6, 4) << vec2{ 1,  0};
    QTest::newRow("315") << pi(7, 4) << vec2{ d,  d};
}

#define D(x) void MathTest::x(void) { MathTest::rotate_data(); }
D(rotate_x_data)
D(rotate_y_data)
D(rotate_z_data)
D(rotate_vec_x_data)
D(rotate_vec_y_data)
D(rotate_vec_z_data)
#undef D

void MathTest::rotate(void) {
    rotate_test<vec2, Math::rotate, &vec2::x, &vec2::y>();
}

void MathTest::rotate_x(void) {
    rotate_test<vec3, Math::rotate_x, &vec3::y, &vec3::z>();
}

void MathTest::rotate_y(void) {
    rotate_test<vec3, Math::rotate_y, &vec3::z, &vec3::x>();
}

void MathTest::rotate_z(void) {
    rotate_test<vec3, Math::rotate_z, &vec3::x, &vec3::y>();
}

void MathTest::rotate_vec_x(void) {
    rotate_vec_test<vec3, 1, 0, 0, &vec3::y, &vec3::z>();
}

void MathTest::rotate_vec_y(void) {
    rotate_vec_test<vec3, 0, 1, 0, &vec3::z, &vec3::x>();
}

void MathTest::rotate_vec_z(void) {
    rotate_vec_test<vec3, 0, 0, 1, &vec3::x, &vec3::y>();
}

void MathTest::ortho(void) {
    constexpr float width = 800, height = 600;
    const auto ret = Math::ortho(
        width / -2, width / 2, height / -2, height / 2, 0.01f, 2048.0f);
    const float x = 0.0025f;
    const float y = 0.0033333333333333335f;
    const float z = -0.0009765672683948652f;
    const float zt = -1.000009765672684f;
    const auto ortho = Math::transpose(mat4{
        {x, 0, 0,  0},
        {0, y, 0,  0},
        {0, 0, z, zt},
        {0, 0, 0,  1},
    });
    if(!fuzzy_eq(ret, ortho))
        QCOMPARE(ret, ortho);
}

void MathTest::perspective(void) {
    const auto ret = Math::perspective(
        Math::pi<float>() / 6.0f, 800.0f / 600.0f, 0.01f, 2048.0f);
    const float z = -1.000009765672684f;
    const float y = 3.7320508075688776f;
    const float x = 2.799038105676658f;
    const float zt = -0.02000009765672684f;
    const auto persp = Math::transpose(mat4{
        {x, 0,  0,  0},
        {0, y,  0,  0},
        {0, 0,  z, zt},
        {0, 0, -1,  0},
    });
    if(!fuzzy_eq(ret, persp))
        QCOMPARE(ret, persp);
}

QTEST_MAIN(MathTest)

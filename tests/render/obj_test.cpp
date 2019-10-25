#include "obj_test.h"

#include <sstream>

#include "math/vec3.h"
#include "render/obj.h"
#include "utils/ranges.h"

#include "tests/tests.h"

using nngn::vec3, nngn::zvec3;

namespace {

struct parse_result {
    std::vector<vec3> vs, ts, ns;
    std::vector<std::array<zvec3, 3>> fs;
};

auto parse(const char *input, parse_result e) {
    std::vector<vec3> vs = {}, ts = {}, ns = {};
    std::vector<std::array<zvec3, 3>> fs = {};
    auto f = std::istringstream{input};
    const bool ok = nngn::parse_obj(
        &f, std::span{nngn::owning_view{std::array<char, 64>{}}},
        [&v = vs](auto x) { v.push_back(x); },
        [&v = ts](auto x) { v.push_back(x); },
        [&v = ns](auto x) { v.push_back(x); },
        [&v = fs](auto x) { v.push_back(x); });
    QVERIFY(ok);
    QCOMPARE(vs, e.vs);
    QCOMPARE(ts, e.ts);
    QCOMPARE(ns, e.ns);
    QCOMPARE(fs, e.fs);
}

}

void ObjTest::parse_empty(void) {
    ::parse("", {});
}

void ObjTest::parse_comment(void) {
    ::parse("#\n# a comment\no # comment", {});
}

void ObjTest::parse_cr(void) {
    ::parse("\r\n", {});
}

void ObjTest::parse_ignored(void) {
    ::parse("o\ng\ns\nmtllib\nmtl", {});
}

void ObjTest::parse_space(void) {
    ::parse("o ", {});
}

void ObjTest::parse_vertex(void) {
    ::parse("v", {.vs = {vec3{0, 0, 0}}});
    ::parse("v 1.0 2.0", {.vs = {vec3{1, 2, 0}}});
    ::parse("v 1.0 2.0 3.0", {.vs = {vec3{1, 2, 3}}});
}

void ObjTest::parse(void) {
    ::parse(
        R"(v 0.0 1.0 2.0
vt 3.0 4.0 5.0
vn 6.0 7.0 8.0
f 1 2 3
v 9.0 10.0 11.0
vt 12.0 13.0 14.0
vn 15.0 16.0 17.0
f 4 5 6
v 18.0 19.0 20.0
vt 21.0 22.0 23.0
vn 24.0 25.0 26.0
f 7 8 9
)", {
        .vs = std::vector{vec3{0, 1, 2}, vec3{9, 10, 11}, vec3{18, 19, 20}},
        .ts = std::vector{vec3{3, 4, 5}, vec3{12, 13, 14}, vec3{21, 22, 23}},
        .ns = std::vector{vec3{6, 7, 8}, vec3{15, 16, 17}, vec3{24, 25, 26}},
        .fs = std::vector{
            std::array{zvec3{0, 0, 0}, zvec3{1, 1, 1}, zvec3{2, 2, 2}},
            std::array{zvec3{3, 3, 3}, zvec3{4, 4, 4}, zvec3{5, 5, 5}},
            std::array{zvec3{6, 6, 6}, zvec3{7, 7, 7}, zvec3{8, 8, 8}},
        },
    });
}

void ObjTest::parse_faces(void) {
    ::parse("f 1/2/3 4/5/6 7/8/9", {
        .fs = std::vector{
            std::array{zvec3{0, 1, 2}, zvec3{3, 4, 5}, zvec3{6, 7, 8}},
        },
    });
    ::parse("f 1/2 3/4 5/6", {
        .fs = std::vector{
            std::array{zvec3{0, 1, 0}, zvec3{2, 3, 0}, zvec3{4, 5, 0}},
        },
    });
    ::parse("f 1//2 3//4 5//6", {
        .fs = std::vector{
            std::array{zvec3{0, 0, 1}, zvec3{2, 0, 3}, zvec3{4, 0, 5}},
        },
    });
    constexpr zvec3
        f0 = {0, 1, 2}, f1 = {3, 4, 5}, f2 = {6, 7, 8}, f3 = {9, 10, 11},
        f4 = {12, 13}, f5 = {14, 0, 15}, f6 = zvec3{16};
    ::parse("f 1/2/3 4/5/6 7/8/9 10/11/12 13/14 15//16 17", {
        .fs = std::vector{
            std::array{f0, f1, f2}, std::array{f0, f2, f3},
            std::array{f0, f3, f4}, std::array{f0, f4, f5},
            std::array{f0, f5, f6},
        },
    });
}

QTEST_MAIN(ObjTest)

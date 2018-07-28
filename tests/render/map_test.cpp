#include <sstream>

#include "debug.h"

#include "lua/state.h"
#include "graphics/pseudo.h"
#include "render/map.h"

#include "map_test.h"

using nngn::u32, nngn::u64;

struct MapTestGraphics : nngn::Pseudograph {
    std::vector<nngn::Vertex> vbo = {};
    std::vector<u32> ebo = {};
    struct {
        u64 off = 0, len = 0;
    } vbo_copy = {}, ebo_copy = {};
    u64 vbo_size = 0, ebo_size = 0;
    u32 create_buffer(const BufferConfiguration &conf) final;
    bool write_to_buffer(
        u32 b, u64 offset, u64 n, u64 size,
        void *data, void f(void*, void*, u64, u64)) final;
    bool set_buffer_size(u32 b, u64 size) final;
};

namespace nngn {
    bool operator==(const Vertex &l, const Vertex &r);
}

u32 MapTestGraphics::create_buffer(const BufferConfiguration &conf) {
    switch(conf.type) {
    case BufferConfiguration::Type::VERTEX: return 1;
    case BufferConfiguration::Type::INDEX: return 2;
    default: return 0;
    }
}

bool MapTestGraphics::write_to_buffer(
    u32 b, u64 offset, u64 n, u64 size,
    void *data, void f(void*, void*, u64, u64)
) {
    if(b == 1) {
        this->vbo_copy = {offset, n * size};
        this->vbo.resize(
            static_cast<std::size_t>(n * size / sizeof(nngn::Vertex)));
        f(data, this->vbo.data() + offset, 0, n);
        return true;
    }
    if(b == 2) {
        this->ebo_copy = {offset, n * size};
        this->ebo.resize(static_cast<std::size_t>(n * size / sizeof(u32)));
        f(data, this->ebo.data() + offset, 0, n);
        return true;
    }
    assert(false);
    return false;
}

bool MapTestGraphics::set_buffer_size(u32 b, u64 size) {
    switch(b) {
    case 1: this->vbo_size = size; return true;
    case 2: this->ebo_size = size; return true;
    default: assert(false); return false;
    }
}

namespace nngn {
    bool operator ==(const Vertex &l, const Vertex &r)
        { return l.pos == r.pos && l.norm == r.norm && l.color == r.color; }
}

void MapTest::load_tiles() {
    nngn::lua::state lua;
    QVERIFY(lua.init());
    const auto t = lua.create_table(12, 0);
    for(unsigned int i = 0; i < 12; ++i)
        t.raw_set(i + 1, i);
    const auto v = nngn::Map::load_tiles(3, 2, t);
    QCOMPARE(v.size(), 6ul);
    QCOMPARE(v[0], nngn::uvec2( 0,  1));
    QCOMPARE(v[1], nngn::uvec2( 2,  3));
    QCOMPARE(v[2], nngn::uvec2( 4,  5));
    QCOMPARE(v[3], nngn::uvec2( 6,  7));
    QCOMPARE(v[4], nngn::uvec2( 8,  9));
    QCOMPARE(v[5], nngn::uvec2(10, 11));
}

void MapTest::gen() {
    nngn::lua::state lua;
    QVERIFY(lua.init());
    MapTestGraphics g;
    nngn::Map m;
    m.init(nullptr);
    m.set_graphics(&g);
    constexpr std::size_t w = 4, h = 5, n = w * h;
    constexpr std::size_t vsize = n * 4 * sizeof(nngn::Vertex);
    constexpr std::size_t isize = n * 6 * sizeof(u32);
    const auto t = lua.create_table(2 * n, 0);
    for(lua_Integer i = 0, n2 = 2 * n; i < n2; ++i)
        t.raw_set(i + 1, i);
    QVERIFY(m.load(1, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f, w, h, t));
    QCOMPARE(g.vbo.size(), 4 * n);
    QCOMPARE(g.ebo.size(), 6 * n);
    QCOMPARE(g.vbo_copy.off, 0);
    QCOMPARE(g.vbo_copy.len, vsize);
    QCOMPARE(g.ebo_copy.off, 0);
    QCOMPARE(g.ebo_copy.len, isize);
    QCOMPARE(g.vbo_size, vsize);
    QCOMPARE(g.ebo_size, isize);
    constexpr nngn::Vertex v[4 * w * h] = {
        {{-7.0f, -11.0f, 0}, {0, 0, 1}, { 0.0f,   0.5f, 1}},
        {{-2.0f, -11.0f, 0}, {0, 0, 1}, { 0.5f,   0.5f, 1}},
        {{-7.0f,  -5.0f, 0}, {0, 0, 1}, { 0.0f,   0.0f, 1}},
        {{-2.0f,  -5.0f, 0}, {0, 0, 1}, { 0.5f,   0.0f, 1}},
        {{-2.0f, -11.0f, 0}, {0, 0, 1}, { 1.0f,  -0.5f, 1}},
        {{ 3.0f, -11.0f, 0}, {0, 0, 1}, { 1.5f,  -0.5f, 1}},
        {{-2.0f,  -5.0f, 0}, {0, 0, 1}, { 1.0f,  -1.0f, 1}},
        {{ 3.0f,  -5.0f, 0}, {0, 0, 1}, { 1.5f,  -1.0f, 1}},
        {{ 3.0f, -11.0f, 0}, {0, 0, 1}, { 2.0f,  -1.5f, 1}},
        {{ 8.0f, -11.0f, 0}, {0, 0, 1}, { 2.5f,  -1.5f, 1}},
        {{ 3.0f,  -5.0f, 0}, {0, 0, 1}, { 2.0f,  -2.0f, 1}},
        {{ 8.0f,  -5.0f, 0}, {0, 0, 1}, { 2.5f,  -2.0f, 1}},
        {{ 8.0f, -11.0f, 0}, {0, 0, 1}, { 3.0f,  -2.5f, 1}},
        {{13.0f, -11.0f, 0}, {0, 0, 1}, { 3.5f,  -2.5f, 1}},
        {{ 8.0f,  -5.0f, 0}, {0, 0, 1}, { 3.0f,  -3.0f, 1}},
        {{13.0f,  -5.0f, 0}, {0, 0, 1}, { 3.5f,  -3.0f, 1}},
        {{-7.0f,  -5.0f, 0}, {0, 0, 1}, { 4.0f,  -3.5f, 1}},
        {{-2.0f,  -5.0f, 0}, {0, 0, 1}, { 4.5f,  -3.5f, 1}},
        {{-7.0f,   1.0f, 0}, {0, 0, 1}, { 4.0f,  -4.0f, 1}},
        {{-2.0f,   1.0f, 0}, {0, 0, 1}, { 4.5f,  -4.0f, 1}},
        {{-2.0f,  -5.0f, 0}, {0, 0, 1}, { 5.0f,  -4.5f, 1}},
        {{ 3.0f,  -5.0f, 0}, {0, 0, 1}, { 5.5f,  -4.5f, 1}},
        {{-2.0f,   1.0f, 0}, {0, 0, 1}, { 5.0f,  -5.0f, 1}},
        {{ 3.0f,   1.0f, 0}, {0, 0, 1}, { 5.5f,  -5.0f, 1}},
        {{ 3.0f,  -5.0f, 0}, {0, 0, 1}, { 6.0f,  -5.5f, 1}},
        {{ 8.0f,  -5.0f, 0}, {0, 0, 1}, { 6.5f,  -5.5f, 1}},
        {{ 3.0f,   1.0f, 0}, {0, 0, 1}, { 6.0f,  -6.0f, 1}},
        {{ 8.0f,   1.0f, 0}, {0, 0, 1}, { 6.5f,  -6.0f, 1}},
        {{ 8.0f,  -5.0f, 0}, {0, 0, 1}, { 7.0f,  -6.5f, 1}},
        {{13.0f,  -5.0f, 0}, {0, 0, 1}, { 7.5f,  -6.5f, 1}},
        {{ 8.0f,   1.0f, 0}, {0, 0, 1}, { 7.0f,  -7.0f, 1}},
        {{13.0f,   1.0f, 0}, {0, 0, 1}, { 7.5f,  -7.0f, 1}},
        {{-7.0f,   1.0f, 0}, {0, 0, 1}, { 8.0f,  -7.5f, 1}},
        {{-2.0f,   1.0f, 0}, {0, 0, 1}, { 8.5f,  -7.5f, 1}},
        {{-7.0f,   7.0f, 0}, {0, 0, 1}, { 8.0f,  -8.0f, 1}},
        {{-2.0f,   7.0f, 0}, {0, 0, 1}, { 8.5f,  -8.0f, 1}},
        {{-2.0f,   1.0f, 0}, {0, 0, 1}, { 9.0f,  -8.5f, 1}},
        {{ 3.0f,   1.0f, 0}, {0, 0, 1}, { 9.5f,  -8.5f, 1}},
        {{-2.0f,   7.0f, 0}, {0, 0, 1}, { 9.0f,  -9.0f, 1}},
        {{ 3.0f,   7.0f, 0}, {0, 0, 1}, { 9.5f,  -9.0f, 1}},
        {{ 3.0f,   1.0f, 0}, {0, 0, 1}, {10.0f,  -9.5f, 1}},
        {{ 8.0f,   1.0f, 0}, {0, 0, 1}, {10.5f,  -9.5f, 1}},
        {{ 3.0f,   7.0f, 0}, {0, 0, 1}, {10.0f, -10.0f, 1}},
        {{ 8.0f,   7.0f, 0}, {0, 0, 1}, {10.5f, -10.0f, 1}},
        {{ 8.0f,   1.0f, 0}, {0, 0, 1}, {11.0f, -10.5f, 1}},
        {{13.0f,   1.0f, 0}, {0, 0, 1}, {11.5f, -10.5f, 1}},
        {{ 8.0f,   7.0f, 0}, {0, 0, 1}, {11.0f, -11.0f, 1}},
        {{13.0f,   7.0f, 0}, {0, 0, 1}, {11.5f, -11.0f, 1}},
        {{-7.0f,   7.0f, 0}, {0, 0, 1}, {12.0f, -11.5f, 1}},
        {{-2.0f,   7.0f, 0}, {0, 0, 1}, {12.5f, -11.5f, 1}},
        {{-7.0f,  13.0f, 0}, {0, 0, 1}, {12.0f, -12.0f, 1}},
        {{-2.0f,  13.0f, 0}, {0, 0, 1}, {12.5f, -12.0f, 1}},
        {{-2.0f,   7.0f, 0}, {0, 0, 1}, {13.0f, -12.5f, 1}},
        {{ 3.0f,   7.0f, 0}, {0, 0, 1}, {13.5f, -12.5f, 1}},
        {{-2.0f,  13.0f, 0}, {0, 0, 1}, {13.0f, -13.0f, 1}},
        {{ 3.0f,  13.0f, 0}, {0, 0, 1}, {13.5f, -13.0f, 1}},
        {{ 3.0f,   7.0f, 0}, {0, 0, 1}, {14.0f, -13.5f, 1}},
        {{ 8.0f,   7.0f, 0}, {0, 0, 1}, {14.5f, -13.5f, 1}},
        {{ 3.0f,  13.0f, 0}, {0, 0, 1}, {14.0f, -14.0f, 1}},
        {{ 8.0f,  13.0f, 0}, {0, 0, 1}, {14.5f, -14.0f, 1}},
        {{ 8.0f,   7.0f, 0}, {0, 0, 1}, {15.0f, -14.5f, 1}},
        {{13.0f,   7.0f, 0}, {0, 0, 1}, {15.5f, -14.5f, 1}},
        {{ 8.0f,  13.0f, 0}, {0, 0, 1}, {15.0f, -15.0f, 1}},
        {{13.0f,  13.0f, 0}, {0, 0, 1}, {15.5f, -15.0f, 1}},
        {{-7.0f,  13.0f, 0}, {0, 0, 1}, {16.0f, -15.5f, 1}},
        {{-2.0f,  13.0f, 0}, {0, 0, 1}, {16.5f, -15.5f, 1}},
        {{-7.0f,  19.0f, 0}, {0, 0, 1}, {16.0f, -16.0f, 1}},
        {{-2.0f,  19.0f, 0}, {0, 0, 1}, {16.5f, -16.0f, 1}},
        {{-2.0f,  13.0f, 0}, {0, 0, 1}, {17.0f, -16.5f, 1}},
        {{ 3.0f,  13.0f, 0}, {0, 0, 1}, {17.5f, -16.5f, 1}},
        {{-2.0f,  19.0f, 0}, {0, 0, 1}, {17.0f, -17.0f, 1}},
        {{ 3.0f,  19.0f, 0}, {0, 0, 1}, {17.5f, -17.0f, 1}},
        {{ 3.0f,  13.0f, 0}, {0, 0, 1}, {18.0f, -17.5f, 1}},
        {{ 8.0f,  13.0f, 0}, {0, 0, 1}, {18.5f, -17.5f, 1}},
        {{ 3.0f,  19.0f, 0}, {0, 0, 1}, {18.0f, -18.0f, 1}},
        {{ 8.0f,  19.0f, 0}, {0, 0, 1}, {18.5f, -18.0f, 1}},
        {{ 8.0f,  13.0f, 0}, {0, 0, 1}, {19.0f, -18.5f, 1}},
        {{13.0f,  13.0f, 0}, {0, 0, 1}, {19.5f, -18.5f, 1}},
        {{ 8.0f,  19.0f, 0}, {0, 0, 1}, {19.0f, -19.0f, 1}},
        {{13.0f,  19.0f, 0}, {0, 0, 1}, {19.5f, -19.0f, 1}},
    };
    const auto vcmp = vdiff(g.vbo, v);
    if(!vcmp.empty())
        QFAIL(vcmp.c_str());
    constexpr uint32_t i[6 * w * h] = {
         0,  1,  2,  2,  1,  3,
         4,  5,  6,  6,  5,  7,
         8,  9, 10, 10,  9, 11,
        12, 13, 14, 14, 13, 15,
        16, 17, 18, 18, 17, 19,
        20, 21, 22, 22, 21, 23,
        24, 25, 26, 26, 25, 27,
        28, 29, 30, 30, 29, 31,
        32, 33, 34, 34, 33, 35,
        36, 37, 38, 38, 37, 39,
        40, 41, 42, 42, 41, 43,
        44, 45, 46, 46, 45, 47,
        48, 49, 50, 50, 49, 51,
        52, 53, 54, 54, 53, 55,
        56, 57, 58, 58, 57, 59,
        60, 61, 62, 62, 61, 63,
        64, 65, 66, 66, 65, 67,
        68, 69, 70, 70, 69, 71,
        72, 73, 74, 74, 73, 75,
        76, 77, 78, 78, 77, 79,
    };
    const auto ecmp = vdiff(g.ebo, i);
    if(!ecmp.empty())
        QFAIL(ecmp.c_str());
}

QTEST_MAIN(MapTest)

#include "static_vector_test.h"

#include <sstream>

#include "utils/static_vector.h"

#include "tests/tests.h"

namespace {

struct S {
    std::uintptr_t p = {};
    std::size_t id = {};
    S() = default;
    explicit S(std::size_t id_) : id(id_) {}
    bool operator==(const S &rhs) const { return this->id == rhs.id; }
};

auto make_vec(std::size_t n) {
    nngn::static_vector<S> ret(n);
    for(std::size_t i = 1; i <= n; ++i)
        ret.emplace(i);
    return ret;
}

}

namespace nngn {

std::string str(const static_vector<S> &v);

std::string str(const static_vector<S> &v) {
    if(v.empty())
        return "{}";
    std::stringstream s;
    s << "{" << v[0].id;
    for(auto i = begin(v) + 1, e = end(v); i != e; ++i)
        s << ", " << i->id;
    s << "}";
    return s.str();
}

}

void StaticVectorTest::emplace() {
    nngn::static_vector<S> v(3);
    QCOMPARE(str(v), "{}");
    v.emplace(std::size_t{1});
    QCOMPARE(str(v), "{1}");
    v.emplace(std::size_t{2});
    QCOMPARE(str(v), "{1, 2}");
    v.emplace(std::size_t{3});
    QCOMPARE(str(v), "{1, 2, 3}");
    QVERIFY(v.full());
}

void StaticVectorTest::erase() {
    auto v = make_vec(3);
    const auto b = begin(v);
    QCOMPARE(v.size(), 3);
    QCOMPARE(v.n_free(), 0);
    QVERIFY(v.full());
    QCOMPARE(str(v), "{1, 2, 3}");
    v.erase(b);
    QCOMPARE(v.size(), 2);
    QCOMPARE(v.n_free(), 1);
    QVERIFY(!v.full());
    QCOMPARE(str(v), "{0, 2, 3}");
    v.erase(b + 1);
    QCOMPARE(v.size(), 1);
    QCOMPARE(v.n_free(), 2);
    QVERIFY(!v.full());
    QCOMPARE(str(v), "{0, 0, 3}");
    v.erase(b + 2);
    QCOMPARE(v.size(), 0);
    QCOMPARE(v.n_free(), 2);
    QVERIFY(!v.full());
    QCOMPARE(str(v), "{0, 0}");
}

void StaticVectorTest::erase_reverse() {
    auto v = make_vec(3);
    const auto b = begin(v);
    QCOMPARE(v.size(), 3);
    QCOMPARE(v.n_free(), 0);
    QVERIFY(v.full());
    QCOMPARE(str(v), "{1, 2, 3}");
    v.erase(b + 2);
    QCOMPARE(v.size(), 2);
    QCOMPARE(v.n_free(), 0);
    QVERIFY(!v.full());
    QCOMPARE(str(v), "{1, 2}");
    v.erase(b + 1);
    QCOMPARE(v.size(), 1);
    QCOMPARE(v.n_free(), 0);
    QVERIFY(!v.full());
    QCOMPARE(str(v), "{1}");
    v.erase(b);
    QCOMPARE(v.size(), 0);
    QCOMPARE(v.n_free(), 0);
    QVERIFY(!v.full());
    QCOMPARE(str(v), "{}");
}

void StaticVectorTest::erase_emplace() {
    auto v = make_vec(3);
    const auto b = begin(v);
    QCOMPARE(v.size(), 3);
    QVERIFY(v.full());
    QCOMPARE(str(v), "{1, 2, 3}");
    v.erase(b + 2);
    QCOMPARE(v.size(), 2);
    QCOMPARE(v.n_free(), 0);
    QVERIFY(!v.full());
    QCOMPARE(str(v), "{1, 2}");
    v.emplace(std::size_t{3});
    QCOMPARE(v.size(), 3);
    QCOMPARE(v.n_free(), 0);
    QVERIFY(v.full());
    QCOMPARE(str(v), "{1, 2, 3}");
    v.erase(b + 1);
    QCOMPARE(v.size(), 2);
    QCOMPARE(v.n_free(), 1);
    QVERIFY(!v.full());
    QCOMPARE(str(v), "{1, 0, 3}");
    v.erase(b);
    QCOMPARE(v.size(), 1);
    QCOMPARE(v.n_free(), 2);
    QVERIFY(!v.full());
    QCOMPARE(str(v), "{0, 0, 3}");
    v.emplace(std::size_t{1});
    QCOMPARE(v.size(), 2);
    QCOMPARE(v.n_free(), 1);
    QVERIFY(!v.full());
    QCOMPARE(str(v), "{1, 0, 3}");
    v.emplace(std::size_t{2});
    QCOMPARE(v.size(), 3);
    QCOMPARE(v.n_free(), 0);
    QVERIFY(v.full());
    QCOMPARE(str(v), "{1, 2, 3}");
}

void StaticVectorTest::set_capacity() {
    nngn::static_vector<S> v;
    QCOMPARE(v.capacity(), 0);
    QCOMPARE(v.size(), 0);
    v.set_capacity(8);
    QCOMPARE(v.capacity(), 8);
    QCOMPARE(v.size(), 0);
    for(std::size_t i = 0; i < 4; ++i)
        v.emplace();
    QCOMPARE(v.capacity(), 8);
    QCOMPARE(v.size(), 4);
    v.set_capacity(4);
    QCOMPARE(v.capacity(), 4);
    QCOMPARE(v.size(), 0);
    v.set_capacity(8);
    QCOMPARE(v.capacity(), 8);
    QCOMPARE(v.size(), 0);
}

QTEST_MAIN(StaticVectorTest)

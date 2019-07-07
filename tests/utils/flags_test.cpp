#include "utils/flags.h"

#include "flags_test.h"

void FlagsTest::constructor() {
    enum E : std::uint8_t {};
    using F = nngn::Flags<E>;
    static_assert(F().t == 0, "");
    static_assert(F{}.t == 0, "");
}

void FlagsTest::operator_bool() {
    enum E : std::uint8_t { ONE = 1u << 0 };
    using F = nngn::Flags<E>;
    static_assert(!F{}, "");
    static_assert(F{E::ONE}, "");
}

void FlagsTest::is_set() {
    enum E : std::uint8_t { ONE = 1u << 0 };
    using F = nngn::Flags<E>;
    static_assert(!F{}.is_set(E::ONE), "");
    static_assert(F{E::ONE}.is_set(E::ONE), "");
}

void FlagsTest::set() {
    enum E : std::uint8_t { ONE = 1u << 0 };
    using F = nngn::Flags<E>;
    static_assert((F{} | E::ONE).t == 1, "");
    static_assert(F{}.set(E::ONE).t == 1, "");
    F f{};
    QCOMPARE((f |= E::ONE).t, 1);
    QCOMPARE(f.t, 1);
}

void FlagsTest::set_bool() {
    enum E : std::uint8_t { TWO = 1u << 1 };
    using F = nngn::Flags<E>;
    static_assert(F{}.set(E::TWO, true).t == 2, "");
    static_assert(F{E::TWO}.set(E::TWO, false).t == 0, "");
}

void FlagsTest::clear() {
    enum E : std::uint8_t { ONE = 1u << 0 };
    using F = nngn::Flags<E>;
    static_assert((F{E::ONE} & 0).t == 0, "");
    static_assert(F{E::ONE}.clear(E::ONE).t == 0, "");
    F f{E::ONE};
    QCOMPARE((f &= 0).t, 0);
    QCOMPARE(f.t, 0);
}

void FlagsTest::check_and_clear() {
    enum E : std::uint8_t { ONE = 1u << 0 };
    nngn::Flags<E> f;
    f.set(E::ONE, true);
    QVERIFY(f.is_set(E::ONE));
    QVERIFY(f.check_and_clear(E::ONE));
    QVERIFY(!f.is_set(E::ONE));
    QVERIFY(!f.check_and_clear(E::ONE));
}

void FlagsTest::exclusive_or() {
    enum E : std::uint8_t { ONE = 1u << 0, TWO = 1u << 1 };
    using F = nngn::Flags<E>;
    static_assert((F{} ^ E::ONE).t == 1, "");
    static_assert((F{} ^ E::ONE).t == 1, "");
    F f{};
    QCOMPARE((f ^= E::ONE).t, 1);
    QCOMPARE(f.t, 1);
}

void FlagsTest::minus() {
    enum E : std::uint8_t { ONE = 1u << 0 };
    using F = nngn::Flags<E>;
    static_assert((-F{E::ONE}).t == 255, "");
}

void FlagsTest::flip() {
    enum E : std::uint8_t { ONE = 1u << 0, FOUR = 1u << 2 };
    using F = nngn::Flags<E>;
    static_assert((~(F{} | E::ONE | E::FOUR)).t == 250, "");
    static_assert((F{} | E::ONE | E::FOUR).flip().t ==  250, "");
}

QTEST_MAIN(FlagsTest)

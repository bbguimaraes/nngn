#include "flags_test.h"

#include "utils/def.h"
#include "utils/flags.h"

using nngn::u8;

void FlagsTest::constructor(void) {
    enum E : u8 {};
    using F = nngn::Flags<E>;
    static_assert(*F() == 0);
    static_assert(*F{} == 0);
}

void FlagsTest::operator_bool(void) {
    enum E : u8 { ONE = 1u << 0 };
    using F = nngn::Flags<E>;
    static_assert(!F{});
    static_assert(F{E::ONE});
}

void FlagsTest::is_set(void) {
    enum E : u8 { ONE = 1u << 0 };
    using F = nngn::Flags<E>;
    static_assert(!F{}.is_set(E::ONE));
    static_assert(F{E::ONE}.is_set(E::ONE));
}

void FlagsTest::set(void) {
    enum E : u8 { ONE = 1u << 0 };
    using F = nngn::Flags<E>;
    static_assert((F{} | E::ONE) == 1);
    static_assert(F{}.set(E::ONE) == 1);
    F f{};
    QCOMPARE(f |= E::ONE, 1);
    QCOMPARE(f, 1);
}

void FlagsTest::set_bool(void) {
    enum E : u8 { TWO = 1u << 1 };
    using F = nngn::Flags<E>;
    static_assert(F{}.set(E::TWO, true) == 2);
    static_assert(F{E::TWO}.set(E::TWO, false) == 0);
}

void FlagsTest::clear(void) {
    enum E : u8 { ONE = 1u << 0 };
    using F = nngn::Flags<E>;
    static_assert((F{E::ONE} & E{}) == 0);
    static_assert(F{E::ONE}.clear(E::ONE) == 0);
    F f{E::ONE};
    QCOMPARE(f &= 0, 0);
    QCOMPARE(f, 0);
}

void FlagsTest::check_and_clear(void) {
    enum E : u8 { ONE = 1u << 0 };
    nngn::Flags<E> f;
    f.set(E::ONE, true);
    QVERIFY(f.is_set(E::ONE));
    QVERIFY(f.check_and_clear(E::ONE));
    QVERIFY(!f.is_set(E::ONE));
    QVERIFY(!f.check_and_clear(E::ONE));
}

void FlagsTest::exclusive_or(void) {
    enum E : u8 { ONE = 1u << 0, TWO = 1u << 1 };
    using F = nngn::Flags<E>;
    static_assert((F{} ^ E::ONE) == 1);
    static_assert((F{} ^ E::ONE) == 1);
    F f{};
    QCOMPARE(f ^= E::ONE, 1);
    QCOMPARE(f, 1);
}

void FlagsTest::minus(void) {
    enum E : u8 { ONE = 1u << 0 };
    using F = nngn::Flags<E>;
    static_assert(-F{E::ONE} == 255);
}

void FlagsTest::comp(void) {
    enum E : u8 { ONE = 1u << 0, FOUR = 1u << 2 };
    using F = nngn::Flags<E>;
    static_assert((~(F{} | E::ONE | E::FOUR)) == 250);
    static_assert((F{} | E::ONE | E::FOUR).comp() ==  250);
}

QTEST_MAIN(FlagsTest)

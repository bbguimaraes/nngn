#include "types_test.h"

#include "utils/types.h"

namespace {

struct S0 {};
struct S1 {};
struct S2 {};

using types = nngn::types<S0, S1, S2>;

static_assert(std::is_same_v<nngn::types_first_t<types>, S0>);
static_assert(std::is_same_v<nngn::types_last_t<types>, S2>);

}

void TypesTest::test(void) {}

QTEST_MAIN(TypesTest)

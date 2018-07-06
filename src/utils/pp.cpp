#include "pp.h"

static_assert(0 == NNGN_ARGC());
static_assert(1 == NNGN_ARGC(_0));
static_assert(2 == NNGN_ARGC(_0, _1));
static_assert(3 == NNGN_ARGC(_0, _1, _2));
static_assert(4 == NNGN_ARGC(_0, _1, _2, _3));
static_assert(5 == NNGN_ARGC(_0, _1, _2, _3, _4));
static_assert(6 == NNGN_ARGC(_0, _1, _2, _3, _4, _5));
static_assert(7 == NNGN_ARGC(_0, _1, _2, _3, _4, _5, _6));

#define F(...) NNGN_OVERLOAD(F, __VA_ARGS__)
#define F0() 0
#define F1(_0) 1
#define F2(_0, _1) 2
static_assert(0 == F());
static_assert(1 == F(_0));
static_assert(2 == F(_0, _1));
#undef F2
#undef F1
#undef F0

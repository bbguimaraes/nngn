#include "native.h"

nngn::Colliders CollisionNativeBench::make_colliders() const {
    nngn::Colliders ret = {};
    ret.set_backend(nngn::Colliders::native_backend());
    return ret;
}

QTEST_MAIN(CollisionNativeBench)

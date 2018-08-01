#ifndef NNGN_TEST_BENCH_COLLISION_NATIVE_H
#define NNGN_TEST_BENCH_COLLISION_NATIVE_H

#include "collision.h"

class CollisionNativeBench : public CollisionBench {
    Q_OBJECT
    nngn::Colliders make_colliders() const override;
};

#endif

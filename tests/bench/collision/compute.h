#ifndef TESTS_BENCH_COLLISION_COMPUTE_H
#define TESTS_BENCH_COLLISION_COMPUTE_H

#include "compute/compute.h"

#include "collision.h"

class CollisionComputeBench : public CollisionBench {
    Q_OBJECT
    std::unique_ptr<nngn::Compute> compute = {};
    nngn::Colliders make_colliders() const override;
public:
    CollisionComputeBench();
};

#endif

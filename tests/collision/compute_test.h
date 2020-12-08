#ifndef NNGN_TEST_COMPUTE_H
#define NNGN_TEST_COMPUTE_H

#include <memory>

#include "compute/compute.h"

#include "collision_test.h"

class ComputeTest : public CollisionTest {
    Q_OBJECT
public:
    ComputeTest();
    ~ComputeTest() { this->colliders.set_backend(nullptr); }
private:
    std::unique_ptr<nngn::Compute> compute = {};
};

#endif

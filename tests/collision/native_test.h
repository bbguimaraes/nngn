#ifndef NNGN_TEST_COLLISION_NATIVE_H
#define NNGN_TEST_COLLISION_NATIVE_H

#include "collision_test.h"

class NativeTest : public CollisionTest {
    Q_OBJECT
public:
    NativeTest();
    ~NativeTest() { this->colliders.set_backend(nullptr); }
};

#endif

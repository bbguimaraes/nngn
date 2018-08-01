#include "native_test.h"

NativeTest::NativeTest() {
    this->colliders.set_backend(nngn::Colliders::native_backend());
}

QTEST_MAIN(NativeTest)

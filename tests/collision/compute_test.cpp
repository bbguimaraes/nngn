#include "compute_test.h"

#include "os/platform.h"

ComputeTest::ComputeTest() {
    if(const char *d = std::getenv("srcdir"))
        nngn::Platform::src_dir = std::filesystem::path(d);
    const nngn::Compute::OpenCLParameters params = {true};
    this->compute = nngn::Compute::create(
        nngn::Compute::Backend::OPENCL_BACKEND, &params);
    QVERIFY(this->compute->init());
    this->colliders.set_backend(
        nngn::Colliders::compute_backend(this->compute.get()));
}

QTEST_MAIN(ComputeTest)

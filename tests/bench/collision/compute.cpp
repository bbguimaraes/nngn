#include "compute.h"

#include "os/platform.h"

CollisionComputeBench::CollisionComputeBench() {
    if(const char *d = std::getenv("srcdir"))
        nngn::Platform::src_dir = std::filesystem::path(d);
    nngn::Compute::OpenCLParameters params;
    params.debug = true;
    this->compute = nngn::Compute::create(
        nngn::Compute::Backend::OPENCL_BACKEND, &params);
    QVERIFY(this->compute->init());
}

nngn::Colliders CollisionComputeBench::make_colliders() const {
    nngn::Colliders ret = {};
    ret.set_backend(nngn::Colliders::compute_backend(this->compute.get()));
    return ret;
}

QTEST_MAIN(CollisionComputeBench)

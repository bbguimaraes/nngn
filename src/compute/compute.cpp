#include "compute.h"

#include "utils/log.h"

namespace nngn {

std::unique_ptr<Compute> Compute::create(
        Backend b, const void *params) {
    NNGN_LOG_CONTEXT_CF(Compute);
    switch(b) {
#define C(T) case T: return compute_create_backend<T>(params);
    C(Backend::PSEUDOCOMP)
    C(Backend::OPENCL_BACKEND)
#undef C
    }
    nngn::Log::l() << "invalid backend: " << static_cast<int>(b) << '\n';
    return nullptr;
}

}

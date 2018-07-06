#include <cassert>
#include <chrono>
#include <thread>
#include <vector>

#include "os/platform.h"
#include "utils/log.h"

#include "pseudo.h"

namespace nngn {

bool Pseudograph::write_to_buffer(
    u32, u64, u64 n, u64 size,
    void *data, void f(void*, void*, u64, u64)
) {
    assert(n * size < SIZE_MAX);
    const auto vsize = static_cast<std::size_t>(n * size);
    f(data, std::vector<std::byte>(vsize).data(), 0, n);
    return true;
}

bool Pseudograph::vsync() {
    constexpr auto t = std::chrono::milliseconds(1000) / 60.0f;
    std::this_thread::sleep_for(t);
    return true;
}

}

namespace {

constexpr auto Backend = nngn::Graphics::Backend::PSEUDOGRAPH;

}

namespace nngn {

#ifdef NNGN_PLATFORM_EMSCRIPTEN
template<>
std::unique_ptr<Graphics> graphics_create_backend<Backend>(const void*) {
    nngn::Log::l() << "compiled without pseudograph support\n";
    return {};
}
#else
template<>
std::unique_ptr<Graphics> graphics_create_backend<Backend>(const void *params) {
    if(params) {
        nngn::Log::l() << "no parameters allowed\n";
        return {};
    }
    return std::make_unique<Pseudograph>();
}
#endif

}

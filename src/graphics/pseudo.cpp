#include <chrono>
#include <thread>
#include <vector>

#include "os/platform.h"
#include "utils/log.h"

#include "pseudo.h"

namespace nngn {


bool Pseudograph::render() {
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
std::unique_ptr<Graphics> graphics_create_backend<Backend>() {
    nngn::Log::l() << "compiled without pseudograph support\n";
    return {};
}
#else
template<>
std::unique_ptr<Graphics> graphics_create_backend<Backend>()
    { return std::make_unique<Pseudograph>(); }
#endif

}

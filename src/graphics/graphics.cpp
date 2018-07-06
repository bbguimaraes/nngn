#include "graphics.h"

#include "utils/log.h"

#include "graphics.h"

namespace nngn {

std::unique_ptr<Graphics> Graphics::create(Backend b, const void *params) {
    NNGN_LOG_CONTEXT_CF(Graphics);
    switch(b) {
#define C(T) case T: return graphics_create_backend<T>(params);
    C(Backend::PSEUDOGRAPH)
    C(Backend::GLFW_BACKEND)
#undef C
    }
    Log::l()
        << "invalid backend: "
        << static_cast<std::underlying_type_t<decltype(b)>>(b)
        << std::endl;
    return nullptr;
}

}

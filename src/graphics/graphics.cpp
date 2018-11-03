#include "graphics.h"

#include "utils/log.h"

#include "graphics.h"

namespace nngn {

std::unique_ptr<Graphics> Graphics::create(Backend b, const void *params) {
    NNGN_LOG_CONTEXT_CF(Graphics);
    switch(b) {
#define C(T) case T: return graphics_create_backend<T>(params);
    C(Backend::PSEUDOGRAPH)
    C(Backend::OPENGL_BACKEND)
    C(Backend::OPENGL_ES_BACKEND)
    C(Backend::VULKAN_BACKEND)
#undef C
    }
    Log::l()
        << "invalid backend: "
        << static_cast<std::underlying_type_t<decltype(b)>>(b)
        << std::endl;
    return nullptr;
}

const char *Graphics::enum_str(DeviceInfo::Type t) {
    switch(t) {
#define C(T) case DeviceInfo::Type::T: return #T;
    default:
    C(OTHER)
    C(INTEGRATED_GPU)
    C(DISCRETE_GPU)
    C(VIRTUAL_GPU)
    C(CPU)
#undef C
    }
}

const char *Graphics::enum_str(QueueFamily::Flag f) {
    switch(f) {
#define C(T) case QueueFamily::Flag::T: return #T;
    C(GRAPHICS)
    C(COMPUTE)
    C(TRANSFER)
    C(PRESENT)
    default: return "invalid";
#undef C
    }
}

std::string Graphics::flags_str(QueueFamily::Flag f) {
    using F = QueueFamily::Flag;
    std::stringstream s;
#define C(T) if(f & F::T) s << #T "|";
    C(GRAPHICS)
    C(COMPUTE)
    C(TRANSFER)
    C(PRESENT)
#undef C
    auto ret = s.str();
    ret.erase(end(ret) - 1);
    return ret;
}

const char *Graphics::enum_str(PresentMode m) {
    switch(m) {
#define C(T) case PresentMode::T: return #T;
    C(IMMEDIATE)
    C(MAILBOX)
    C(FIFO)
    C(FIFO_RELAXED)
    default: return "invalid";
#undef C
    }
}

const char *Graphics::enum_str(MemoryHeap::Flag m) {
    switch(m) {
#define C(T) case MemoryHeap::Flag::T: return #T;
    C(DEVICE_LOCAL)
    default: return "invalid";
#undef C
    }
}

std::string Graphics::flags_str(MemoryHeap::Flag f) {
    std::stringstream s;
#define C(T) if(f & MemoryHeap::Flag::T) s << #T "|";
    C(DEVICE_LOCAL)
#undef C
    auto ret = s.str();
    ret.erase(end(ret) - 1);
    return ret;
}

const char *Graphics::enum_str(MemoryType::Flag m) {
    switch(m) {
#define C(T) case MemoryType::Flag::T: return #T;
    C(DEVICE_LOCAL)
    C(HOST_VISIBLE)
    C(HOST_COHERENT)
    C(HOST_CACHED)
    C(LAZILY_ALLOCATED)
    default: return "invalid";
#undef C
    }
}

std::string Graphics::flags_str(MemoryType::Flag f) {
    std::stringstream s;
#define C(T) if(f & MemoryType::Flag::T) s << #T "|";
    C(DEVICE_LOCAL)
    C(HOST_VISIBLE)
    C(HOST_COHERENT)
    C(HOST_CACHED)
    C(LAZILY_ALLOCATED)
#undef C
    auto ret = s.str();
    ret.erase(end(ret) - 1);
    return ret;
}

}

#include "os/platform.h"

#ifdef NNGN_PLATFORM_HAS_VULKAN
#include "memory.h"

static_assert(
    static_cast<std::uint32_t>(nngn::Graphics::MemoryHeap::Flag::DEVICE_LOCAL)
    == VK_MEMORY_HEAP_DEVICE_LOCAL_BIT);
static_assert(
    static_cast<std::uint32_t>(nngn::Graphics::MemoryType::Flag::DEVICE_LOCAL)
    == VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
static_assert(
    static_cast<std::uint32_t>(nngn::Graphics::MemoryType::Flag::HOST_VISIBLE)
    == VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);
static_assert(
    static_cast<std::uint32_t>(nngn::Graphics::MemoryType::Flag::HOST_COHERENT)
    == VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
static_assert(
    static_cast<std::uint32_t>(nngn::Graphics::MemoryType::Flag::HOST_CACHED)
    == VK_MEMORY_PROPERTY_HOST_CACHED_BIT);
static_assert(
    static_cast<std::uint32_t>(
        nngn::Graphics::MemoryType::Flag::LAZILY_ALLOCATED)
    == VK_MEMORY_PROPERTY_LAZILY_ALLOCATED_BIT);

namespace nngn {

void DeviceMemoryInfo::init(VkPhysicalDevice dev) {
    using HF = Graphics::MemoryHeap::Flag;
    using TF = Graphics::MemoryType::Flag;
    VkPhysicalDeviceMemoryProperties props = {};
    vkGetPhysicalDeviceMemoryProperties(dev, &props);
    const auto n = props.memoryHeapCount;
    this->heaps.reserve(n);
    this->memory_types.reserve(n);
    for(std::size_t ih = 0; ih < n; ++ih) {
        const auto &h = props.memoryHeaps[ih];
        this->heaps.push_back({
            .size = h.size,
            .flags = static_cast<HF>(h.flags)});
        auto &types = this->memory_types.emplace_back();
        for(std::size_t it = 0; it < props.memoryTypeCount; ++it) {
            const auto &t = props.memoryTypes[it];
            if(t.heapIndex != ih)
                continue;
            types.push_back({.flags = static_cast<TF>(t.propertyFlags)});
        }
    }
}

}
#endif

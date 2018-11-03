#ifndef NNGN_GRAPHICS_VULKAN_MEMORY_H
#define NNGN_GRAPHICS_VULKAN_MEMORY_H

#include <span>
#include <vector>

#include "graphics/graphics.h"

#include "vulkan.h"

namespace nngn {

/** Aggregate type for information about a device's memory heaps. */
struct DeviceMemoryInfo {
    std::vector<Graphics::MemoryHeap> heaps = {};
    std::vector<std::vector<Graphics::MemoryType>> memory_types = {};
    void init(VkPhysicalDevice dev);
};

}

#endif

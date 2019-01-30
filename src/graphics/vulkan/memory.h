#ifndef NNGN_GRAPHICS_VULKAN_MEMORY_H
#define NNGN_GRAPHICS_VULKAN_MEMORY_H

#include <cstdint>
#include <span>
#include <type_traits>
#include <vector>

#include "graphics/graphics.h"
#include "utils/log.h"

#include "utils.h"
#include "vulkan.h"

namespace nngn {

using MemoryAllocation = std::uint64_t;

/** Manages device memory queries, allocations, and lifetime. */
class DeviceMemory {
public:
    /** Indicates whether a memory type is guaranteed to exist. */
    static constexpr bool required(VkMemoryPropertyFlags f);
    template<VkMemoryPropertyFlags f>
    using find_ret = std::conditional_t<required(f), void, bool>;
    NNGN_MOVE_ONLY(DeviceMemory)
    DeviceMemory(void) = default;
    ~DeviceMemory(void);
    bool init(VkInstance inst, VkPhysicalDevice physical_dev, VkDevice dev);
    /**
     * Finds a memory type among those offered by the device.
     * \returns
     *     If \c f is \ref required, \c void.  Otherwise, returns \c true if a
     *     suitable type was found and written to \c p.
     */
    template<VkMemoryPropertyFlags f>
    find_ret<f> find_type(std::uint32_t type, std::uint32_t *p) const;
    /** Allocates device memory from a type that fulfills \c req. */
    template<VkMemoryPropertyFlags f>
    bool alloc(const VkMemoryRequirements *req, VkDeviceMemory *p);
    /**
     * Allocates device memory from a type that fulfills \c req.
     * Device memory may come from a pool.
     */
    template<VkMemoryPropertyFlags f>
    bool alloc(
        const VkMemoryRequirements *req, MemoryAllocation *alloc,
        VkDeviceMemory *mem, VkDeviceSize *offset);
    /** Allocates device memory from a type for a given buffer. */
    template<VkMemoryPropertyFlags f>
    bool alloc(VkBuffer buf, VkDeviceMemory *p);
    /**
     * Allocates device memory from a type that fulfills \c req.
     * Device memory may come from a pool.
     */
    template<VkMemoryPropertyFlags f>
    bool alloc(
        VkBuffer b, MemoryAllocation *alloc,
        VkDeviceMemory *mem, VkDeviceSize *offset);
    /** Allocates device memory from a type for a given image. */
    template<VkMemoryPropertyFlags f>
    bool alloc(VkImage img, VkDeviceMemory *p);
    /**
     * Allocates device memory from a type for a given image.
     * Device memory may come from a pool.
     */
    template<VkMemoryPropertyFlags f>
    bool alloc(
        VkImage img, MemoryAllocation *alloc,
        VkDeviceMemory *mem, VkDeviceSize *offset);
    /** Deallocates a block of device memory. */
    void dealloc(VkDeviceMemory mem);
    /** Deallocates a block of memory from an allocation pool (if necessary). */
    void dealloc(MemoryAllocation alloc);
private:
    /** Shared implementation for all dedicated allocations. */
    bool alloc(std::uint32_t type, VkDeviceSize size, VkDeviceMemory *p);
    /** Shared implementation for all pooled allocations. */
    bool alloc(
        std::uint32_t type, const VkMemoryRequirements *req,
        MemoryAllocation *alloc, VkDeviceMemory *mem, VkDeviceSize *offset);
    VkDevice dev = {};
    void *allocator = {};
    VkPhysicalDeviceMemoryProperties props = {};
};

/** Aggregate type for information about a device's memory heaps. */
struct DeviceMemoryInfo {
    std::vector<Graphics::MemoryHeap> heaps = {};
    std::vector<std::vector<Graphics::MemoryType>> memory_types = {};
    void init(VkPhysicalDevice dev);
};

constexpr bool DeviceMemory::required(VkMemoryPropertyFlags f) {
    constexpr auto device = VK_MEMORY_HEAP_DEVICE_LOCAL_BIT;
    constexpr auto host = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT
        | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
    return f & device || (f & host) == host;
}

template<VkMemoryPropertyFlags f>
auto DeviceMemory::find_type(
    std::uint32_t type, std::uint32_t *p
) const -> find_ret<f> {
    constexpr auto ret = []([[maybe_unused]] auto b) {
        if constexpr(!DeviceMemory::required(f))
            return b;
    };
    for(std::uint32_t i = 0; i < this->props.memoryTypeCount; ++i) {
        if(!(type & (std::uint32_t{1} << i)))
            continue;
        if((this->props.memoryTypes[i].propertyFlags & f) != f)
            continue;
        *p = i;
        return ret(true);
    }
    NNGN_LOG_CONTEXT_CF(DeviceMemory);
    Log::l() << "no suitable type found\n";
    return ret(false);
}

template<VkMemoryPropertyFlags f>
bool DeviceMemory::alloc(
    const VkMemoryRequirements *req, VkDeviceMemory *p
) {
    std::uint32_t type = {};
    if constexpr(DeviceMemory::required(f))
        this->find_type<f>(req->memoryTypeBits, &type);
    else if(!this->find_type<f>(req->memoryTypeBits, &type))
        return false;
    return this->alloc(type, req->size, p);
}

template<VkMemoryPropertyFlags f>
bool DeviceMemory::alloc(
    const VkMemoryRequirements *req, MemoryAllocation *alloc,
    VkDeviceMemory *mem, VkDeviceSize *offset
) {
    std::uint32_t type = {};
    if constexpr(DeviceMemory::required(f))
        this->find_type<f>(req->memoryTypeBits, &type);
    else if(!this->find_type<f>(req->memoryTypeBits, &type))
        return false;
    return this->alloc(type, req, alloc, mem, offset);
}

template<VkMemoryPropertyFlags f>
bool DeviceMemory::alloc(VkBuffer b, VkDeviceMemory *p) {
    VkMemoryRequirements req;
    vkGetBufferMemoryRequirements(this->dev, b, &req);
    return this->alloc<f>(&req, p);
}

template<VkMemoryPropertyFlags f>
bool DeviceMemory::alloc(
    VkBuffer b, MemoryAllocation *alloc,
    VkDeviceMemory *mem, VkDeviceSize *offset
) {
    VkMemoryRequirements req;
    vkGetBufferMemoryRequirements(this->dev, b, &req);
    return this->alloc<f>(&req, alloc, mem, offset);
}

template<VkMemoryPropertyFlags f>
bool DeviceMemory::alloc(VkImage img, VkDeviceMemory *p) {
    VkMemoryRequirements req;
    vkGetImageMemoryRequirements(this->dev, img, &req);
    return this->alloc<f>(&req, p);
}

template<VkMemoryPropertyFlags f>
bool DeviceMemory::alloc(
    VkImage img, MemoryAllocation *alloc,
    VkDeviceMemory *mem, VkDeviceSize *offset
) {
    VkMemoryRequirements req;
    vkGetImageMemoryRequirements(this->dev, img, &req);
    return this->alloc<f>(&req, alloc, mem, offset);
}

}

#endif

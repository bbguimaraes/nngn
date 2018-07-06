#include "os/platform.h"

#ifdef NNGN_PLATFORM_HAS_VULKAN
#include "memory.h"

#include <cassert>

#include "os/platform.h"

#ifdef NNGN_PLATFORM_HAS_VMA
#define VMA_IMPLEMENTATION
#include <vk_mem_alloc.h>
#endif

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

namespace {

bool init_allocator(
    VkInstance inst, VkPhysicalDevice physical_dev, VkDevice dev, void **p);
void destroy_allocator(void *p);
bool allocate(
    void *p, std::uint32_t type, const VkMemoryRequirements *req,
    nngn::MemoryAllocation *alloc, VkDeviceMemory *mem, VkDeviceSize *offset);
void dealloc(void *p, nngn::MemoryAllocation alloc);

#ifdef NNGN_PLATFORM_HAS_VMA
bool init_allocator(
    VkInstance inst, VkPhysicalDevice physical_dev, VkDevice dev,
    void **pp
) {
    VmaAllocator p = {};
    CHECK_RESULT(
        vmaCreateAllocator,
        nngn::rptr(VmaAllocatorCreateInfo{
            .flags = VMA_ALLOCATOR_CREATE_EXTERNALLY_SYNCHRONIZED_BIT,
            .physicalDevice = physical_dev,
            .device = dev,
            .instance = inst,
        }), &p);
    *pp = p;
    return true;
}

void destroy_allocator(void *p)
    { vmaDestroyAllocator(static_cast<VmaAllocator>(p)); }

bool allocate(
    void *p, std::uint32_t type, const VkMemoryRequirements *req,
    nngn::MemoryAllocation *alloc, VkDeviceMemory *mem, VkDeviceSize *offset
) {
    VmaAllocation a = {};
    VmaAllocationInfo info = {};
    CHECK_RESULT(
        vmaAllocateMemory, static_cast<VmaAllocator>(p), req,
        nngn::rptr(VmaAllocationCreateInfo{.memoryTypeBits = 1u << type}),
        &a, &info);
    *alloc = reinterpret_cast<std::uint64_t>(a);
    *mem = info.deviceMemory;
    *offset = info.offset;
    return true;
}

void dealloc(void *p, nngn::MemoryAllocation alloc) {
    vmaFreeMemory(
        static_cast<VmaAllocator>(p),
        reinterpret_cast<VmaAllocation>(alloc));
}
#else // !NNGN_PLATFORM_HAS_VMA
bool init_allocator(
    VkInstance, VkPhysicalDevice, VkDevice dev, void**p
) {
    *p = dev;
    return true;
}

void destroy_allocator(void*) {}

bool allocate(
    void *p, std::uint32_t type, const VkMemoryRequirements *req,
    nngn::MemoryAllocation *alloc, VkDeviceMemory *mem, VkDeviceSize*
) {
    CHECK_RESULT(
        vkAllocateMemory, static_cast<VkDevice>(p),
        nngn::rptr(VkMemoryAllocateInfo{
            .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
            .allocationSize = req->size,
            .memoryTypeIndex = type,
        }),
        nullptr, mem);
    *alloc = reinterpret_cast<nngn::MemoryAllocation>(mem);
    return true;
}

void dealloc(void *p, nngn::MemoryAllocation alloc)
    { vkFreeMemory(static_cast<VkDevice>(p), VkDeviceMemory(alloc), nullptr); }
#endif // NNGN_PLATFORM_HAS_VMA

}

namespace nngn {

DeviceMemory::~DeviceMemory() {
    NNGN_LOG_CONTEXT_CF(DeviceMemory);
    destroy_allocator(this->allocator);
}

bool DeviceMemory::init(
    VkInstance inst, VkPhysicalDevice physical_dev, VkDevice dev_
) {
    if(!init_allocator(inst, physical_dev, dev_, &this->allocator))
        return false;
    this->dev = dev_;
    vkGetPhysicalDeviceMemoryProperties(physical_dev, &this->props);
    return true;
}

bool DeviceMemory::alloc(
    std::uint32_t type, const VkMemoryRequirements *req,
    MemoryAllocation *alloc, VkDeviceMemory *mem, VkDeviceSize *offset
) {
    return allocate(this->allocator, type, req, alloc, mem, offset);
}

bool DeviceMemory::alloc(
    std::uint32_t type, VkDeviceSize size, VkDeviceMemory *mem
) {
    return LOG_RESULT(
        vkAllocateMemory, this->dev,
        rptr(VkMemoryAllocateInfo{
            .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
            .allocationSize = size,
            .memoryTypeIndex = type,
        }),
        nullptr, mem);
}

void DeviceMemory::dealloc(VkDeviceMemory mem)
    { vkFreeMemory(this->dev, mem, nullptr); }
void DeviceMemory::dealloc(MemoryAllocation alloc)
    { ::dealloc(this->allocator, alloc); }

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

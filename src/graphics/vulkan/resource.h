#ifndef NNGN_GRAPHICS_VULKAN_RESOURCE_H
#define NNGN_GRAPHICS_VULKAN_RESOURCE_H

#include <cassert>
#include <cstddef>
#include <cstdint>
#include <span>

#include "utils/log.h"

#include "memory.h"
#include "vulkan.h"

namespace nngn {

class DeviceMemory;

namespace detail {

/** A buffer and its associated memory allocation. */
class Buffer {
public:
    VkBuffer id() const { return this->h; }
    VkDeviceSize size() const { return this->m_size; }
    VkDeviceSize capacity() const { return this->m_capacity; }
    void set_size(VkDeviceSize s)
        { assert(s <= this->m_capacity); this->m_size = s; }
protected:
    void set_capacity(VkDeviceSize c) { this->m_capacity = c; }
    bool init(VkDevice dev, VkDeviceSize size, VkBufferUsageFlags usage);
    bool alloc();
    void destroy();
    void destroy(VkDevice dev);
private:
    VkBuffer h = {};
    VkDeviceSize m_size = {}, m_capacity = {};
};

}

class DedicatedBuffer : public detail::Buffer {
public:
    VkDeviceMemory mem() const { return this->hm; }
    template<VkMemoryPropertyFlags f>
    bool init(
        VkDevice dev, DeviceMemory *dev_mem,
        VkDeviceSize size, VkBufferUsageFlags usage);
    void destroy(VkDevice dev, DeviceMemory *dev_mem);
    void fill(
        VkDevice dev, VkDeviceSize off, VkDeviceSize n, VkDeviceSize stride,
        std::span<const std::byte> s) const;
private:
    VkDeviceMemory hm = {};
};

class Buffer : public detail::Buffer {
public:
    template<VkMemoryPropertyFlags f>
    bool init(
        VkDevice dev, DeviceMemory *dev_mem,
        VkDeviceSize size, VkBufferUsageFlags usage);
    void destroy(VkDevice dev, DeviceMemory *dev_mem);
private:
    MemoryAllocation ha = {};
};

struct Image {
public:
    VkImage id() const { return this->h; }
    VkDeviceMemory mem() const { return this->hm; }
    void destroy(VkDevice dev, DeviceMemory *dev_mem);
    template<VkMemoryPropertyFlags f>
    bool init(
        VkDevice dev, DeviceMemory *dev_mem,
        VkImageCreateFlags flags, VkImageType type, VkFormat format,
        VkExtent3D extent, std::uint32_t mip_levels, std::uint32_t n_layers,
        VkSampleCountFlagBits n_samples, VkImageTiling tiling,
        VkImageUsageFlags usage);
    bool init_mipmaps(
        VkCommandBuffer cmd,
        VkExtent3D extent, std::uint32_t mip_levels, std::uint32_t base_layer,
        std::uint32_t n_layers) const;
    bool create_view(
        VkDevice dev, VkImageViewType type, VkFormat format,
        VkImageAspectFlags aspect_flags,
        std::uint32_t mip_levels, std::uint32_t base_layer,
        std::uint32_t n_layers, VkImageView *p) const;
    void transition_layout(
        VkCommandBuffer cmd,
        VkPipelineStageFlags src_stage, VkPipelineStageFlags dst_stage,
        VkAccessFlags src_mask, VkAccessFlags dst_mask,
        VkImageLayout src, VkImageLayout dst,
        const VkImageSubresourceRange &range) const;
private:
    bool init(
        VkDevice dev,
        VkImageCreateFlags flags, VkImageType type, VkFormat format,
        VkExtent3D extent, std::uint32_t mip_levels, std::uint32_t n_layers,
        VkSampleCountFlagBits n_samples, VkImageTiling tiling,
        VkImageUsageFlags usage);
    VkImage h = {};
    VkDeviceMemory hm = {};
};

template<VkMemoryPropertyFlags f>
bool DedicatedBuffer::init(
    VkDevice dev, DeviceMemory *dev_mem,
    VkDeviceSize size, VkBufferUsageFlags usage
) {
    NNGN_LOG_CONTEXT_CF(Buffer);
    this->destroy(dev, dev_mem);
    if(!detail::Buffer::init(dev, size, usage))
        return false;
    if(!dev_mem->alloc<f>(this->id(), &this->hm))
        return false;
    vkBindBufferMemory(dev, this->id(), this->hm, 0);
    this->set_capacity(size);
    return true;
}

template<VkMemoryPropertyFlags f>
bool Buffer::init(
    VkDevice dev, DeviceMemory *dev_mem,
    VkDeviceSize size, VkBufferUsageFlags usage
) {
    NNGN_LOG_CONTEXT_CF(Buffer);
    this->destroy(dev, dev_mem);
    if(!detail::Buffer::init(dev, size, usage))
        return false;
    VkDeviceMemory mem = {};
    VkDeviceSize off = {};
    if(!dev_mem->alloc<f>(this->id(), &this->ha, &mem, &off))
        return false;
    vkBindBufferMemory(dev, this->id(), mem, off);
    this->set_capacity(size);
    return true;
}

template<VkMemoryPropertyFlags f>
bool Image::init(
    VkDevice dev, DeviceMemory *dev_mem,
    VkImageCreateFlags flags, VkImageType type, VkFormat format,
    VkExtent3D extent, std::uint32_t mip_levels, std::uint32_t n_layers,
    VkSampleCountFlagBits n_samples, VkImageTiling tiling,
    VkImageUsageFlags usage
) {
    NNGN_LOG_CONTEXT_CF(Image);
    return this->init(
            dev, flags, type, format, extent, mip_levels, n_layers, n_samples,
            tiling, usage)
        && dev_mem->alloc<f>(this->h, &this->hm)
        && (vkBindImageMemory(dev, this->h, this->hm, 0), true);
}

}

#endif

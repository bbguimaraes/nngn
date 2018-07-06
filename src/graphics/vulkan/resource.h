#ifndef NNGN_GRAPHICS_VULKAN_RESOURCE_H
#define NNGN_GRAPHICS_VULKAN_RESOURCE_H

#include <cassert>
#include <cstddef>
#include <cstdint>

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

}

#endif

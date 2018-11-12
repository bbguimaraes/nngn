#include "os/platform.h"

#ifdef NNGN_PLATFORM_HAS_VULKAN
#include "resource.h"

#include <cassert>
#include <utility>

#include "utils/utils.h"

namespace nngn {

namespace detail {

bool Buffer::init(VkDevice dev, VkDeviceSize size, VkBufferUsageFlags usage) {
    assert(!this->h);
    return LOG_RESULT(
        vkCreateBuffer, dev,
        rptr(vk_create_info<VkBuffer>({
            .size = size,
            .usage = usage,
            .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
        })),
        nullptr, &this->h);
}

void Buffer::destroy(void) {
    this->m_size = this->m_capacity = {};
}

void Buffer::destroy(VkDevice dev) {
    vkDestroyBuffer(dev, std::exchange(this->h, {}), nullptr);
}

}

void DedicatedBuffer::fill(
    VkDevice dev, VkDeviceSize off, VkDeviceSize n, VkDeviceSize stride,
    std::span<const std::byte> s
) const {
    assert(stride <= s.size());
    assert(off + n * stride <= this->capacity());
    void *vp = {};
    vkMapMemory(dev, this->hm, off, s.size(), 0, &vp);
    auto p = static_cast<std::byte*>(vp);
    for(auto *const e = p + n * s.size(); p < e; p += stride)
        std::memcpy(p, s.data(), s.size());
    vkUnmapMemory(dev, this->hm);
}

void DedicatedBuffer::destroy(
    VkDevice dev, nngn::DeviceMemory *dev_mem
) {
    detail::Buffer::destroy(dev);
    if(this->hm)
        dev_mem->dealloc(std::exchange(this->hm, {}));
}

void Buffer::destroy(
    VkDevice dev, DeviceMemory *dev_mem
) {
    detail::Buffer::destroy(dev);
    dev_mem->dealloc(this->ha);
}

}
#endif

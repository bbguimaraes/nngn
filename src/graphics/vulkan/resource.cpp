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

void Image::destroy(VkDevice dev, DeviceMemory *dev_mem) {
    vkDestroyImage(dev, std::exchange(this->h, {}), nullptr);
    if(this->hm)
        dev_mem->dealloc(std::exchange(this->hm, {}));
}

bool Image::init(
    VkDevice dev,
    VkImageCreateFlags flags, VkImageType type, VkFormat format,
    VkExtent3D extent, std::uint32_t mip_levels, std::uint32_t n_layers,
    VkSampleCountFlagBits n_samples, VkImageTiling tiling,
    VkImageUsageFlags usage
) {
    assert(!this->h);
    return LOG_RESULT(
        vkCreateImage, dev,
        rptr(vk_create_info<VkImage>({
            .flags = flags,
            .imageType = type,
            .format = format,
            .extent = extent,
            .mipLevels = mip_levels,
            .arrayLayers = n_layers,
            .samples = n_samples,
            .tiling = tiling,
            .usage = usage,
            .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
            .queueFamilyIndexCount = 0,
            .pQueueFamilyIndices = {},
            .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
        })),
        nullptr, &this->h);
}

bool Image::create_view(
    VkDevice dev, VkImageViewType type, VkFormat format,
    VkImageAspectFlags aspect_flags,
    std::uint32_t mip_levels, std::uint32_t base_layer,
    std::uint32_t n_layers, VkImageView *p
) const {
    NNGN_LOG_CONTEXT_CF(Image);
    return LOG_RESULT(
        vkCreateImageView, dev,
        rptr(vk_create_info<VkImageView>({
            .image = this->h,
            .viewType = type,
            .format = format,
            .subresourceRange = {
                .aspectMask = aspect_flags,
                .levelCount = mip_levels,
                .baseArrayLayer = base_layer,
                .layerCount = n_layers,
            },
        })),
        nullptr, p);
}

}
#endif

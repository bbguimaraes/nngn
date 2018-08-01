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

bool Image::init_mipmaps(
    VkCommandBuffer cmd,
    VkExtent3D extent, std::uint32_t mip_levels, std::uint32_t base_layer,
    std::uint32_t n_layers
) const {
    NNGN_LOG_CONTEXT_CF(Image);
    auto img = this->h;
    const auto barrier = [cmd, img, base_layer, n_layers](
        auto mip_level, auto dst_stage,
        auto src_access, auto dst_access,
        auto src_layout, auto dst_layout
    ) {
        vkCmdPipelineBarrier(cmd,
            VK_PIPELINE_STAGE_TRANSFER_BIT,
            dst_stage,
            0, 0, nullptr, 0, nullptr, 1,
            rptr(VkImageMemoryBarrier{
                .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
                .srcAccessMask = src_access,
                .dstAccessMask = dst_access,
                .oldLayout = src_layout,
                .newLayout = dst_layout,
                .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
                .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
                .image = img,
                .subresourceRange = {
                    .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                    .baseMipLevel = mip_level,
                    .levelCount = 1,
                    .baseArrayLayer = base_layer,
                    .layerCount = n_layers,
                },
            }));
    };
    auto src_width = static_cast<std::int32_t>(extent.width);
    auto src_height = static_cast<std::int32_t>(extent.height);
    for(std::uint32_t i = 1; i < mip_levels; ++i) {
        barrier(
            i - 1,
            VK_PIPELINE_STAGE_TRANSFER_BIT,
            VK_ACCESS_TRANSFER_WRITE_BIT,
            VK_ACCESS_TRANSFER_READ_BIT,
            VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
            VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);
        auto dst_width = std::max(src_width / 2, std::int32_t{1});
        auto dst_height = std::max(src_height / 2, std::int32_t{1});
        vkCmdBlitImage(cmd,
            img, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
            img, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1,
            nngn::rptr(VkImageBlit{
                .srcSubresource = {
                    .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                    .mipLevel = i - 1,
                    .baseArrayLayer = base_layer,
                    .layerCount = n_layers,
                },
                .srcOffsets = {{}, {src_width, src_height, 1}},
                .dstSubresource = {
                    .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                    .mipLevel = i,
                    .baseArrayLayer = base_layer,
                    .layerCount = n_layers,
                },
                .dstOffsets = {{}, {dst_width, dst_height, 1}},
            }),
            VK_FILTER_NEAREST);
        barrier(
            i - 1,
            VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
            VK_ACCESS_TRANSFER_READ_BIT,
            VK_ACCESS_SHADER_READ_BIT,
            VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
            VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
        src_width = dst_width;
        src_height = dst_height;
    }
    barrier(
        mip_levels - 1,
        VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
        VK_ACCESS_TRANSFER_WRITE_BIT,
        VK_ACCESS_SHADER_READ_BIT,
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
    return true;
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

void Image::transition_layout(
    VkCommandBuffer cmd,
    VkPipelineStageFlags src_stage, VkPipelineStageFlags dst_stage,
    VkAccessFlags src_mask, VkAccessFlags dst_mask,
    VkImageLayout src, VkImageLayout dst,
    const VkImageSubresourceRange &range
) const {
    NNGN_LOG_CONTEXT_CF(Image);
    vkCmdPipelineBarrier(
        cmd, src_stage, dst_stage, 0, 0, nullptr, 0, nullptr, 1,
        rptr(VkImageMemoryBarrier{
            .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
            .srcAccessMask = src_mask,
            .dstAccessMask = dst_mask,
            .oldLayout = src,
            .newLayout = dst,
            .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
            .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
            .image = this->h,
            .subresourceRange = range,
        }));
}

}
#endif

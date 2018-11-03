#ifndef NNGN_GRAPHICS_VULKAN_TYPES_H
#define NNGN_GRAPHICS_VULKAN_TYPES_H

#include "os/platform.h"

#ifdef NNGN_PLATFORM_HAS_VULKAN
#include "utils/macros.h"
#include "utils/utils.h"

#include "vulkan.h"

namespace nngn {

/**
 * X-macro that operates on Vulkan types.
 * Fields: Vulkan type, enum name fragment, create info function.
 */
#define NNGN_VK_TYPES(f) \
    f(VkBuffer, BUFFER, NNGN_APPLY2) \
    f(VkCommandBuffer, COMMAND_BUFFER, NNGN_IGNORE2) \
    f(VkCommandPool, COMMAND_POOL, NNGN_APPLY2) \
    f(VkDescriptorPool, DESCRIPTOR_POOL, NNGN_APPLY2) \
    f(VkDescriptorSet, DESCRIPTOR_SET, NNGN_IGNORE2) \
    f(VkDescriptorSetLayout, DESCRIPTOR_SET_LAYOUT, NNGN_APPLY2) \
    f(VkDeviceMemory, DEVICE_MEMORY, NNGN_IGNORE2) \
    f(VkFence, FENCE, NNGN_APPLY2) \
    f(VkFramebuffer, FRAMEBUFFER, NNGN_APPLY2) \
    f(VkImage, IMAGE, NNGN_APPLY2) \
    f(VkImageView, IMAGE_VIEW, NNGN_APPLY2) \
    f(VkPipeline, PIPELINE, NNGN_IGNORE2) \
    f(VkQueue, QUEUE, NNGN_IGNORE2) \
    f(VkPipelineCache, PIPELINE_CACHE, NNGN_APPLY2) \
    f(VkPipelineLayout, PIPELINE_LAYOUT, NNGN_APPLY2) \
    f(VkRenderPass, RENDER_PASS, NNGN_APPLY2) \
    f(VkSampler, SAMPLER, NNGN_APPLY2) \
    f(VkSemaphore, SEMAPHORE, NNGN_APPLY2) \
    f(VkShaderModule, SHADER_MODULE, NNGN_APPLY2)

namespace detail {

/* Implements \ref vk_create_info_type. */
template<typename T> struct vk_create_info_type_impl;

}

/** Maps types to *CretaeInfo structures. */
template<typename T> using vk_create_info_type =
    typename detail::vk_create_info_type_impl<T>::type;

/** Maps types to VkStructureType values. */
template<typename T> inline constexpr empty vk_struct_type = {};

/** Maps types to VkObjectType values. */
template<typename T> inline constexpr empty vk_obj_type = {};

namespace detail {

#define C(x, _) \
    template<> struct vk_create_info_type_impl<x> \
        { using type = x ## CreateInfo; };
#define X(x, n, c) c(C, x, n)
NNGN_VK_TYPES(X)
#undef X
#undef C

}

#define C(x, n) \
    template<> inline constexpr auto vk_struct_type<x> = \
        VK_STRUCTURE_TYPE_ ## n ## _CREATE_INFO;
#define X(x, n, c) c(C, x, n)
NNGN_VK_TYPES(X)
#undef X
#undef C

#define X(x, n, _) \
    template<> inline constexpr auto vk_obj_type<x> = \
        VK_OBJECT_TYPE_ ## n;
NNGN_VK_TYPES(X)
#undef X

}

#undef NNGN_VK_TYPES

#endif

#endif

#include "os/platform.h"

#ifdef NNGN_PLATFORM_HAS_VULKAN
#include "utils.h"

#include <algorithm>
#include <cassert>
#include <charconv>
#ifndef __clang__
#include <ranges>
#endif

namespace nngn {

const char *vk_strerror(VkResult result) {
    switch(result) {
#define C(x) case x: return #x;
    C(VK_SUCCESS)
    C(VK_NOT_READY)
    C(VK_TIMEOUT)
    C(VK_EVENT_SET)
    C(VK_EVENT_RESET)
    C(VK_INCOMPLETE)
    C(VK_ERROR_OUT_OF_HOST_MEMORY)
    C(VK_ERROR_OUT_OF_DEVICE_MEMORY)
    C(VK_ERROR_INITIALIZATION_FAILED)
    C(VK_ERROR_DEVICE_LOST)
    C(VK_ERROR_MEMORY_MAP_FAILED)
    C(VK_ERROR_LAYER_NOT_PRESENT)
    C(VK_ERROR_EXTENSION_NOT_PRESENT)
    C(VK_ERROR_FEATURE_NOT_PRESENT)
    C(VK_ERROR_INCOMPATIBLE_DRIVER)
    C(VK_ERROR_TOO_MANY_OBJECTS)
    C(VK_ERROR_FORMAT_NOT_SUPPORTED)
    C(VK_ERROR_FRAGMENTED_POOL)
#ifdef VK_VERSION_1_2
    C(VK_ERROR_UNKNOWN)
#endif
#ifdef VK_VERSION_1_1
    C(VK_ERROR_OUT_OF_POOL_MEMORY)
    C(VK_ERROR_INVALID_EXTERNAL_HANDLE)
#endif
#ifdef VK_EXT_descriptor_indexing
    C(VK_ERROR_FRAGMENTATION_EXT)
#endif
#ifdef VK_VERSION_1_2
    C(VK_ERROR_INVALID_OPAQUE_CAPTURE_ADDRESS)
#endif
#ifdef VK_KHR_surface
    C(VK_ERROR_SURFACE_LOST_KHR)
    C(VK_ERROR_NATIVE_WINDOW_IN_USE_KHR)
#endif
#ifdef VK_KHR_swapchain
    C(VK_SUBOPTIMAL_KHR)
    C(VK_ERROR_OUT_OF_DATE_KHR)
#endif
#ifdef VK_KHR_display_swapchain
    C(VK_ERROR_INCOMPATIBLE_DISPLAY_KHR)
#endif
#ifdef VK_EXT_debug_report
    C(VK_ERROR_VALIDATION_FAILED_EXT)
#endif
#ifdef VK_NV_glsl_shader
    C(VK_ERROR_INVALID_SHADER_NV)
#endif
#if 135 <= VK_HEADER_VERSION && VK_HEADER_VERSION <= 161
    C(VK_ERROR_INCOMPATIBLE_VERSION_KHR)
#endif
#ifdef VK_EXT_image_drm_format_modifier
    C(VK_ERROR_INVALID_DRM_FORMAT_MODIFIER_PLANE_LAYOUT_EXT)
#endif
#ifdef VK_EXT_global_priority
    C(VK_ERROR_NOT_PERMITTED_EXT)
#endif
#if VK_HEADER_VERSION >= 105
    C(VK_ERROR_FULL_SCREEN_EXCLUSIVE_MODE_LOST_EXT)
#endif
#if VK_HEADER_VERSION >= 135
    C(VK_THREAD_IDLE_KHR)
    C(VK_THREAD_DONE_KHR)
    C(VK_OPERATION_DEFERRED_KHR)
    C(VK_OPERATION_NOT_DEFERRED_KHR)
#if defined(VK_EXT_buffer_device_address) && !defined(VK_VERSION_1_2)
    C(VK_ERROR_INVALID_DEVICE_ADDRESS_EXT)
#endif
    C(VK_ERROR_PIPELINE_COMPILE_REQUIRED_EXT)
#endif
#if !defined(VK_VERSION_1_2) || VK_HEADER_VERSION < 140
    C(VK_RESULT_RANGE_SIZE)
#endif
    C(VK_RESULT_MAX_ENUM)
    default: return "unknown";
    }
}

const char *vk_enum_str(VkDebugUtilsMessageSeverityFlagBitsEXT f) {
    switch(f) {
    case VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT: return "verbose";
    case VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT: return "info";
    case VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT: return "warning";
    case VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT: return "error";
    case VK_DEBUG_UTILS_MESSAGE_SEVERITY_FLAG_BITS_MAX_ENUM_EXT:
    default: return "unknown";
    }
}

std::string vk_version_str(Graphics::Version v) {
    constexpr std::size_t l255 = 3, dot = 1, max = 3 * l255 + 2 * dot;
    std::array<char, max> ret = {};
    auto *p = ret.data();
    auto res = std::to_chars(p, p + l255, static_cast<std::uint8_t>(v.major));
    assert(res.ec == std::errc{});
    p = res.ptr;
    *p++ = '.';
    res = std::to_chars(p, p + l255, static_cast<std::uint8_t>(v.minor));
    assert(res.ec == std::errc{});
    p = res.ptr;
    *p++ = '.';
    res = std::to_chars(p, p + l255, static_cast<std::uint8_t>(v.patch));
    assert(res.ec == std::errc{});
    return {ret.data(), res.ptr};
}

std::string vk_version_str(std::uint32_t v) {
    return vk_version_str(Graphics::Version{
        .major = VK_VERSION_MAJOR(v),
        .minor = VK_VERSION_MINOR(v),
        .patch = VK_VERSION_PATCH(v),
        .name = {}});
}

Graphics::Version vk_parse_version(std::uint32_t v) {
    return {VK_VERSION_MAJOR(v), VK_VERSION_MINOR(v), VK_VERSION_PATCH(v), ""};
}

const char *vk_enum_str(VkDebugUtilsMessageTypeFlagsEXT f) {
    switch(f) {
    case VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT: return "general";
    case VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT: return "validation";
    case VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT: return "performance";
    case VK_DEBUG_UTILS_MESSAGE_TYPE_FLAG_BITS_MAX_ENUM_EXT:
    default: return "unknown";
    }
}

std::vector<Graphics::Extension> vk_parse_extensions(
    std::span<const VkExtensionProperties> s
) {
    std::vector<Graphics::Extension> ret;
    ret.reserve(s.size());
    for(const auto &x : s) {
        auto &e = ret.emplace_back(Graphics::Extension{
            .version = x.specVersion,
        });
        std::strcpy(e.name.data(), x.extensionName);
    }
    std::ranges::sort(ret, [](const auto &lhs, const auto &rhs) {
        return str_less(lhs.name.data(), rhs.name.data());
    });
    return ret;
}

Graphics::PresentMode vk_present_mode(VkPresentModeKHR m) {
    switch(m) {
    case VK_PRESENT_MODE_IMMEDIATE_KHR:
        return Graphics::PresentMode::IMMEDIATE;
    case VK_PRESENT_MODE_MAILBOX_KHR:
        return Graphics::PresentMode::MAILBOX;
    case VK_PRESENT_MODE_FIFO_KHR:
        return Graphics::PresentMode::FIFO;
    case VK_PRESENT_MODE_FIFO_RELAXED_KHR:
        return Graphics::PresentMode::FIFO_RELAXED;
    case VK_PRESENT_MODE_SHARED_DEMAND_REFRESH_KHR:
    case VK_PRESENT_MODE_SHARED_CONTINUOUS_REFRESH_KHR:
    case VK_PRESENT_MODE_MAX_ENUM_KHR:
    default:
        NNGN_LOG_CONTEXT_F();
        Log::l()
            << "ignoring present mode "
            << static_cast<std::ptrdiff_t>(m) << '\n';
        return {};
    }
}

}
#endif

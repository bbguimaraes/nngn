#ifndef NNGN_GRAPHICS_VULKAN_UTILS_H
#define NNGN_GRAPHICS_VULKAN_UTILS_H

#include <algorithm>
#include <cstdint>
#ifndef __clang__
#include <ranges>
#endif
#include <span>
#include <string>
#include <vector>

#include "graphics/graphics.h"
#include "utils/concepts.h"
#include "utils/fn.h"
#include "utils/log.h"

#include "types.h"

#ifdef __clang__
#define LIKELY
#define UNLIKELY
#else
#define LIKELY [[likely]]
#define UNLIKELY [[likely]]
#endif

#define LOG_RESULT(f, ...) nngn::vk_check_result(#f, f(__VA_ARGS__))
#define CHECK_RESULT(f, ...) \
    do { if(!LOG_RESULT(f, __VA_ARGS__)) UNLIKELY return false; } while(0);

namespace nngn {

const char *vk_strerror(VkResult result);
const char *vk_enum_str(VkDebugUtilsMessageSeverityFlagBitsEXT f);
const char *vk_enum_str(VkDebugUtilsMessageTypeFlagsEXT f);

/** Unpacks a Vulkan version number into a character array. */
std::string vk_version_str(std::uint32_t v);
std::string vk_version_str(Graphics::Version v);

/** Unpacks a Vulkan version number. */
Graphics::Version vk_parse_version(std::uint32_t v);

template<typename T>
vk_create_info_type<T> vk_create_info(vk_create_info_type<T> info);

inline nngn::uvec2 vk_extent_to_vec(VkExtent2D e)
    { return {e.width, e.height}; }
inline VkExtent2D vk_vec_to_extent(nngn::uvec2 v)
    { return {.width = v.x, .height = v.y}; }

/**
 * Checks that \c r is \c VK_SUCCESS or emits a log message.
 * \return <tt>c == VK_SUCCESS</tt>
 */
bool vk_check_result(const char *func, VkResult r);

/**
 * Creates a vector from the result of calling \c f with \c args.
 * Simplifies the usual two-step enumeration sequence of calls when dynamic
 * allocation is desired.
 */
template<c_function_pointer auto f>
auto vk_enumerate(const auto &...args);

/** Populates \ref Graphics::Extension objects from their Vulkan equivalents. */
std::vector<Graphics::Extension> vk_parse_extensions(
    std::span<const VkExtensionProperties> s);

/** Populates a \ref Graphics::PresentMode from its Vulkan equivalent. */
Graphics::PresentMode vk_present_mode(VkPresentModeKHR m);

template<typename T, auto ...Ps>
constexpr auto vk_vertex_input(
    std::span<const VkVertexInputBindingDescription> bindings);

template<typename T>
concept VkNamedRange = requires {
    std::ranges::sized_range<T>;
    {std::declval<std::ranges::range_value_t<T>>().name.data()}
        -> std::same_as<char*>;
};

/** Extracts the \c name character array from common objects. */
std::vector<const char*> vk_names(const VkNamedRange auto &r);

template<typename T>
vk_create_info_type<T> vk_create_info(vk_create_info_type<T> info) {
    vk_create_info_type<T> ret = info;
    ret.sType = vk_struct_type<T>;
    return ret;
}

inline bool vk_check_result(const char *func, VkResult r) {
    if(r == VK_SUCCESS) LIKELY
        return true;
    Log::l() << func << ": " << vk_strerror(r) << '\n';
    return false;
}

template<c_function_pointer auto f>
auto vk_enumerate(const auto &...args) {
    u32 n = 0;
    f(args..., &n, nullptr);
    std::vector<std::remove_pointer_t<types_last_t<fn_args<f>>>> ret(n);
    f(args..., &n, ret.data());
    return ret;
}

std::vector<const char*> vk_names(const VkNamedRange auto &r) {
    std::vector<const char*> ret = {};
    ret.reserve(r.size());
    for(const auto &x : r)
        ret.push_back(x.name.data());
    return ret;
}

template<standard_layout T, auto ...Ps>
auto vk_vertex_attrs() {
    constexpr auto n = sizeof...(Ps);
    const std::array off = {static_cast<u32>(offsetof_ptr(Ps))...};
    std::array<VkVertexInputAttributeDescription, n> ret = {};
    for(std::size_t i = 0; i < n; ++i)
        ret[i] = {
            .location = static_cast<u32>(i),
            .binding = 0,
            .format = VK_FORMAT_R32G32B32_SFLOAT,
            .offset = off[i],
        };
    return ret;
}

constexpr VkPipelineVertexInputStateCreateInfo vk_vertex_input(
    std::span<const VkVertexInputBindingDescription> bindings,
    std::span<const VkVertexInputAttributeDescription> attrs
) {
    return {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
        .vertexBindingDescriptionCount =
            static_cast<std::uint32_t>(bindings.size()),
        .pVertexBindingDescriptions = bindings.data(),
        .vertexAttributeDescriptionCount =
            static_cast<std::uint32_t>(attrs.size()),
        .pVertexAttributeDescriptions = attrs.data(),
    };
}

}

#endif

#include "os/platform.h"

#ifdef NNGN_PLATFORM_HAS_VULKAN
#include "device.h"

#include <algorithm>
#include <cassert>
#ifndef __clang__
#include <ranges>
#endif

#include "utils/log.h"

#include "instance.h"
#include "utils.h"

using namespace std::string_view_literals;

static_assert(
    VK_MAX_PHYSICAL_DEVICE_NAME_SIZE
    <= std::tuple_size<decltype(nngn::Graphics::DeviceInfo::name)>{});

static_assert(
    static_cast<std::uint32_t>(nngn::Graphics::QueueFamily::Flag::GRAPHICS)
    == VK_QUEUE_GRAPHICS_BIT);
static_assert(
    static_cast<std::uint32_t>(nngn::Graphics::QueueFamily::Flag::COMPUTE)
    == VK_QUEUE_COMPUTE_BIT);
static_assert(
    static_cast<std::uint32_t>(nngn::Graphics::QueueFamily::Flag::TRANSFER)
    == VK_QUEUE_TRANSFER_BIT);

namespace nngn {

Device::~Device() {
    NNGN_LOG_CONTEXT_CF(Device);
    vkDestroyDevice(this->h, nullptr);
}

bool Device::init(
    const Instance &inst, VkPhysicalDevice physical_dev,
    std::uint32_t graphics_family, std::uint32_t present_family,
    std::span<const char *const> extensions,
    std::span<const char *const> layers,
    const VkPhysicalDeviceFeatures &features)
{
    NNGN_LOG_CONTEXT_CF(Device);
    std::array v = {graphics_family, present_family};
    std::ranges::sort(v);
    // TODO https://bugs.llvm.org/show_bug.cgi?id=44833
    //const auto n = std::ranges::unique(v).size();
    const auto n = static_cast<std::size_t>(
        std::distance(begin(v), std::unique(begin(v), end(v))));
    constexpr auto priority = 1.0f;
    std::array<VkDeviceQueueCreateInfo, v.size()> queue_infos = {};
    for(std::size_t i = 0; i < n; ++i)
        queue_infos[i] = {
            .sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
            .pNext = {},
            .flags = {},
            .queueFamilyIndex = v[i],
            .queueCount = 1,
            .pQueuePriorities = &priority};
    const bool ok = LOG_RESULT(
        vkCreateDevice, physical_dev,
        rptr(VkDeviceCreateInfo{
            .sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
            .pNext = {},
            .flags = {},
            .queueCreateInfoCount = static_cast<std::uint32_t>(n),
            .pQueueCreateInfos = queue_infos.data(),
            .enabledLayerCount = static_cast<std::uint32_t>(layers.size()),
            .ppEnabledLayerNames = layers.data(),
            .enabledExtensionCount =
                static_cast<std::uint32_t>(extensions.size()),
            .ppEnabledExtensionNames = extensions.data(),
            .pEnabledFeatures = &features}),
        nullptr, &this->h);
    if(!ok)
        return false;
    this->ph = physical_dev;
    this->families.graphics = graphics_family;
    this->families.present = present_family;
    VkQueue gq = {}, pq = {};
    vkGetDeviceQueue(this->h, graphics_family, 0, &gq);
    vkGetDeviceQueue(this->h, present_family, 0, &pq);
    if(n == 1) {
        if(!inst.set_obj_name(this->h, gq, "graphics_present_queue"sv))
            return false;
    } else if(!inst.set_obj_name(this->h, gq, "graphics_queue"sv)
            || !inst.set_obj_name(this->h, pq, "present_queue"sv))
        return false;
    this->queues.graphics = gq;
    this->queues.present = pq;
    return true;
}

VkShaderModule Device::create_shader(
    const Instance &inst,
    std::string_view name, std::span<const std::uint8_t> src
) const {
    NNGN_LOG_CONTEXT_CF(Device);
    NNGN_LOG_CONTEXT(name.data());
    VkShaderModule m = {};
    const bool ok = LOG_RESULT(
        vkCreateShaderModule, this->h,
        rptr(VkShaderModuleCreateInfo{
            .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
            .pNext = {},
            .flags = {},
            .codeSize = src.size(),
            .pCode = reinterpret_cast<const std::uint32_t*>(src.data())}),
        nullptr, &m);
    if(!ok || !inst.set_obj_name(this->h, m, name))
        return {};
    return m;
}

void DeviceInfo::init(VkSurfaceKHR s, VkPhysicalDevice d) {
    this->dev = d;
    VkPhysicalDeviceProperties p = {};
    vkGetPhysicalDeviceProperties(d, &p);
    std::strcpy(this->info.name.data(), p.deviceName);
    std::strcpy(
        this->info.version.data(),
        vk_version_str(p.apiVersion).c_str());
    this->info.driver_version = p.driverVersion;
    this->info.vendor_id = p.vendorID;
    this->info.device_id = p.deviceID;
    switch(p.deviceType) {
    default:
    case VK_PHYSICAL_DEVICE_TYPE_MAX_ENUM:
    case VK_PHYSICAL_DEVICE_TYPE_OTHER:
        this->info.type = Graphics::DeviceInfo::Type::OTHER; break;
    case VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU:
        this->info.type = Graphics::DeviceInfo::Type::INTEGRATED_GPU; break;
    case VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU:
        this->info.type = Graphics::DeviceInfo::Type::DISCRETE_GPU; break;
    case VK_PHYSICAL_DEVICE_TYPE_VIRTUAL_GPU:
        this->info.type = Graphics::DeviceInfo::Type::VIRTUAL_GPU; break;
    case VK_PHYSICAL_DEVICE_TYPE_CPU:
        this->info.type = Graphics::DeviceInfo::Type::CPU; break;
    }
    this->extensions = vk_parse_extensions(
        vk_enumerate<vkEnumerateDeviceExtensionProperties>(d, nullptr));
    const auto fam = vk_enumerate<vkGetPhysicalDeviceQueueFamilyProperties>(d);
    this->queue_families.reserve(fam.size());
    for(std::size_t i = 0, n = fam.size(); i < n; ++i) {
        using F = Graphics::QueueFamily::Flag;
        const auto &f = fam[i];
        auto flags = static_cast<F>(f.queueFlags);
        if(this->can_present(s, i))
            flags = static_cast<F>(flags | F::PRESENT);
        this->queue_families.push_back({.count = f.queueCount, .flags = flags});
    }
}

std::tuple<bool, std::uint32_t, std::uint32_t> DeviceInfo::find_queues() const {
    using F = Graphics::QueueFamily::Flag;
    constexpr auto both = static_cast<F>(F::GRAPHICS | F::PRESENT);
    const auto pred = [](F f)
        { return [f](const auto &x) { return x.flags & f; }; };
    auto &v = this->queue_families;
    const auto i = std::find_if(
        begin(v), end(v), pred(both));
    if(i != end(v)) {
        const auto ret = static_cast<std::uint32_t>(i - begin(v));
        return {true, ret, ret};
    }
    const auto gi = std::find_if(begin(v), end(v), pred(F::GRAPHICS));
    if(gi == end(v))
        return {false, {}, {}};
    const auto pi = std::find_if(begin(v), end(v), pred(F::PRESENT));
    if(pi == end(v))
        return {false, {}, {}};
    return {
        true,
        static_cast<std::uint32_t>(gi - begin(v)),
        static_cast<std::uint32_t>(pi - begin(v))};
}

bool DeviceInfo::can_present(
    VkSurfaceKHR surface, std::size_t queue_family
) const {
    VkBool32 ret = {};
    return LOG_RESULT(
            vkGetPhysicalDeviceSurfaceSupportKHR,
            this->dev, static_cast<std::uint32_t>(queue_family),
            surface, &ret)
        && ret;
}

}
#endif

#include "os/platform.h"

#ifdef NNGN_PLATFORM_HAS_VULKAN
#include "instance.h"

#include <algorithm>
#include <sstream>

#include "utils/log.h"

#include "utils.h"

#define GET_PROC_ADDR(i, x) (i).get_proc_addr<PFN_ ## x>(#x)

static_assert(
    VK_MAX_EXTENSION_NAME_SIZE
    <= std::tuple_size<decltype(nngn::Graphics::Extension::name)>{});
static_assert(
    VK_MAX_EXTENSION_NAME_SIZE
    <= std::tuple_size<decltype(nngn::Graphics::Layer::name)>{});
static_assert(
    VK_MAX_DESCRIPTION_SIZE
    <= std::tuple_size<decltype(nngn::Graphics::Layer::description)>{});

namespace {

template<typename T>
bool named_cmp(const T &lhs, const T &rhs)
    { return nngn::str_less(lhs.name.data(), rhs.name.data()); }

auto app_info(nngn::Graphics::Version v) {
     return VkApplicationInfo{
        .sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
        .pNext = {},
        .pApplicationName = {},
        .applicationVersion = {},
        .pEngineName = {},
        .engineVersion = {},
        .apiVersion = VK_MAKE_VERSION(v.major, v.minor, v.patch)};
}

VKAPI_ATTR VkBool32 VKAPI_CALL debug_callback(
    VkDebugUtilsMessageSeverityFlagBitsEXT severity,
    VkDebugUtilsMessageTypeFlagsEXT type,
    const auto *data, void *p
) {
    nngn::Log::l()
        << "vulkan_debug_callback("
        << nngn::vk_enum_str(severity)
        << ", " << nngn::vk_enum_str(type)
        << "): " << data->pMessage << '\n';
    if(severity >= VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT)
        static_cast<nngn::Instance*>(p)->set_error();
    return VK_FALSE;
}

VkDebugUtilsMessengerCreateInfoEXT debug_info(
    nngn::Instance *i, nngn::Graphics::LogLevel log_level
) {
    VkDebugUtilsMessengerCreateInfoEXT info = {};
    info.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
    switch(log_level) {
        case nngn::Graphics::LogLevel::DEBUG:
        info.messageSeverity |= VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT;
        info.messageSeverity |= VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT;
        [[fallthrough]];
        case nngn::Graphics::LogLevel::WARNING:
        info.messageSeverity |= VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT;
        [[fallthrough]];
    case nngn::Graphics::LogLevel::ERROR:
        info.messageSeverity |=
            VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
    }
    info.messageType =
        VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT
        | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT
        | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
    info.pUserData = i;
    info.pfnUserCallback = debug_callback;
    return info;
}

}

namespace nngn {

Instance::~Instance() {
    NNGN_LOG_CONTEXT_CF(Instance);
    if(this->messenger)
        if(const auto f = GET_PROC_ADDR(*this, vkDestroyDebugUtilsMessengerEXT))
            f(this->h, this->messenger, nullptr);
    vkDestroyInstance(this->h, nullptr);
}

bool Instance::init(
    Graphics::Version version,
    std::span<const char *const> extensions
) {
    NNGN_LOG_CONTEXT_CF(Instance);
    const auto app = app_info(version);
    return LOG_RESULT(
        vkCreateInstance,
        rptr(VkInstanceCreateInfo{
            .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
            .pNext = {},
            .flags = {},
            .pApplicationInfo = &app,
            .enabledLayerCount = 0,
            .ppEnabledLayerNames = {},
            .enabledExtensionCount =
                static_cast<std::uint32_t>(extensions.size()),
            .ppEnabledExtensionNames = extensions.data()}),
        nullptr, &this->h);
}

bool Instance::init_debug(
    Graphics::Version version,
    std::span<const char *const> extensions,
    std::span<const char *const> layers,
    ErrorFn error_fn_, void *error_data_, Graphics::LogLevel log_level
) {
    NNGN_LOG_CONTEXT_CF(Instance);
    this->error_fn = error_fn_;
    this->error_data = error_data_;
    const auto app = app_info(version);
    const auto dbg = debug_info(this, log_level);
    std::vector<const char*> ext;
    ext.reserve(extensions.size() + 1);
    ext.insert(begin(ext), cbegin(extensions), cend(extensions));
    ext.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
    const bool ok = LOG_RESULT(
        vkCreateInstance,
        rptr(VkInstanceCreateInfo{
            .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
            .pNext = &dbg,
            .flags = {},
            .pApplicationInfo = &app,
            .enabledLayerCount = static_cast<std::uint32_t>(layers.size()),
            .ppEnabledLayerNames = layers.data(),
            .enabledExtensionCount = static_cast<std::uint32_t>(ext.size()),
            .ppEnabledExtensionNames = ext.data()}),
        nullptr, &this->h);
    if(!ok)
        return false;
    const auto f = GET_PROC_ADDR(*this, vkCreateDebugUtilsMessengerEXT);
    return f
        && vk_check_result(
            "vkCreateDebugUtilsMessengerEXT",
            f(this->h, &dbg, nullptr, &this->messenger))
        && (this->set_obj_name_f =
            GET_PROC_ADDR(*this, vkSetDebugUtilsObjectNameEXT));
}

template<typename R>
R Instance::get_proc_addr(const char *name) const {
    NNGN_LOG_CONTEXT_CF(Instance);
    const auto ret = reinterpret_cast<R>(vkGetInstanceProcAddr(this->h, name));
    if(!ret)
        Log::l() << name << " not present" << std::endl;
    return ret;
}

bool Instance::set_obj_name(
    VkDevice dev, VkObjectType type, std::uint64_t obj, std::string_view name
) const {
    return !this->set_obj_name_f || LOG_RESULT(
        this->set_obj_name_f, dev,
        rptr(VkDebugUtilsObjectNameInfoEXT{
            .sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT,
            .pNext = {},
            .objectType = type,
            .objectHandle = obj,
            .pObjectName = name.data()}));
}

bool Instance::set_obj_name(
    VkDevice d, VkObjectType t,
    std::uint64_t obj, std::string_view name, std::size_t n, std::string *buf
) const {
    *buf = name.data() + std::to_string(n);
    return !this->set_obj_name_f
        || this->set_obj_name(d, t, obj, buf->c_str());
}

bool InstanceInfo::init() {
    CHECK_RESULT(vkEnumerateInstanceVersion, &this->version);
    this->init_extensions();
    this->init_layers();
    return true;
}

void InstanceInfo::init_extensions() {
    this->extensions = vk_parse_extensions(
        std::span<const VkExtensionProperties>{
            nngn::vk_enumerate<
                vkEnumerateInstanceExtensionProperties>(nullptr)});
}

void InstanceInfo::init_layers() {
    const auto v = vk_enumerate<vkEnumerateInstanceLayerProperties>();
    this->layers.reserve(v.size());
    for(const auto &x : v){
        auto &l = this->layers.emplace_back(Graphics::Layer{
            .name = {},
            .description = {},
            .spec_version = {},
            .version = x.implementationVersion});
        std::strcpy(l.name.data(), x.layerName);
        std::strcpy(l.description.data(), x.description);
        std::strcpy(
            l.spec_version.data(),
            vk_version_str(x.specVersion).c_str());
    }
    std::ranges::sort(this->layers, named_cmp<Graphics::Layer>);
}

void InstanceInfo::init_devices(VkInstance i) {
    this->physical_devs = vk_enumerate<vkEnumeratePhysicalDevices>(i);
}

bool InstanceInfo::check_version(Graphics::Version v) const {
    const auto maj = VK_VERSION_MAJOR(this->version);
    const auto min = VK_VERSION_MINOR(this->version);
    const auto patch = VK_VERSION_PATCH(this->version);
    if(maj < v.major)
        return false;
    if(maj > v.major)
        return true;
    if(min < v.minor)
        return false;
    if(min > v.minor)
        return true;
    return v.patch <= patch;
}

}
#endif

#ifndef NNGN_GRAPHICS_VULKAN_INSTANCE_H
#define NNGN_GRAPHICS_VULKAN_INSTANCE_H

#include <cstdint>
#ifndef __clang__
#include <ranges>
#endif
#include <span>
#include <string_view>
#include <type_traits>
#include <vector>

#include "graphics/graphics.h"

#include "types.h"
#include "vulkan.h"

namespace nngn {

/** Owning wrapper for a Vulkan instance. */
class Instance {
public:
    using ErrorFn = void(void*);
    NNGN_MOVE_ONLY(Instance)
    Instance(void) = default;
    /** Destroys the VkInstance and debug objects. */
    ~Instance(void);
    /** Initializes an instance with no debug capabilities. */
    bool init(
        Graphics::Version version,
        std::span<const char *const> extensions);
    /**
     * Initializes an instance in debug mode.
     * \c extensions and \c layers must be supported by the implementation (see
     * \ref InstanceInfo).
     * \param error_fn Called when the debug callback reports an error.
     * \param error_data Argument for \c error_fn.
     */
    bool init_debug(
        Graphics::Version version,
        std::span<const char *const> extensions,
        std::span<const char *const> layers,
        ErrorFn error_fn, void *error_data, Graphics::LogLevel log_level);
    /** Provides access to the underlying Vulkan handle. */
    VkInstance id() const { return this->h; }
    /** Called by the debug callback when an error is received. */
    void set_error() { if(this->error_fn) this->error_fn(this->error_data); }
    /** Retrieves a function pointer by name, cast to \c R. */
    template<typename R> R get_proc_addr(const char *name) const;
    /** Sets the debug name of an object. */
    template<typename T>
    bool set_obj_name(VkDevice dev, T obj, std::string_view name) const;
    /** Sets the debug name of an object. */
    template<typename T>
    bool set_obj_name(
        VkDevice d, T obj, std::string_view name, std::size_t n,
        std::string *buf) const;
    /** Sets the debug name of a range of objects. */
    template<typename T>
    bool set_obj_name(
        VkDevice d, std::span<T> v, std::string_view name) const;
    /** Sets the debug name of a range of objects. */
    template<typename T>
    bool set_obj_name(
        VkDevice d, T obj, std::string_view name, std::size_t n) const;
private:
    template<typename T> static auto obj_handle(T o);
    bool set_obj_name(
        VkDevice d, VkObjectType t,
        std::uint64_t obj, std::string_view name) const;
    bool set_obj_name(
        VkDevice d, VkObjectType t,
        std::uint64_t obj, std::string_view name, std::size_t n,
        std::string *buf) const;
    VkInstance h = {};
    VkDebugUtilsMessengerEXT messenger = {};
    PFN_vkSetDebugUtilsObjectNameEXT set_obj_name_f = {};
    ErrorFn *error_fn = {};
    void *error_data = {};
};

/** Aggregate type for information about an instance. */
struct InstanceInfo {
public:
    std::uint32_t version = {};
    std::vector<Graphics::Extension> extensions = {};
    std::vector<Graphics::Layer> layers = {};
    std::vector<VkPhysicalDevice> physical_devs = {};
    /** Initializes instance-independent data. */
    bool init();
    /** Initializes device data. */
    void init_devices(VkInstance i);
    /** Verifies that the given version is supported. */
    bool check_version(Graphics::Version v) const;
private:
    void init_extensions();
    void init_layers();
};

template<typename T> auto Instance::obj_handle(T obj) {
    if constexpr(std::is_base_of_v<Handle, T>)
        return obj.id;
    else
        return reinterpret_cast<std::uintptr_t>(obj);
}

template<typename T>
inline bool Instance::set_obj_name(
    VkDevice d, T obj, std::string_view name
) const {
    return this->set_obj_name(
        d, vk_obj_type<T>, Instance::obj_handle(obj), name);
}

template<typename T>
inline bool Instance::set_obj_name(
    VkDevice d, T obj, std::string_view name, std::size_t n
) const {
    return this->set_obj_name(
        d, vk_obj_type<T>, Instance::obj_handle(obj), name, n);
}

template<typename T>
inline bool Instance::set_obj_name(
    VkDevice d, T obj, std::string_view name, std::size_t n,
    std::string *buf
) const {
    return this->set_obj_name(
        d, vk_obj_type<T>, Instance::obj_handle(obj), name, n, buf);
}

template<typename T>
inline bool Instance::set_obj_name(
    VkDevice d, std::span<T> v, std::string_view name
) const {
    std::string buf = {};
    constexpr auto t = vk_obj_type<std::decay_t<T>>;
    for(std::size_t i = 0, n = v.size(); i < n; ++i)
        if(!this->set_obj_name(d, t, Instance::obj_handle(v[i]), name, i, &buf))
            return false;
    return true;
}

}

#endif

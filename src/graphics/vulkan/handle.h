#ifndef NNGN_GRAPHICS_VULKAN_HANDLE_H
#define NNGN_GRAPHICS_VULKAN_HANDLE_H

#include <cstdint>
#include <type_traits>

namespace nngn {

/**
 * Base class for strongly-typed handles in 32-bit platforms.
 * Example usage:
 * \code{.cpp}
 *     #if !(defined(_WIN64) || defined(__x86_64__) || ...)
 *     #define VK_DEFINE_NON_DISPATCHABLE_HANDLE(o) \
 *         struct o : Handle { using Handle::Handle; };
 *     #endif
 * \endcode
 */
struct Handle {
    std::uint64_t id;
    Handle() = default;
    Handle(std::uint64_t id_) : id(id_) {}
    operator std::uint64_t() const { return this->id; }
};

static_assert(sizeof(Handle) == sizeof(std::uint64_t));
static_assert(alignof(Handle) == alignof(std::uint64_t));
static_assert(std::is_standard_layout_v<Handle>);

}

#endif

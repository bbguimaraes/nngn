#ifndef NNGN_GRAPHICS_VULKAN_VULKAN_H
#define NNGN_GRAPHICS_VULKAN_VULKAN_H

#include "os/platform.h"

#ifdef NNGN_PLATFORM_HAS_VULKAN
#include "handle.h"

#ifdef NNGN_PLATFORM_32BIT
#define VK_DEFINE_NON_DISPATCHABLE_HANDLE(o) \
    struct o : nngn::Handle { using Handle::Handle; };
#endif

#include <vulkan/vulkan.h>
#endif

#endif

#ifndef RENDER_H_INCLUDED
#define RENDER_H_INCLUDED

#include <vulkan/vulkan.h>

struct RenderContext {
    VkInstance instance;
    VkAllocationCallbacks* allocationCallbacks;
    PFN_vkGetInstanceProcAddr fp_vkGetInstanceProcAddr;
};

struct RenderContext rc_init();
void rc_cleanup(struct RenderContext* renderContext);

#endif

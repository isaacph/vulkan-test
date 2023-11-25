#ifndef RENDER_H_INCLUDED
#define RENDER_H_INCLUDED
#include "util.h"
#include "render_functions.h"

struct RenderContext {
    VkInstance instance;
    VkPhysicalDevice physicalDevice;
    VkDevice device;
    VkQueue queue;
    uint32_t queueFamilyIndex;
    VkSurfaceKHR surface;
    VkAllocationCallbacks* allocationCallbacks;
    VkSwapchainKHR swapchain;
    VkSurfaceFormatKHR surfaceFormat;
};

struct RenderContext rc_init(PFN_vkGetInstanceProcAddr fp_vkGetInstanceProcAddr);
void rc_swapchain_init(struct RenderContext* renderContext);
void rc_cleanup(struct RenderContext* renderContext);

#if defined(_WIN32)
struct RenderContext rc_init_win32(HINSTANCE hInstance, HWND hwnd);
#endif

#endif

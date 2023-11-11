#ifndef RENDER_H_INCLUDED
#define RENDER_H_INCLUDED
#include "util.h"
#include "render_functions.h"

struct RenderContext {
    VkInstance instance;
    VkDevice device;
    VkQueue queue;
    VkSurfaceKHR surface;
    VkAllocationCallbacks* allocationCallbacks;
};

struct RenderContext rc_init(PFN_vkGetInstanceProcAddr fp_vkGetInstanceProcAddr);
void rc_cleanup(struct RenderContext* renderContext);

#if defined(_WIN32)
struct RenderContext rc_init_win32(HINSTANCE hInstance, HWND hwnd);
#endif

#endif

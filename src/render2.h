#ifndef RENDER2_H_INCLUDED
#define RENDER2_H_INCLUDED

#include "render_functions.h"
#define FRAME_COUNT 2

typedef struct FrameData {
    VkCommandPool commandPool;
    VkCommandBuffer mainCommandBuffer;
} FrameData;

typedef struct RenderContext2 {
    VkInstance instance;
    VkDebugUtilsMessengerEXT debugMessenger;
    VkPhysicalDevice physicalDevice;
    VkDevice device;
    VkSurfaceKHR surface;
    VkAllocationCallbacks* allocationCallbacks;
    VkSwapchainKHR swapchain;
    VkSurfaceFormatKHR surfaceFormat;
    VkExtent2D swapchainExtent;
    VkQueue graphicsQueue;
    uint32_t graphicsQueueFamily;
    FrameData frames[FRAME_COUNT];
} RenderContext2;

RenderContext2 rc_init_instance(PFN_vkGetInstanceProcAddr fp_vkGetInstanceProcAddr);
void rc_swapchain_init(RenderContext2* renderContext);
void rc_cleanup(RenderContext2* renderContext);
void rc_draw(RenderContext2* context);
void rc_size_change(RenderContext2* context, uint32_t width, uint32_t height);
VkResult rc_load_shader_module(RenderContext2* rc, const unsigned char* source, uint32_t length, VkShaderModule* outShaderModule);
void rc_init_pipelines(RenderContext2* context);
#if defined(_WIN32)
struct RenderContext2 rc_init_win32(HINSTANCE hInstance, HWND hwnd);
#endif

#endif // RENDER2_H_INCLUDED

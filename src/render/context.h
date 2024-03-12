#ifndef RENDER_CONTEXT_H_INCLUDED
#define RENDER_CONTEXT_H_INCLUDED
#include "functions.h"
#define RC_SWAPCHAIN_LENGTH 3

typedef struct FrameData {
    VkCommandPool commandPool;
    VkCommandBuffer mainCommandBuffer;
    VkSemaphore swapchainSemaphore, renderSemaphore;
    VkFence renderFence;
} FrameData;

typedef struct SwapchainImageData {
    VkImage swapchainImage;
    VkImageView swapchainImageView;
} SwapchainImageData;

typedef struct RenderContext {
    VkInstance instance;
    VkPhysicalDevice physicalDevice;
    VkDevice device;
    VkSurfaceKHR surface;
    VkAllocationCallbacks* allocationCallbacks;
    VkSwapchainKHR swapchain;
    VkSurfaceFormatKHR surfaceFormat;
    VkExtent2D swapchainExtent;
    VkQueue graphicsQueue;
    uint32_t graphicsQueueFamily;
    FrameData frames[RC_SWAPCHAIN_LENGTH];
    SwapchainImageData images[RC_SWAPCHAIN_LENGTH];
    uint64_t frameNumber;
} RenderContext;

RenderContext rc_init_instance(PFN_vkGetInstanceProcAddr fp_vkGetInstanceProcAddr);
void rc_swapchain_configure(RenderContext* renderContext);
void rc_init_swapchain(RenderContext* renderContext, uint32_t width, uint32_t height);
void rc_destroy(RenderContext* renderContext);
void rc_draw(RenderContext* context);
void rc_size_change(RenderContext* context, uint32_t width, uint32_t height);
// VkResult rc_load_shader_module(RenderContext* rc, const unsigned char* source, uint32_t length, VkShaderModule* outShaderModule);
// void rc_init_pipelines(RenderContext* context);
void rc_init_loop(RenderContext* context);
void rc_transition_image(VkCommandBuffer cmd, VkImage image, VkImageLayout currentLayout, VkImageLayout newLayout);
VkImageSubresourceRange basic_image_subresource_range(VkImageAspectFlags aspectMask);
#if defined(_WIN32)
struct RenderContext rc_init_win32(HINSTANCE hInstance, HWND hwnd);
#endif

#endif // RENDER_CONTEXT_H_INCLUDED 

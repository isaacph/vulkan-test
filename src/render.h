#ifndef RENDER_H_INCLUDED
#define RENDER_H_INCLUDED
#include "util.h"
#include "render_functions.h"
#define RC_SWAPCHAIN_LENGTH 3

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
    VkImage swapchainImages[RC_SWAPCHAIN_LENGTH];
    VkImageView swapchainImageViews[RC_SWAPCHAIN_LENGTH];
    VkExtent2D swapchainExtent;
    VkCommandPool commandPool;
    VkCommandBuffer commandBuffer;
    VkRenderPass renderPass;
    VkFramebuffer framebuffers[RC_SWAPCHAIN_LENGTH];
    VkFence renderFence;
    VkSemaphore presentSemaphore;
    VkSemaphore renderSemaphore;

    int frameNumber;
};

struct RenderContext rc_init(PFN_vkGetInstanceProcAddr fp_vkGetInstanceProcAddr);
void rc_swapchain_init(struct RenderContext* renderContext);
void rc_cleanup(struct RenderContext* renderContext);
void rc_draw(struct RenderContext* context);
void rc_size_change(struct RenderContext* context, uint32_t width, uint32_t height);

#if defined(_WIN32)
struct RenderContext rc_init_win32(HINSTANCE hInstance, HWND hwnd);
#endif

#endif

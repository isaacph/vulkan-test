#ifndef RENDER_CONTEXT_H_INCLUDED
#define RENDER_CONTEXT_H_INCLUDED
#include "functions.h"
#define RC_SWAPCHAIN_LENGTH 3
#include "util/memory.h"
#include <stdbool.h>
#include "win32.h"
#include "wayland.h"

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

typedef struct AllocatedImage {
    VkImage image;
    VkImageView imageView;
    // insert memory stuff
    VkExtent3D imageExtent;
    VkFormat imageFormat;
} AllocatedImage;

typedef struct RenderContext {
    VkInstance instance;
    VkPhysicalDevice physicalDevice;
    VkDevice device;
    VkSurfaceKHR surface;
    VkSwapchainKHR swapchain;
    VkSurfaceFormatKHR surfaceFormat;
    VkExtent2D swapchainExtent;
    VkQueue graphicsQueue;
    uint32_t graphicsQueueFamily;
    FrameData frames[RC_SWAPCHAIN_LENGTH];
    SwapchainImageData images[RC_SWAPCHAIN_LENGTH];
    uint64_t frameNumber;

    AllocatedImage drawImage;
    VkExtent2D drawExtent;
} RenderContext;

// implementation must be platform-specific
PFN_vkGetInstanceProcAddr rc_proc_addr();

typedef struct InitInstance {
    VkInstance instance;
} InitInstance;
InitInstance rc_init_instance(PFN_vkGetInstanceProcAddr fp_vkGetInstanceProcAddr, bool debug, StaticCache* cleanup);

// implementation and WindowHandle are per OS
typedef void (*OnWindowFunction)(void*);
typedef struct InitSurfaceParams {
    VkInstance instance;
    const char* title;
    int width;
    int height;
} InitSurfaceParams;
typedef struct InitSurface {
    WindowHandle windowHandle;
    VkSurfaceKHR surface;
    void* user;
} InitSurface;
InitSurface rc_init_surface(InitSurfaceParams params, StaticCache* cleanup);

typedef struct InitDeviceParams {
    VkInstance instance;
    VkSurfaceKHR surface;
    VkSurfaceFormatKHR surfaceFormat;
} InitDeviceParams;
typedef struct InitDevice {
    VkPhysicalDevice physicalDevice;
    VkDevice device;
    VkQueue queue;
    uint32_t graphicsQueueFamily;
} InitDevice;
InitDevice rc_init_device(InitDeviceParams params);


void rc_swapchain_configure(RenderContext* renderContext);
void rc_init_swapchain(RenderContext* renderContext, uint32_t width, uint32_t height);
void rc_destroy(RenderContext* renderContext);
void rc_draw(RenderContext* context);
void rc_size_change(RenderContext* context, uint32_t width, uint32_t height);
// VkResult rc_load_shader_module(RenderContext* rc, const unsigned char* source, uint32_t length, VkShaderModule* outShaderModule);
// void rc_init_pipelines(RenderContext* context);
void rc_init_loop(RenderContext* context);
#if defined(_WIN32)
struct RenderContext rc_init_win32(HINSTANCE hInstance, HWND hwnd);
#endif

// image functions
VkImageSubresourceRange basic_image_subresource_range(VkImageAspectFlags aspectMask);
void rc_transition_image(VkCommandBuffer cmd, VkImage image, VkImageLayout currentLayout, VkImageLayout newLayout);
VkImageViewCreateInfo imageview_create_info(VkFormat format, VkImage image, VkImageAspectFlags aspectFlags);
VkImageCreateInfo image_create_info(VkFormat format, VkImageUsageFlags usageFlags, VkExtent3D extent);

#endif // RENDER_CONTEXT_H_INCLUDED 

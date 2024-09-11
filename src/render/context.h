#ifndef RENDER_CONTEXT_H_INCLUDED
#define RENDER_CONTEXT_H_INCLUDED
#include "functions.h"
#define RC_SWAPCHAIN_LENGTH 3
#define FRAME_OVERLAP 2
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
    FrameData frames[FRAME_OVERLAP];
    SwapchainImageData images[RC_SWAPCHAIN_LENGTH];
    uint64_t frameNumber;
} RenderContext;

// implementation must be platform-specific
PFN_vkGetInstanceProcAddr rc_proc_addr();

typedef struct InitInstance {
    VkInstance instance;
} InitInstance;
InitInstance rc_init_instance(PFN_vkGetInstanceProcAddr fp_vkGetInstanceProcAddr, bool debug, StaticCache* cleanup);

// implementation and WindowHandle are per OS
static const VkExtent2D DEFAULT_SURFACE_SIZE = { .width = INT_MIN, .height = INT_MIN };
typedef void (*OnWindowFunction)(void*);
typedef struct InitSurfaceParams {
    VkInstance instance;
    const char* title;
    int titleLength;
    VkExtent2D size;
    bool headless;
} InitSurfaceParams;
typedef struct InitSurface {
    WindowHandle windowHandle;
    VkSurfaceKHR surface;
    VkExtent2D size;
} InitSurface;
InitSurface rc_init_surface(InitSurfaceParams params, StaticCache* cleanup);

typedef struct InitDeviceParams {
    VkInstance instance;
    VkSurfaceKHR surface;
} InitDeviceParams;
typedef struct InitDevice {
    VkPhysicalDevice physicalDevice;
    VkDevice device;
    VkQueue graphicsQueue;
    uint32_t graphicsQueueFamily;
    VkSurfaceFormatKHR surfaceFormat;
    VkSurfaceCapabilitiesKHR surfaceCapabilities;
} InitDevice;
InitDevice rc_init_device(InitDeviceParams params, StaticCache* cleanup);

// init swapchain is to be called every time the window size changes to rebuild a new swapchain for it
// invalidates oldSwapchain to reuse its resources if possible via the Vulkan implementation
typedef struct InitSwapchainParams {
    // the extent of the window to initialize a swapchain for (will be validated)
    VkExtent2D extent;

    // setup data
    VkPhysicalDevice physicalDevice;
    uint32_t graphicsQueueFamily;
    VkDevice device;
    VkSurfaceKHR surface;
    VkSurfaceFormatKHR surfaceFormat;

    // pass in the swapchain from the previous call for reuse (or VK_NULL_HANDLE if there is none)
    // these handles will all be deleted and cleared
    VkSwapchainKHR oldSwapchain;
    sc_t swapchainCleanupHandle;
} InitSwapchainParams;
typedef struct InitSwapchain {
    VkSwapchainKHR swapchain;
    SwapchainImageData images[RC_SWAPCHAIN_LENGTH];
    sc_t swapchainCleanupHandle;
} InitSwapchain;
InitSwapchain rc_init_swapchain(InitSwapchainParams params, StaticCache* cleanup);

typedef struct InitLoopParams {
    VkDevice device;
    uint32_t graphicsQueueFamily;
} InitLoopParams;
typedef struct InitLoop {
    FrameData frames[FRAME_OVERLAP];
} InitLoop;
InitLoop rc_init_loop(InitLoopParams params, StaticCache* cleanup);

typedef struct WindowUpdate {
    bool windowClosed;
    bool shouldDraw;
    bool requireResize;
    VkExtent2D resize; // 0 unless resized
} WindowUpdate;
WindowUpdate rc_window_update(WindowHandle* windowHandle);

typedef struct DrawParams {
    VkDevice device;
    VkSwapchainKHR swapchain;
    FrameData frame;
    float color;
    VkQueue graphicsQueue;
    SwapchainImageData swapchainImages[RC_SWAPCHAIN_LENGTH];
} DrawParams;
void rc_draw(DrawParams params);

void rc_destroy(RenderContext* renderContext);
void rc_size_change(RenderContext* context, uint32_t width, uint32_t height);
// VkResult rc_load_shader_module(RenderContext* rc, const unsigned char* source, uint32_t length, VkShaderModule* outShaderModule);
// void rc_init_pipelines(RenderContext* context);
// void rc_init_loop(RenderContext* context);
#if defined(_WIN32)
struct RenderContext rc_init_win32(HINSTANCE hInstance, HWND hwnd);
#endif

// image functions
VkImageSubresourceRange basic_image_subresource_range(VkImageAspectFlags aspectMask);
void rc_transition_image(VkCommandBuffer cmd, VkImage image, VkImageLayout currentLayout, VkImageLayout newLayout);
VkImageViewCreateInfo imageview_create_info(VkFormat format, VkImage image, VkImageAspectFlags aspectFlags);
VkImageCreateInfo image_create_info(VkFormat format, VkImageUsageFlags usageFlags, VkExtent3D extent);

#endif // RENDER_CONTEXT_H_INCLUDED 

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

#endif // RENDER2_H_INCLUDED

#ifndef RENDER_H_INCLUDED
#define RENDER_H_INCLUDED
#include <stdbool.h>
#include "util.h"
#include "render_functions.h"
#include <ft2build.h>

#include <freetype/freetype.h>
#include <freetype/ftoutln.h>

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
    VkShaderModule triangleVertShader;
    VkShaderModule triangleFragShader;
    VkPipelineLayout trianglePipelineLayout;
    VkPipeline trianglePipeline;

    int frameNumber;
};

struct RenderContext rc_init(PFN_vkGetInstanceProcAddr fp_vkGetInstanceProcAddr);
void rc_swapchain_init(struct RenderContext* renderContext);
void rc_cleanup(struct RenderContext* renderContext);
void rc_draw(struct RenderContext* context);
void rc_size_change(struct RenderContext* context, uint32_t width, uint32_t height);
VkResult rc_load_shader_module(struct RenderContext* rc, const unsigned char* source, uint32_t length, VkShaderModule* outShaderModule);
void rc_init_pipelines(struct RenderContext* context);

#if defined(_WIN32)
struct RenderContext rc_init_win32(HINSTANCE hInstance, HWND hwnd);
#endif

#endif

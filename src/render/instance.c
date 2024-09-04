#include "../app.h"
#include "context.h"
#include "util.h"
#include <stdbool.h>
#include "util/memory.h"
#include "util/backtrace.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

const char* const ENABLE_EXTENSIONS[] = {
    "VK_KHR_surface", // rendering to monitors
    "VK_KHR_win32_surface", // TODO: make this array change based on platform
    "VK_KHR_get_surface_capabilities2", // it doesn't seem to care whether we enable this but oh well
    "VK_EXT_swapchain_colorspace",
    "VK_EXT_debug_utils",
};
const size_t ENABLE_EXTENSIONS_COUNT = 4;

// uses VK_MAX_EXTENSION_NAME_SIZE according to:
// https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VkLayerProperties.html
const char* const ENABLE_LAYERS[] = {
    "VK_LAYER_KHRONOS_validation",
};
const size_t ENABLE_LAYERS_COUNT = 0;


void cleanup_instance(void* user_ptr) {
    printf("Instance cleaned up\n");
    vkDestroyInstance((VkInstance) user_ptr, NULL);
}

InitInstance rc_init_instance(PFN_vkGetInstanceProcAddr fp_vkGetInstanceProcAddr, bool debug, StaticCache* cleanup) {
    // variables we are keeping around, the function's output
    VkInstance instance = NULL;

    // initialize loader functions
    init_loader_functions(fp_vkGetInstanceProcAddr);

    // do preinit checks
    {
        uint32_t extensionsCount;
        VkExtensionProperties* extensions;
        uint32_t layersCount = 0;
        VkLayerProperties* layers = NULL; // must be freed

        // check VK_API_VERSION
        uint32_t instanceVersion;
        check(vkEnumerateInstanceVersion(&instanceVersion));
        char vk_api_version[64];
        calc_VK_API_VERSION(instanceVersion, vk_api_version, 64);
        if (debug) printf("Vulkan instance version %s\n", vk_api_version);

        // get VkExtensionProperties
        extensionsCount = 0;
        extensions = NULL;
        check(vkEnumerateInstanceExtensionProperties(NULL, &extensionsCount, NULL));
        extensions = malloc(sizeof(VkExtensionProperties) * extensionsCount);
        if (extensions == NULL) exception();
        check(vkEnumerateInstanceExtensionProperties(NULL, &extensionsCount, extensions));
        if (debug) print_VkExtensionProperties(extensionsCount, extensions);
        // free(extensions);

        // get VkLayerProperties
        layersCount = 0;
        layers = NULL;
        check(vkEnumerateInstanceLayerProperties(&layersCount, NULL));
        layers = malloc(sizeof(VkLayerProperties) * layersCount);
        if (layers == NULL) exception();
        check(vkEnumerateInstanceLayerProperties(&layersCount, layers));
        if (debug) print_VkLayerProperties(layersCount, layers);
        // free(layers);

        // check we have the required extensions
        for (int i = 0; i < ENABLE_EXTENSIONS_COUNT; ++i) {
            const char* required = ENABLE_EXTENSIONS[i];
            bool found = false;
            for (int j = 0; j < extensionsCount && !found; ++j) {
                if (strncmp(required, extensions[j].extensionName, VK_MAX_EXTENSION_NAME_SIZE) == 0) {
                    found = true;
                }
            }
            if (!found) {
                char msg[VK_MAX_EXTENSION_NAME_SIZE + 64];
                sprintf(msg, "ERROR: missing required Vulkan instance extension: %s\n", required);
                exception_msg(msg);
            }
        }

        // check we have the required layers
        for (int i = 0; i < ENABLE_LAYERS_COUNT; ++i) {
            const char* required = ENABLE_LAYERS[i];
            bool found = false;
            for (int j = 0; j < layersCount && !found; ++j) {
                if (strncmp(required, layers[j].layerName, VK_MAX_EXTENSION_NAME_SIZE) == 0) {
                    found = true;
                }
            }
            if (!found) {
                char msg[VK_MAX_EXTENSION_NAME_SIZE + 64];
                sprintf(msg, "ERROR: missing required Vulkan layer: %s\n", required);
                exception_msg(msg);
            }
        }
        if (debug) printf("All required instance extensions and layers found.\n");

        free(extensions);
        free(layers);
    }

    // initialize vkInstance
    {
        // make and load instance
        printf("Initializing vkInstance\n");
        VkApplicationInfo app_info = {
            .sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
            .pNext = NULL,
            .pApplicationName = APP_NAME,
            .applicationVersion = APP_VERSION,
            .pEngineName = APP_ENGINE_NAME,
            .engineVersion = APP_ENGINE_VERSION,
            .apiVersion = VK_MAKE_API_VERSION(0, 1, 3, 236)
        };
        VkInstanceCreateInfo instanceCreateInfo = {
            .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
            .flags = (VkInstanceCreateFlags) 0,
            .pApplicationInfo = &app_info,
            .enabledLayerCount = ENABLE_LAYERS_COUNT,
            .ppEnabledLayerNames = ENABLE_LAYERS,
            .enabledExtensionCount = ENABLE_EXTENSIONS_COUNT,
            .ppEnabledExtensionNames = ENABLE_EXTENSIONS,
        };
        instance = NULL;
        check(vkCreateInstance(&instanceCreateInfo, NULL, &instance));
        printf("Created instance\n");

        // init instance functions
        init_instance_functions(instance);
    }

    // RenderContext renderContext = {
    //     // make render context for later use
    //     .instance = instance,
    //     .physicalDevice = VK_NULL_HANDLE,
    //     .device = VK_NULL_HANDLE,
    //     .surface = VK_NULL_HANDLE, // defined by platform-specific code
    //     .swapchain = VK_NULL_HANDLE,
    //     .surfaceFormat = {0},
    //     .swapchainExtent = {0},
    //     .graphicsQueue = VK_NULL_HANDLE,
    //     .graphicsQueueFamily = 0,
    //     .frameNumber = 0,
    //     .frames = {0},
    //     .images = {0},
    // };
    // FrameData emptyFrame = {
    //     .commandPool = VK_NULL_HANDLE,
    //     .mainCommandBuffer = VK_NULL_HANDLE,
    //     .swapchainSemaphore = VK_NULL_HANDLE,
    //     .renderSemaphore = VK_NULL_HANDLE,
    //     .renderFence = VK_NULL_HANDLE,
    // };
    // SwapchainImageData emptyImage = {
    //     .swapchainImage = VK_NULL_HANDLE,
    //     .swapchainImageView = VK_NULL_HANDLE,
    // };
    // for (int i = 0; i < RC_SWAPCHAIN_LENGTH; ++i) {
    //     renderContext.frames[i] = emptyFrame;
    //     renderContext.images[i] = emptyImage;
    // }

    StaticCache_add(cleanup, cleanup_instance, (void*) instance);

    return (InitInstance) {
        .instance = instance,
    };
}

void rc_destroy(RenderContext* context) {
    printf("Cleaning up Vulkan\n");
    printf("Waiting for fences first...\n");
    for (int i = 0; i < RC_SWAPCHAIN_LENGTH; ++i) {
        check(vkWaitForFences(context->device, 1, &context->frames[i].renderFence, true, 1000000000));
        check(vkResetFences(context->device, 1, &context->frames[i].renderFence));
    }
    printf("Destroying sync primitives");
    for (int i = 0; i < RC_SWAPCHAIN_LENGTH; ++i) {
        vkDestroySemaphore(context->device, context->frames[i].swapchainSemaphore, NULL);
        vkDestroySemaphore(context->device, context->frames[i].renderSemaphore, NULL);
        vkDestroyFence(context->device, context->frames[i].renderFence, NULL);
    }
    vkDestroyImageView(context->device, context->drawImage.imageView, NULL);
    for (int i = 0; i < RC_SWAPCHAIN_LENGTH; ++i) {
        if (context->images[i].swapchainImageView != VK_NULL_HANDLE) {
            vkDestroyImageView(
                context->device,
                context->images[i].swapchainImageView,
                NULL);
            // it doesn't seem like we use framebuffers for this version?
            // vkDestroyFramebuffer(
            //     context->device,
            //     context->framebuffers[i],
            //     NULL);
        }
    }
    vkDestroySwapchainKHR(
            context->device,
            context->swapchain,
            NULL);
    vkDestroySurfaceKHR(
            context->instance,
            context->surface,
            NULL);
    for (int i = 0; i < RC_SWAPCHAIN_LENGTH; ++i) {
        if (context->images[i].swapchainImageView != VK_NULL_HANDLE) {
            vkFreeCommandBuffers(context->device, context->frames[i].commandPool, 1, &context->frames[i].mainCommandBuffer);
            vkDestroyCommandPool(context->device, context->frames[i].commandPool, NULL);
        }
    }
    vkDestroyDevice(context->device, NULL);
    vkDestroyInstance(context->instance, NULL);
}

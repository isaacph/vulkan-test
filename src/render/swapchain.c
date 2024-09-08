#include "context.h"
#include "util.h"
#include <stdbool.h>
#include "../util/memory.h"
#include "util/backtrace.h"
#include <stdlib.h>
#include <stdio.h>

// must be called prior to calling rc_init_swapchain
ConfigureSwapchain rc_configure_swapchain(ConfigureSwapchainParams params, StaticCache* cleanup) {
    ConfigureSwapchain swapchain = {0};
    // init command pools and buffers for each frame
    for (uint32_t index = 0; index < RC_SWAPCHAIN_LENGTH; ++index) {
        VkCommandPool commandPool = VK_NULL_HANDLE;
        VkCommandBuffer commandBuffer = VK_NULL_HANDLE;

        VkCommandPoolCreateInfo commandPoolCreateInfo = {
            .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
            .queueFamilyIndex = params.graphicsQueueFamily,
            .flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
            .pNext = NULL,
        };
        check(vkCreateCommandPool(params.device, &commandPoolCreateInfo, NULL, &commandPool));

        VkCommandBufferAllocateInfo cmdAllocInfo = {
            .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
            .pNext = NULL,
            .commandPool = commandPool,
            .commandBufferCount = 1,
            .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
        };
        check(vkAllocateCommandBuffers(params.device, &cmdAllocInfo, &commandBuffer));

        swapchain.frames[index].commandPool = commandPool;
        swapchain.frames[index].mainCommandBuffer = commandBuffer;
    }
    return swapchain;
}

typedef struct SwapchainCleanup {
    VkDevice device;
    VkSwapchainKHR swapchain;
    VkImageView imageViews[RC_SWAPCHAIN_LENGTH];
} SwapchainCleanup;
void cleanup_swapchain(void* ptr, sc_t id) {
    SwapchainCleanup* params = (SwapchainCleanup*) ptr;
    vkDestroySwapchainKHR(params->device, params->swapchain, NULL);
    for (int i = 0; i < RC_SWAPCHAIN_LENGTH; ++i) {
        vkDestroyImageView(params->device, params->imageViews[i], NULL);
    }
    free(params);
}

// I think this is basically glViewport, but in this case we also receive a recommendation from the graphics card??????
InitSwapchain rc_init_swapchain(InitSwapchainParams params, StaticCache* cleanup) {
    if (params.surface == VK_NULL_HANDLE) {
        exception_msg("Must create surface before creating swapchain\n");
    }
    if (params.device == VK_NULL_HANDLE) {
        exception_msg("Must create device before creating swapchain\n");
    }
    if (params.surfaceFormat.format == VK_FORMAT_UNDEFINED) {
        exception_msg("Must decide surface format before creating swapchain\n");
    }

    // validate surface extent and image counts
    VkExtent2D extent = { 0 };
    {
        void* pNext = NULL;
        /* once I want to do fullscreen
        // will need to use the same parameters for these two structs
        // inside of VkSwapchainCreateInfoKHR for the capabilities reported here
        // to be equivalent
        VkSurfaceFullScreenExclusiveWin32InfoEXT b = {
            .sType = VK_STRUCTURE_TYPE_SURFACE_FULL_SCREEN_EXCLUSIVE_INFO_EXT,
            .pNext = pNext,
            .hmonitor = MonitorFromWindow(...),
        };
        pNext = &b;
        VkSurfaceFullScreenExclusiveInfoEXT a = {
            .sType = VK_STRUCTURE_TYPE_SURFACE_FULL_SCREEN_EXCLUSIVE_INFO_EXT,
            .pNext = pNext,
            .fullScreenExclusive = VK_FULL_SCREEN_EXCLUSIVE_DEFAULT_EXT,
        };
        pNext = &a;
        */
        // with VK_EXT_surface_maintenance1
        VkSurfacePresentModeEXT c4 = {
            .sType = VK_STRUCTURE_TYPE_SURFACE_PRESENT_MODE_EXT,
            .pNext = pNext,
            .presentMode = VK_PRESENT_MODE_FIFO_RELAXED_KHR,
        };
        pNext = &c4;
        // VK_KHR_get_surface_capabilities2
        VkPhysicalDeviceSurfaceInfo2KHR info = {
            .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SURFACE_INFO_2_KHR,
            .pNext = pNext,
            .surface = params.surface,
        };

        pNext = NULL;
        // doesn't seem useful rn
        // // with VK_KHR_surface_protected_capabilities
        // VkSurfaceProtectedCapabilitiesKHR c1 = {
        //     .sType = VK_STRUCTURE_TYPE_SURFACE_PROTECTED_CAPABILITIES_KHR,
        //     .pNext = pNext,
        // };
        // pNext = &c1;
        VkSurfaceCapabilities2KHR capabilities = {
            .sType = VK_STRUCTURE_TYPE_SURFACE_CAPABILITIES_2_KHR,
            .pNext = pNext,
        };
        check(vkGetPhysicalDeviceSurfaceCapabilities2KHR(params.physicalDevice, &info, &capabilities));
        // validate capabilities in capabilities.surfaceCapabilities
        // https://github.com/KhronosGroup/Vulkan-Samples/tree/main/samples/performance/swapchain_images
        if (RC_SWAPCHAIN_LENGTH < capabilities.surfaceCapabilities.minImageCount ||
            RC_SWAPCHAIN_LENGTH > capabilities.surfaceCapabilities.maxImageCount) {
            exception_msg("Device does not support 3 swapchain images\n");
        }

        extent = (VkExtent2D) {
            .width = params.extent.width,
            .height = params.extent.height,
        };
        if (extent.width < capabilities.surfaceCapabilities.minImageExtent.width ||
            extent.height < capabilities.surfaceCapabilities.minImageExtent.height) {
            printf("Warning: requested swapchain extent (%u x %u) is smaller than required extent (%u x %u)\n",
                    extent.width,
                    extent.height,
                    capabilities.surfaceCapabilities.minImageExtent.width,
                    capabilities.surfaceCapabilities.minImageExtent.height
                    );
            extent.width = MAX(extent.width, capabilities.surfaceCapabilities.minImageExtent.width);
            extent.height = MAX(extent.height, capabilities.surfaceCapabilities.minImageExtent.height);
        }
        if (extent.width > capabilities.surfaceCapabilities.minImageExtent.width ||
            extent.height > capabilities.surfaceCapabilities.minImageExtent.height) {
            printf("Warning: requested swapchain extent (%u x %u) is larger than required extent (%u x %u)\n",
                    extent.width,
                    extent.height,
                    capabilities.surfaceCapabilities.maxImageExtent.width,
                    capabilities.surfaceCapabilities.maxImageExtent.height
                    );
            extent.width = MIN(extent.width, capabilities.surfaceCapabilities.maxImageExtent.width);
            extent.height = MIN(extent.height, capabilities.surfaceCapabilities.maxImageExtent.height);
        }
    }
    
    // create new swapchain
    VkSwapchainKHR swapchain = VK_NULL_HANDLE;
    {
        VkSwapchainCreateInfoKHR createInfo = {
            .sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
            .pNext = NULL,
            .flags = 0,
            .surface = params.surface,
            // https://github.com/KhronosGroup/Vulkan-Samples/tree/main/samples/performance/swapchain_images
            .minImageCount = RC_SWAPCHAIN_LENGTH, // triple buffering is just better 
            .imageFormat = params.surfaceFormat.format,
            .imageColorSpace = params.surfaceFormat.colorSpace,
            .imageExtent = extent,
            .imageArrayLayers = 1, // not VR "stereoscopic 3D"
            // will we need dst/src later?
            .imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT, // we want to use color (not depth RN)
            .imageSharingMode = VK_SHARING_MODE_EXCLUSIVE,
            // we found the queue family earlier
            .queueFamilyIndexCount = 1,
            .pQueueFamilyIndices = &params.graphicsQueueFamily,
            .preTransform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR, // not sure if this will work but oh well
            .compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR, // no transparent windows... FOR NOW
            .presentMode = VK_PRESENT_MODE_FIFO_KHR,
            .oldSwapchain = params.oldSwapchain,
        };
        vkCreateSwapchainKHR(params.device, &createInfo, NULL, &swapchain);
    }

    // get images for swapchain
    SwapchainImageData images[RC_SWAPCHAIN_LENGTH];
    {
        uint32_t imageCount = 0;
        check(vkGetSwapchainImagesKHR(params.device, swapchain, &imageCount, NULL));
        if (imageCount != RC_SWAPCHAIN_LENGTH) {
            char msg[300] = {0};
            snprintf(msg, 300, "Created swapchain has incorrect number of images: %u (should be %u)", imageCount, RC_SWAPCHAIN_LENGTH);
            exception_msg(msg);
        }
        printf("Swapchain image count: %i\n", imageCount);
        VkImage swapchainImages[RC_SWAPCHAIN_LENGTH];
        check(vkGetSwapchainImagesKHR(params.device, swapchain, &imageCount, swapchainImages));
        for (int i = 0; i < RC_SWAPCHAIN_LENGTH; ++i) {
            images[i].swapchainImage = swapchainImages[i];
        }
        // swapchain image lifetime is controlled by the swapchain lifetime, so they must not be destroyed manually
    }

    // create image views for swapchain
    for (int i = 0; i < RC_SWAPCHAIN_LENGTH; ++i) {
        VkImageViewCreateInfo imageViewCreateInfo = {
            .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
            .pNext = NULL,
            .flags = 0,
            .image = images[i].swapchainImage,
            .viewType = VK_IMAGE_VIEW_TYPE_2D,
            .format = params.surfaceFormat.format,
            .components = (VkComponentMapping) {
                .r = VK_COMPONENT_SWIZZLE_IDENTITY,
                .g = VK_COMPONENT_SWIZZLE_IDENTITY,
                .b = VK_COMPONENT_SWIZZLE_IDENTITY,
                .a = VK_COMPONENT_SWIZZLE_IDENTITY,
            },
            .subresourceRange = (VkImageSubresourceRange) {
                .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                .baseMipLevel = 0,
                .levelCount = 1,
                .baseArrayLayer = 0,
                .layerCount = 1,
            },
        };
        VkImageView view = VK_NULL_HANDLE;
        check(vkCreateImageView(
            params.device,
            &imageViewCreateInfo,
            //renderContext->allocationCallbacks,
            NULL,
            &view));
        images[i].swapchainImageView = view;
    }

    // swap out next swapchain to delete if the program ends
    // implicitly deletes the old swapchain if there was one
    SwapchainCleanup* swapchainCleanup = checkMalloc(malloc(sizeof(SwapchainCleanup)));
    *swapchainCleanup = (SwapchainCleanup) {
        .device = params.device,
        .swapchain = swapchain,
        .imageViews = { 0 },
    };
    for (int i = 0; i < RC_SWAPCHAIN_LENGTH; ++i) {
        swapchainCleanup->imageViews[i] = images[i].swapchainImageView;
    }
    sc_t cleanupHandle = StaticCache_put(cleanup, cleanup_swapchain, (void*) swapchainCleanup, params.swapchainCleanupHandle);

    InitSwapchain ret = {
        .swapchain = swapchain,
        .images = { 0 },
        .swapchainCleanupHandle = cleanupHandle,
    };
    // unfortunately we can't init an array directly in C above
    for (int i = 0; i < RC_SWAPCHAIN_LENGTH; ++i) {
        ret.images[i] = images[i];
    }
    return ret;
}

void rc_size_change(RenderContext* context, uint32_t width, uint32_t height) {
    if (context->instance == VK_NULL_HANDLE) {
        // wait to init first
        return;
    }
    // todo:
    // check(vkWaitForFences(context->device, 1, &context->renderFence, true, 1000000000));
    //rc_init_swapchain(context, width, height);
    // rc_init_framebuffers(context);
    // rc_init_pipelines(context);
}


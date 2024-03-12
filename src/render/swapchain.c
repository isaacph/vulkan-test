#include "context.h"
#include "util.h"
#include <stdbool.h>

// must be called prior to calling rc_init_swapchain
void rc_configure_swapchain(RenderContext* renderContext) {
    if (renderContext->surface == VK_NULL_HANDLE) {
        exception_msg("Must create surface before creating swapchain\n");
    }
    if (renderContext->device == VK_NULL_HANDLE) {
        exception_msg("Must create device before creating swapchain\n");
    }

    VkPhysicalDevice physicalDevice = renderContext->physicalDevice;
    VkDevice device = renderContext->device;
    VkSwapchainKHR swapchain = VK_NULL_HANDLE;
    VkSurfaceKHR surface = renderContext->surface;
    VkSurfaceFormatKHR surfaceFormat = {0};

    // get formats, choose the basic one lol
    bool chosen = false;
    uint32_t pSurfaceFormatCount;
    VkSurfaceFormatKHR* pSurfaceFormats;
    check(vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, surface, &pSurfaceFormatCount, NULL));
    pSurfaceFormats = malloc(pSurfaceFormatCount * sizeof(VkSurfaceFormatKHR));
    checkMalloc(pSurfaceFormats);
    check(vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, surface, &pSurfaceFormatCount, pSurfaceFormats));
    for (uint32_t i = 0; i < pSurfaceFormatCount; ++i) {
        VkSurfaceFormatKHR* format = &pSurfaceFormats[i];
        if (format->format == VK_FORMAT_B8G8R8A8_SRGB &&
                format->colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
            surfaceFormat = *format;
            chosen = true;
            break;
        }
    }
    if (!chosen) {
        exception_msg("Missing required surface format");
    }

    renderContext->surfaceFormat = surfaceFormat;
}

// I think this is basically glViewport, but in this case we also receive a recommendation from the graphics card??????
void rc_init_swapchain(RenderContext* renderContext, uint32_t width, uint32_t height) {
    if (renderContext->surface == VK_NULL_HANDLE) {
        exception_msg("Must create surface before creating swapchain\n");
    }
    if (renderContext->device == VK_NULL_HANDLE) {
        exception_msg("Must create device before creating swapchain\n");
    }
    if (renderContext->surfaceFormat.format == VK_FORMAT_UNDEFINED) {
        exception_msg("Must decide surface format before creating swapchain\n");
    }

    VkPhysicalDevice physicalDevice = renderContext->physicalDevice;
    VkDevice device = renderContext->device;
    VkSwapchainKHR swapchain = renderContext->swapchain;
    VkSurfaceKHR surface = renderContext->surface;
    VkSurfaceFormatKHR surfaceFormat = renderContext->surfaceFormat;

    // find surface extent and validate surface
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
            .surface = surface,
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
        check(vkGetPhysicalDeviceSurfaceCapabilities2KHR(physicalDevice, &info, &capabilities));
        // validate capabilities in capabilities.surfaceCapabilities
        // https://github.com/KhronosGroup/Vulkan-Samples/tree/main/samples/performance/swapchain_images
        if (RC_SWAPCHAIN_LENGTH < capabilities.surfaceCapabilities.minImageCount ||
            RC_SWAPCHAIN_LENGTH > capabilities.surfaceCapabilities.maxImageCount) {
            exception_msg("Device does not support 3 swapchain images\n");
        }

        extent = (VkExtent2D) {
            .width = width,
            .height = height,
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
    
    VkSwapchainKHR oldSwapchain = swapchain;
    VkSwapchainCreateInfoKHR createInfo = {
        .sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
        .pNext = NULL,
        .flags = 0,
        .surface = surface,
        // https://github.com/KhronosGroup/Vulkan-Samples/tree/main/samples/performance/swapchain_images
        .minImageCount = RC_SWAPCHAIN_LENGTH, // triple buffering is just better 
        .imageFormat = surfaceFormat.format,
        .imageColorSpace = surfaceFormat.colorSpace,
        .imageExtent = extent,
        .imageArrayLayers = 1, // not VR "stereoscopic 3D"
        // will we need dst/src later?
        .imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT, // we want to use color (not depth RN)
        .imageSharingMode = VK_SHARING_MODE_EXCLUSIVE,
        // we found the queue family earlier
        .queueFamilyIndexCount = 1,
        .pQueueFamilyIndices = &renderContext->graphicsQueueFamily,
        .preTransform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR, // not sure if this will work but oh well
        .compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR, // no transparent windows... FOR NOW
        .presentMode = VK_PRESENT_MODE_FIFO_KHR,
        .oldSwapchain = oldSwapchain,
    };
    vkCreateSwapchainKHR(device, &createInfo, renderContext->allocationCallbacks, &swapchain);
    // clean up old swapchain
    if (oldSwapchain != VK_NULL_HANDLE) {
        vkDestroySwapchainKHR(device, oldSwapchain, renderContext->allocationCallbacks);
    }

    // get images for swapchain
    {
        uint32_t imageCount = 0;
        check(vkGetSwapchainImagesKHR(device, swapchain, &imageCount, NULL));
        if (imageCount != RC_SWAPCHAIN_LENGTH) {
            char msg[300] = {0};
            snprintf(msg, 300, "Created swapchain has incorrect number of images: %u (should be %u)", imageCount, RC_SWAPCHAIN_LENGTH);
            exception_msg(msg);
        }
        printf("Swapchain image count: %i\n", imageCount);
        VkImage swapchainImages[RC_SWAPCHAIN_LENGTH];
        check(vkGetSwapchainImagesKHR(device, swapchain, &imageCount, swapchainImages));
        for (int i = 0; i < RC_SWAPCHAIN_LENGTH; ++i) {
            renderContext->images[i].swapchainImage = swapchainImages[i];
        }
        // swapchain image lifetime is controlled by the swapchain lifetime, so they must not be destroyed manually
    }

    // create image views for swapchain
    for (int i = 0; i < RC_SWAPCHAIN_LENGTH; ++i) {
        if (renderContext->images[i].swapchainImageView != VK_NULL_HANDLE) {
            vkDestroyImageView(device, renderContext->images[i].swapchainImageView, renderContext->allocationCallbacks);
            renderContext->images[i].swapchainImageView = VK_NULL_HANDLE;
        }
        VkImageViewCreateInfo imageViewCreateInfo = {
            .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
            .pNext = NULL,
            .flags = 0,
            .image = renderContext->images[i].swapchainImage,
            .viewType = VK_IMAGE_VIEW_TYPE_2D,
            .format = renderContext->surfaceFormat.format,
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
            device,
            &imageViewCreateInfo,
            //renderContext->allocationCallbacks,
            NULL,
            &view));
        renderContext->images[i].swapchainImageView = view;
    }

    renderContext->swapchain = swapchain;
    renderContext->swapchainExtent = extent;
}

void rc_size_change(RenderContext* context, uint32_t width, uint32_t height) {
    if (context->instance == VK_NULL_HANDLE) {
        // wait to init first
        return;
    }
    // todo:
    // check(vkWaitForFences(context->device, 1, &context->renderFence, true, 1000000000));
    rc_init_swapchain(context, width, height);
    // rc_init_framebuffers(context);
    // rc_init_pipelines(context);
}


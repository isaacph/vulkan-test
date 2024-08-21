#include "context.h"
#include "util.h"
#include <stdbool.h>
#include "../util/memory.h"
#include "util/backtrace.h"
#include <stdlib.h>
#include <stdio.h>

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
    uint32_t graphicsQueueFamily = renderContext->graphicsQueueFamily;

    // init command pools and buffers for each frame
    for (uint32_t index = 0; index < RC_SWAPCHAIN_LENGTH; ++index) {
        VkCommandPool commandPool = VK_NULL_HANDLE;
        VkCommandBuffer commandBuffer = VK_NULL_HANDLE;

        VkCommandPoolCreateInfo commandPoolCreateInfo = {
            .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
            .queueFamilyIndex = graphicsQueueFamily,
            .flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
            .pNext = NULL,
        };
        check(vkCreateCommandPool(device, &commandPoolCreateInfo, NULL, &commandPool));

        VkCommandBufferAllocateInfo cmdAllocInfo = {
            .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
            .pNext = NULL,
            .commandPool = commandPool,
            .commandBufferCount = 1,
            .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
        };
        check(vkAllocateCommandBuffers(device, &cmdAllocInfo, &commandBuffer));

        renderContext->frames[index].commandPool = commandPool;
        renderContext->frames[index].mainCommandBuffer = commandBuffer;
    }

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
    AllocatedImage drawImage = {
        .image = VK_NULL_HANDLE,
        .imageView = VK_NULL_HANDLE,
        .imageExtent = (VkExtent3D) { 0 },
        .imageFormat = VK_FORMAT_UNDEFINED,
    };
    VkExtent2D drawExtent = { 0 };

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
    vkCreateSwapchainKHR(device, &createInfo, NULL, &swapchain);
    // clean up old swapchain
    if (oldSwapchain != VK_NULL_HANDLE) {
        vkDestroySwapchainKHR(device, oldSwapchain, NULL);
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
            vkDestroyImageView(device, renderContext->images[i].swapchainImageView, NULL);
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

	//draw image size will match the window
	VkExtent3D drawImageExtent = {
		width,
		height,
		1
	};

	//hardcoding the draw format to 32 bit float
	drawImage.imageFormat = VK_FORMAT_R16G16B16A16_SFLOAT;
	drawImage.imageExtent = drawImageExtent;

	VkImageUsageFlags drawImageUsages = 0;
	drawImageUsages |= VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
	drawImageUsages |= VK_IMAGE_USAGE_TRANSFER_DST_BIT;
	drawImageUsages |= VK_IMAGE_USAGE_STORAGE_BIT;
	drawImageUsages |= VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;


	// //for the draw image, we want to allocate it from gpu local memory
	// VmaAllocationCreateInfo rimg_allocinfo = {};
	// rimg_allocinfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;
	// rimg_allocinfo.requiredFlags = VkMemoryPropertyFlags(VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

	// //allocate and create the image
	// vmaCreateImage(_allocator, &rimg_info, &rimg_allocinfo, &_drawImage.image, &_drawImage.allocation, nullptr);

    // // let's create a VkImage ourselves -> drawImage.image
	// VkImageCreateInfo rimg_info = image_create_info(drawImage.imageFormat, drawImageUsages, drawImageExtent);
    // VK_CHECK(vkCreateImage(device, &rimg_info, NULL, &drawImage.image));
    // // I think there's a way you're supposed to allocate memory and then create the image indirectly.
    // // I'm currently researching how to do that

	// //build a image-view for the draw image to use for rendering
	// VkImageViewCreateInfo rview_info = imageview_create_info(drawImage.imageFormat, drawImage.image, VK_IMAGE_ASPECT_COLOR_BIT);

	// VK_CHECK(vkCreateImageView(device, &rview_info, NULL, &drawImage.imageView));

	//add to deletion queues
	// _mainDeletionQueue.push_function([=]() {
	// 	vmaDestroyImage(_allocator, _drawImage.image, _drawImage.allocation);
	// });

    renderContext->drawImage = drawImage;
    renderContext->drawExtent = drawExtent;
    renderContext->swapchain = swapchain;
    renderContext->swapchainExtent = extent;


    // exercise: let's create some device memory, map it, write to it, flush it, invalidate it, then unmap it, all just because
    // start with checking for capabilities for host visible
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


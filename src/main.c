#include "util/memory.h"
#include "render/context.h"
#include <assert.h>
#include <stdio.h>
#include <util/backtrace.h>
#include <math.h>

typedef struct AllocationCleanup {
    VkDevice device;
    VkDeviceMemory allocation;
} AllocationCleanup;
void cleanup_allocation(void* user_ptr, sc_t id) {
    AllocationCleanup* ptr = (AllocationCleanup*) user_ptr;
    vkFreeMemory(ptr->device, ptr->allocation, NULL);
    free(ptr);
}

// I see this as something we can build out little by little.
// 1. One allocation per memory type, with reasonable size for allocation. No partial deallocation.
// 2. Multiple allocations per memory type, with reasonable size for each allocation. No partial deallocation.
// 3. Multiple allocations. Simple linear search allocation + simple deallocation tracking.
// 4. Multiple allocations. At this point we need a more advanced strategy for choosing allocation position. Possibly can fragment based on
//    how frequently we need to reallocate some specific memory allocation. Longer-term, larger allocations can get a space, and shorter-term
//    small allocations can get another space which uses a basic allocation strategy. Possibly could design to over-deallocate so that
//    frequent allocation is as stream-lined as possible for these smaller or shorter-term allocations. Could implement tracking of performance
//    too for specific applications so that we can make informed decisions on more complex designs.
typedef struct Allocations {
    VkDeviceMemory allocation[VK_MAX_MEMORY_TYPES];
    VkDeviceSize allocationOffset[VK_MAX_MEMORY_TYPES];
    VkDeviceMemory* toDeallocate; // ptr to array of length VK_MAX_MEMORY_TYPES. Make sure to add all allocations to this array too for cleanup
} Allocations;
typedef struct AllocationsCleanUp {
    VkDevice device;
    VkDeviceMemory* toDeallocate; // ptr to array of length VK_MAX_MEMORY_TYPES
} AllocationsCleanup;
// Another consideration is handling the lifetime of this thing. Every other Vulkan resource we can just assume we can allocate
// and deallocate like it's a stack. But I think this one works a bit differently. There are other Vulkan resources depending on this
// structure, so we need to make sure to deallocate after all of them deallocate. Is there anything that needs to be deallocated before
// this one though? Probably the device + instance at least. Going off that just making a universal memory deallocation to happen
// when device is destroyed
void cleanup_allocations(void* user_ptr, sc_t id) {
    AllocationsCleanup* ptr = (AllocationsCleanup*) user_ptr;
    for (int i = 0; i < VK_MAX_MEMORY_TYPES; ++i) {
        if (ptr->toDeallocate[i] != VK_NULL_HANDLE) {
            vkFreeMemory(ptr->device, ptr->toDeallocate[i], NULL);
        }
    }
    free(ptr);
}

typedef struct SecondSwapchainImageCleanup {
    VkDevice device;
    VkImage image;
    VkImageView imageView;
} SecondSwapchainImageCleanup;
void cleanup_second_swapchain_image(void* user_ptr, sc_t id) {
    SecondSwapchainImageCleanup* ptr = (SecondSwapchainImageCleanup*) user_ptr;
    vkDestroyImageView(ptr->device, ptr->imageView, NULL);
    vkDestroyImage(ptr->device, ptr->image, NULL);
    free(ptr);
}
typedef struct SecondSwapchainImageInit {
    VkPhysicalDevice physicalDevice;
    VkDevice device;
    VkImage drawImage; // current version of this image, or VK_NULL_HANDLE
    VkImageView drawImageView; // ignored if image is VK_NULL_HANDLE
    SecondSwapchainImageCleanup* drawImageCleanup; // current cleanup handle, or NULL
    uint32_t drawImageChosenMemoryType; // ignored if image is VK_NULL_HANDLE
    VkDeviceSize drawImageMemoryPosition; // ignored if image is VK_NULL_HANDLE
    VkExtent2D windowSize;
    Allocations* allocations;
    StaticCache* cleanup;
} SecondSwapchainImageInit;
typedef struct SecondSwapchainImage {
    VkImage drawImage; // new version of this image
    VkImageView drawImageView;
    SecondSwapchainImageCleanup* drawImageCleanup; // new version of cleanup handle
    uint32_t drawImageChosenMemoryType;
    VkDeviceSize drawImageMemoryPosition;
} SecondSwapchainImage;
SecondSwapchainImage rc_init_second_swapchain_image(SecondSwapchainImageInit params) {
    bool newImage = params.drawImage == VK_NULL_HANDLE;
    uint32_t drawImageChosenMemoryType = params.drawImageChosenMemoryType;
    VkDeviceSize drawImageMemoryPosition = params.drawImageMemoryPosition;

    VkImageUsageFlags drawImageUsages = 0;
    drawImageUsages |= VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
    drawImageUsages |= VK_IMAGE_USAGE_TRANSFER_DST_BIT;
    drawImageUsages |= VK_IMAGE_USAGE_STORAGE_BIT;
    drawImageUsages |= VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    VkExtent3D extent = {
        .width = params.windowSize.width,
        .height = params.windowSize.height,
        .depth = 1,
    };
    VkFormat imageFormat = VK_FORMAT_R16G16B16A16_SFLOAT;
    VkImageCreateInfo createInfo = rc_image_create_info(imageFormat, drawImageUsages, extent);

    SecondSwapchainImageCleanup* drawImageCleanup = params.drawImageCleanup;
    if (newImage) {
        drawImageCleanup = checkMalloc(malloc(sizeof(SecondSwapchainImageCleanup)));
        drawImageCleanup->device = params.device;
    }
    VkImage drawImage = params.drawImage;
    if (newImage) {
        vkDestroyImage(params.device, drawImage, NULL);
    }
    check(vkCreateImage(params.device, &createInfo, NULL, &drawImage));
    drawImageCleanup->image = drawImage;
    VkMemoryRequirements imageMemoryRequirements = { 0 };
    vkGetImageMemoryRequirements(params.device, drawImage, &imageMemoryRequirements);

    const VkDeviceSize maxPossibleSize = 131 * 1024 * 1024;
    // look through memoryTypeBits for a memory type that has sufficient memory and is DEVICE_LOCAL
    // then allocate VkDeviceMemory from that memory type
    VkDeviceSize requiredSize = 256 * 1024 * 1024;
    uint32_t chosenMemoryTypeIndex = UINT32_MAX;
    VkPhysicalDeviceMemoryProperties properties = { 0 };
    vkGetPhysicalDeviceMemoryProperties(params.physicalDevice, &properties);
    for (int i = 0; i < properties.memoryTypeCount; ++i) {
        if ((imageMemoryRequirements.memoryTypeBits & (1 << i)) != 0) {
            uint32_t flags = properties.memoryTypes[i].propertyFlags;
            if ((VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT & flags) != 0) {
                uint32_t heapIndex = properties.memoryTypes[i].heapIndex;
                VkDeviceSize heapSize = properties.memoryHeaps[heapIndex].size; 
                if (heapSize >= requiredSize) {
                    chosenMemoryTypeIndex = i;
                    printf("chosen type #%d heapIndex: %u, flags: %x\n", i,
                            properties.memoryTypes[i].heapIndex, properties.memoryTypes[i].propertyFlags);
                    break;
                }
            }
        }
    }
    assert(chosenMemoryTypeIndex != UINT32_MAX);
    if (newImage) {
        VkMemoryAllocateInfo allocateInfo = {
            .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
            .pNext = NULL,
            .allocationSize = requiredSize,
            .memoryTypeIndex = chosenMemoryTypeIndex,
        };
        VkDeviceMemory deviceMemory = VK_NULL_HANDLE;
        check(vkAllocateMemory(params.device, &allocateInfo, NULL, &deviceMemory));
        check(vkBindImageMemory(params.device, drawImage, deviceMemory, 0));
        // normally we'd check if we already had an allocation before allocating here, but for our purposes we know we are
        // the first allocation
        params.allocations->allocation[chosenMemoryTypeIndex] = deviceMemory;
        params.allocations->allocationOffset[chosenMemoryTypeIndex] = maxPossibleSize;
        params.allocations->toDeallocate[chosenMemoryTypeIndex] = deviceMemory; // sets it up for cleanup
        drawImageChosenMemoryType = chosenMemoryTypeIndex;
        drawImageMemoryPosition = 0;
    } else {
        assert(chosenMemoryTypeIndex == drawImageChosenMemoryType);
        // how do we reallocate if the size could be different?????
        // best idea right now: think of biggest possible screen size and just reserve that much lol
        // lmfao if we take 4x the biggest screen size we get 7680 * 4320 * 4 = 132710400 ~= 132 MB
        // lets just give a whole memory block or something?
        VkDeviceMemory deviceMemory = params.allocations->allocation[chosenMemoryTypeIndex];
        assert(deviceMemory != VK_NULL_HANDLE);
        check(vkBindImageMemory(params.device, drawImage, deviceMemory, 0));
    }

    if (newImage) {
        vkDestroyImageView(params.device, params.drawImageView, NULL);
    }
    VkImageViewCreateInfo imageViewInfo = rc_imageview_create_info(imageFormat, drawImage, VK_IMAGE_ASPECT_COLOR_BIT);
    VkImageView drawImageView = VK_NULL_HANDLE;
    check(vkCreateImageView(params.device, &imageViewInfo, NULL, &drawImageView));
    drawImageCleanup->imageView = drawImageView;

    if (newImage) {
        StaticCache_add(params.cleanup, cleanup_second_swapchain_image, (void*) drawImageCleanup);
    }

    return (SecondSwapchainImage) {
        .drawImage = drawImage,
        .drawImageView = drawImageView,
        .drawImageCleanup = drawImageCleanup,
        .drawImageChosenMemoryType = drawImageChosenMemoryType,
        .drawImageMemoryPosition = drawImageMemoryPosition,
    };
}

int main() {
    StaticCache cleanup = StaticCache_init(1000);
    VkInstance instance = VK_NULL_HANDLE;
    VkSurfaceKHR surface = VK_NULL_HANDLE;
    WindowHandle windowHandle = { 0 };
    VkDevice device = VK_NULL_HANDLE;
    VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
    VkExtent2D size = { 0 };
    VkSurfaceFormatKHR surfaceFormat = { 0 };
    uint32_t graphicsQueueFamily = 0;
    VkQueue graphicsQueue = VK_NULL_HANDLE;
    VkSwapchainKHR swapchain;
    SwapchainImageData swapchainImages[RC_SWAPCHAIN_LENGTH];
    FrameData frames[FRAME_OVERLAP];
    sc_t swapchainCleanupHandle = SC_ID_NONE;

    VkImage drawImage = VK_NULL_HANDLE; // the image we draw directly to, copied to swapchain
    VkImageView drawImageView = VK_NULL_HANDLE;
    SecondSwapchainImageCleanup* drawImageCleanup = NULL;
    uint32_t drawImageChosenMemoryType = 0; // we need to be able to reallocate this memory on resize so record its properties here
    VkDeviceSize drawImageMemoryPosition = 0;

    Allocations allocations = { 0 };
    for (int i = 0; i < VK_MAX_MEMORY_TYPES; ++i) {
        allocations.allocation[i] = VK_NULL_HANDLE;
        allocations.allocationOffset[i] = 0;
    }
    allocations.toDeallocate = calloc(VK_MAX_MEMORY_TYPES, sizeof(VkDeviceMemory));
    {
        PFN_vkGetInstanceProcAddr proc_addr = rc_proc_addr();
        InitInstance init = rc_init_instance(proc_addr, false, &cleanup);
        instance = init.instance;
        assert(instance != VK_NULL_HANDLE);
    }
    {
        const char* title = "Test window! \xF0\x9F\x87\xBA\xF0\x9F\x87\xB8";
        InitSurfaceParams params = {
            .instance = instance,
            .title = title,
            .titleLength = strlen(title),
            .size = DEFAULT_SURFACE_SIZE,
            .headless = false,
        };
        InitSurface ret = rc_init_surface(params, &cleanup);
        surface = ret.surface;
        windowHandle = ret.windowHandle;
        size = ret.size;
        assert(surface != NULL);
        assert(size.width != 0);
        assert(size.height != 0);
        assert(size.width != DEFAULT_SURFACE_SIZE.width && size.height != DEFAULT_SURFACE_SIZE.height);
    }
    {
        // so we need to refactor the logic so that surface format is chosen when physical device is chosen
        InitDeviceParams params = {
            .instance = instance,
            .surface = surface,
        };
        InitDevice ret = rc_init_device(params, &cleanup);
        device = ret.device;
        surfaceFormat = ret.surfaceFormat;
        graphicsQueueFamily = ret.graphicsQueueFamily;
        physicalDevice = ret.physicalDevice;
        graphicsQueue = ret.graphicsQueue;
        assert(ret.device != NULL);
        AllocationsCleanup* allocCleanup = checkMalloc(malloc(sizeof(AllocationsCleanup)));
        *allocCleanup = (AllocationsCleanup) {
            .device = ret.device,
            .toDeallocate = allocations.toDeallocate,
        };
        StaticCache_add(&cleanup, cleanup_allocations, allocCleanup);
    }
    {
        InitSwapchainParams params = {
            .extent = size,

            .device = device,
            .physicalDevice = physicalDevice,
            .surface = surface,
            .surfaceFormat = surfaceFormat,
            .graphicsQueueFamily = graphicsQueueFamily,

            .oldSwapchain = VK_NULL_HANDLE,
            .swapchainCleanupHandle = swapchainCleanupHandle,
        };
        InitSwapchain ret = rc_init_swapchain(params, &cleanup);
        swapchain = ret.swapchain;
        for (int i = 0; i < RC_SWAPCHAIN_LENGTH; ++i) {
            swapchainImages[i] = ret.images[i];
        }
        swapchainCleanupHandle  = ret.swapchainCleanupHandle;
        assert(swapchain != NULL);
    }
    {
        InitLoopParams params = {
            .device = device,
            .graphicsQueueFamily = graphicsQueueFamily,
        };
        InitLoop ret = rc_init_loop(params, &cleanup);
        for (int i = 0; i < FRAME_OVERLAP; ++i) {
            assert(ret.frames[i].commandPool != VK_NULL_HANDLE);
            frames[i] = ret.frames[i];
        }
    }
    // init device memory allocation
    // {
    //     // is it possible to allocate some of each memory requirement first?
    //     // so it seems this simple:
    //     // I want to be able to allocate memory in advance.
    //     // The implementation wants to require some specific sets of memory types that we can even put images in though
    //     // the options are to either find out what types those are in advance by making sets of image types that we expect to use
    //     // that we can use to anticipate required types so that the allocations are already there once we need them
    //     // or we can just wait on allocation until a specific type is required. then we would do the allocation
    //     //
    //     // waiting on the allocation until a type is required is better because 1. if the types change over program lifetime no
    //     // extra handling might be needed
    //     //
    //     // the question is if that 1. is a valid reason. checking the documentation for "can vkGetImageMemoryRequirements2 return
    //     // different results with the same inputs"
    //     //
    //     //
    //     // so based on the docs, as long as I'm not using features that are too esoteric, types will not change over program lifetime
    //     //
    //     // so why might i want to wait on allocation until a type is required, if we assume we can figure that out at the start?
    //     // well not waiting means I have to know in advance what image formats I'm going to need, and allocate for each of those.
    //     // (and buffer formats). keeping track of this sounds like a lot of work I'm trying to minimize until absolutely necessary
    //     //
    //     // what's bad about waiting? well if I wait then I have to basically have an array of an allocation for each memoryType
    //     // it may even need to be a 2d array if we want long term scalability
    //     // it means i have to do some framework writing here right away.
    //     // normally the point of frameworks is to align to changing programmer side requirements. we find this to be overkill a lot of the time,
    //     // hence we are trying this type of coding we're doing here where we are trying to minimize frameworking
    //     // but. now we have to align to changing runtime device requirements.
    //     //
    //     // can we just go implement a really naive solution for now and make it better later?

    //     // look through memoryTypeBits for a memory type that has sufficient memory and is DEVICE_LOCAL
    //     // then allocate VkDeviceMemory from that memory type
    //     VkDeviceSize requiredSize = 256 * 1024 * 1024;
    //     uint32_t chosenMemoryTypeIndex = UINT32_MAX;
    //     VkPhysicalDeviceMemoryProperties properties = { 0 };
    //     vkGetPhysicalDeviceMemoryProperties(physicalDevice, &properties);
    //     for (int i = 0; i < properties.memoryTypeCount; ++i) {
    //         if ((imageMemoryRequirements.memoryTypeBits & (1 << i)) != 0) {
    //             uint32_t flags = properties.memoryTypes[i].propertyFlags;
    //             if ((VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT & flags) != 0) {
    //                 uint32_t heapIndex = properties.memoryTypes[i].heapIndex;
    //                 VkDeviceSize heapSize = properties.memoryHeaps[heapIndex].size; 
    //                 if (heapSize >= requiredSize) {
    //                     chosenMemoryTypeIndex = i;
    //                     printf("chosen type #%d heapIndex: %u, flags: %x\n", i,
    //                             properties.memoryTypes[i].heapIndex, properties.memoryTypes[i].propertyFlags);
    //                     break;
    //                 }
    //             }
    //         }
    //     }
    //     assert(chosenMemoryTypeIndex != UINT32_MAX);
    //     VkMemoryAllocateInfo allocateInfo = {
    //         .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
    //         .pNext = NULL,
    //         .allocationSize = requiredSize,
    //         .memoryTypeIndex = chosenMemoryTypeIndex,
    //     };
    //     check(vkAllocateMemory(device, &allocateInfo, NULL, &allocation));
    //     check(vkBindImageMemory(device, image, allocation, 0));
    //     cleanupObject->memory = allocation;
    // }
    // init the image that we draw to
    {
        SecondSwapchainImageInit params = {
            .physicalDevice = physicalDevice,
            .device = device,
            .drawImage = VK_NULL_HANDLE,
            .drawImageView = VK_NULL_HANDLE,
            .drawImageCleanup = NULL,
            .drawImageChosenMemoryType = 0,
            .drawImageMemoryPosition = 0,
            .windowSize = size,
            .allocations = &allocations,
            .cleanup = &cleanup,
        };
        SecondSwapchainImage ret = rc_init_second_swapchain_image(params);
        drawImage = ret.drawImage;
        drawImageView = ret.drawImageView;
        drawImageCleanup = ret.drawImageCleanup;
        drawImageChosenMemoryType = ret.drawImageChosenMemoryType;
        drawImageMemoryPosition = ret.drawImageMemoryPosition;
    }
    {
        DrawParams params = {
            .device = device,
            .graphicsQueue = graphicsQueue,
            .swapchain = swapchain,
            .swapchainImages = { 0 },
        };
        for (int i = 0; i < RC_SWAPCHAIN_LENGTH; ++i) {
            params.swapchainImages[i] = swapchainImages[i];
        }

        bool running = true;
        int frameNumber = 0;
        // raise this limit to test resizing manually
        while (running) {
            WindowUpdate update = rc_window_update(&windowHandle);
            running = !update.windowClosed;
            if (update.resize) {
                printf("new size: %d x %d\n", size.width, size.height);
                size = update.newSize;
                if (size.width < 0 || size.height < 0) {
                    size = (VkExtent2D) {
                        .width = 0,
                        .height = 0,
                    };
                }
                printf("%dx%d\n", size.width, size.height);
                if (size.width * size.height > 0) {
                    InitSwapchainParams swapchainParams = {
                        .extent = size,

                        .device = device,
                        .physicalDevice = physicalDevice,
                        .surface = surface,
                        .surfaceFormat = surfaceFormat,
                        .graphicsQueueFamily = graphicsQueueFamily,

                        .oldSwapchain = swapchain,
                        .swapchainCleanupHandle = swapchainCleanupHandle,
                    };
                    InitSwapchain ret = rc_init_swapchain(swapchainParams, &cleanup);
                    swapchain = ret.swapchain;
                    swapchainCleanupHandle = ret.swapchainCleanupHandle;
                    for (int i = 0; i < RC_SWAPCHAIN_LENGTH; ++i) {
                        swapchainImages[i] = ret.images[i];
                    }
                    // YOU MUST make sure to update swapchain in context!
                    params.swapchain = swapchain;
                    for (int i = 0; i < RC_SWAPCHAIN_LENGTH; ++i) {
                        params.swapchainImages[i] = ret.images[i];
                    }

                    // we also need to accept second swapchain
                    {
                        SecondSwapchainImageInit params = {
                            .physicalDevice = physicalDevice,
                            .device = device,
                            .drawImage = drawImage,
                            .drawImageView = drawImageView,
                            .drawImageCleanup = drawImageCleanup,
                            .drawImageChosenMemoryType = drawImageChosenMemoryType,
                            .drawImageMemoryPosition = drawImageMemoryPosition,
                            .windowSize = size,
                            .allocations = &allocations,
                            .cleanup = &cleanup,
                        };
                        SecondSwapchainImage ret = rc_init_second_swapchain_image(params);
                        drawImage = ret.drawImage;
                        drawImageView = ret.drawImageView;
                        drawImageCleanup = ret.drawImageCleanup;
                        drawImageChosenMemoryType = ret.drawImageChosenMemoryType;
                        drawImageMemoryPosition = ret.drawImageMemoryPosition;
                    }
                }
            }

            if (update.shouldDraw && size.width * size.height > 0) {
                frameNumber++;
                params.frame = frames[frameNumber % 2];
                params.color = fabs(sin(frameNumber / 120.f));
                params.swapchainExtent = size;
                params.drawImageExtent = size;
                params.drawImage = drawImage;
                rc_draw(params);
            }
        }
    }

    StaticCache_clean_up(&cleanup);
}

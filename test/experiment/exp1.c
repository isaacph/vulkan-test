#include "util/memory.h"
#include "render/context.h"
#include <assert.h>
// #include <dlfcn.h>
#include <stdio.h>
#include <util/backtrace.h>
#include <math.h>

typedef struct TestImageFreeMemoryCleanup {
    VkDevice device;
    VkImage image;
    VkDeviceMemory memory;
    VkImageView imageView;
} TestImageFreeMemoryCleanup;
void test_run_free_memory(void* user_ptr, sc_t id) {
    TestImageFreeMemoryCleanup* ptr = (TestImageFreeMemoryCleanup*) user_ptr;
    vkDestroyImageView(ptr->device, ptr->imageView, NULL);
    vkFreeMemory(ptr->device, ptr->memory, NULL);
    vkDestroyImage(ptr->device, ptr->image, NULL);
    free(ptr);
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

        // extra swapchain image init logic
        VkImageUsageFlags drawImageUsages = 0;
        drawImageUsages |= VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
        drawImageUsages |= VK_IMAGE_USAGE_TRANSFER_DST_BIT;
        drawImageUsages |= VK_IMAGE_USAGE_STORAGE_BIT;
        drawImageUsages |= VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
        VkExtent3D extent = {
            .width = 100,
            .height = 100,
            .depth = 1,
        };
        VkFormat imageFormat = VK_FORMAT_R16G16B16A16_SFLOAT;
        VkImageCreateInfo createInfo = rc_image_create_info(imageFormat, drawImageUsages, extent);

        TestImageFreeMemoryCleanup* cleanupObject = checkMalloc(malloc(sizeof(TestImageFreeMemoryCleanup)));
        cleanupObject->device = device;
        VkImage image = VK_NULL_HANDLE;
        check(vkCreateImage(device, &createInfo, NULL, &image));
        cleanupObject->image = image;
        VkMemoryRequirements imageMemoryRequirements = { 0 };
        vkGetImageMemoryRequirements(device, image, &imageMemoryRequirements);

        // look through memoryTypeBits for a memory type that has sufficient memory and is DEVICE_LOCAL
        // then allocate VkDeviceMemory from that memory type
        VkDeviceSize requiredSize = 256 * 1024 * 1024;
        uint32_t chosenMemoryTypeIndex = UINT32_MAX;
        VkPhysicalDeviceMemoryProperties properties = { 0 };
        vkGetPhysicalDeviceMemoryProperties(physicalDevice, &properties);
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
        VkMemoryAllocateInfo allocateInfo = {
            .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
            .pNext = NULL,
            .allocationSize = requiredSize,
            .memoryTypeIndex = chosenMemoryTypeIndex,
        };
        VkDeviceMemory deviceMemory = VK_NULL_HANDLE;
        check(vkAllocateMemory(device, &allocateInfo, NULL, &deviceMemory));
        check(vkBindImageMemory(device, image, deviceMemory, 0));
        cleanupObject->memory = deviceMemory;

        VkImageViewCreateInfo imageViewInfo = rc_imageview_create_info(imageFormat, image, VK_IMAGE_ASPECT_COLOR_BIT);
        VkImageView imageView = VK_NULL_HANDLE;
        check(vkCreateImageView(device, &imageViewInfo, NULL, &imageView));
        cleanupObject->imageView = imageView;

        StaticCache_add(&cleanup, test_run_free_memory, (void*) cleanupObject);
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
            ++frameNumber;
            WindowUpdate update = rc_window_update(&windowHandle);
            running = !update.windowClosed;
            if (update.resize) {
                printf("new size: %d x %d\n", size.width, size.height);
                size = update.newSize;
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
                // YOU MUST make sure to update the params!
                params.swapchain = swapchain;
                for (int i = 0; i < RC_SWAPCHAIN_LENGTH; ++i) {
                    params.swapchainImages[i] = ret.images[i];
                }
            }

            if (update.shouldDraw) {
                params.frame = frames[frameNumber % 2];
                params.color = fabs(sin(frameNumber / 120.f));
                printf("%f\n", params.color);
                rc_draw(params);
            }
        }
    }

    StaticCache_clean_up(&cleanup);
}

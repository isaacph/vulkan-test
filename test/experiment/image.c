#include <util/memory.h>
#include <render/context.h>
#include <render/util.h>
#include <assert.h>
#include <stdio.h>
#include <unity.h>
#include <util/backtrace.h>
#include <stdbool.h>
#include <vulkan/vulkan_core.h>

StaticCache cleanup;
VkInstance instance = VK_NULL_HANDLE;
VkSurfaceKHR surface = VK_NULL_HANDLE;
WindowHandle windowHandle = {0};
VkPhysicalDevice physicalDevice;
VkDevice device;
VkQueue graphicsQueue;
uint32_t graphicsQueueFamily;

void setUp(void) {
    init_exceptions(false);
    cleanup = StaticCache_init(1000);
    {
        PFN_vkGetInstanceProcAddr proc_addr = rc_proc_addr();
        InitInstance init = rc_init_instance(proc_addr, false, &cleanup);
        instance = init.instance;
        assert(instance != VK_NULL_HANDLE);
    }
    {
        // for now, we don't have a way to init without making a hidden window
        // later we'll make a more minimal init function for testing
        const char* title = "Invisible window";
        InitSurfaceParams params = {
            .instance = instance,
            .title = title,
            .titleLength = strlen(title),
            .size = DEFAULT_SURFACE_SIZE,
            .headless = true,
        };
        InitSurface ret = rc_init_surface(params, &cleanup);
        surface = ret.surface;
        windowHandle = ret.windowHandle;
        assert(surface != NULL);
    }
    {
        InitDeviceParams params = {
            .instance = instance,
            .surface = surface,
        };
        InitDevice ret = rc_init_device(params, &cleanup);
        physicalDevice = ret.physicalDevice;
        device = ret.device;
        graphicsQueue = ret.graphicsQueue;
        graphicsQueueFamily = ret.graphicsQueueFamily;
        assert(device != NULL);
    }
}
void tearDown(void) {
    StaticCache_clean_up(&cleanup);
}

void test_log(void) {
    VkPhysicalDeviceMemoryProperties properties = { 0 };
    vkGetPhysicalDeviceMemoryProperties(physicalDevice, &properties);
    printf("memHeapCount: %d, memTypeCount: %d\n", properties.memoryHeapCount, properties.memoryTypeCount);
    for (int i = 0; i < properties.memoryHeapCount; ++i) {
        uint32_t flags = properties.memoryHeaps[i].flags;
        printf("heap #%d size: %llu, flags: %x\n", i, properties.memoryHeaps[i].size, properties.memoryHeaps[i].flags);
        if ((VK_MEMORY_HEAP_DEVICE_LOCAL_BIT & flags) != 0) {
            printf("has VK_MEMORY_HEAP_DEVICE_LOCAL_BIT\n");
        }
        if ((VK_MEMORY_HEAP_MULTI_INSTANCE_BIT & flags) != 0) {
            printf("has VK_MEMORY_HEAP_MULTI_INSTANCE_BIT\n");
        }
        if ((VK_MEMORY_HEAP_MULTI_INSTANCE_BIT & flags) != 0) {
            printf("has VK_MEMORY_HEAP_MULTI_INSTANCE_BIT\n");
        }
        if ((VK_MEMORY_HEAP_MULTI_INSTANCE_BIT_KHR & flags) != 0) {
            printf("has VK_MEMORY_HEAP_MULTI_INSTANCE_BIT_KHR\n");
        }
    }
    for (int i = 0; i < properties.memoryTypeCount; ++i) {
        uint32_t flags = properties.memoryTypes[i].propertyFlags;
        printf("type #%d heapIndex: %u, flags: %x\n", i, properties.memoryTypes[i].heapIndex, properties.memoryTypes[i].propertyFlags);
        if ((VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT & flags) != 0) {
            printf("has VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT\n");
        }
        if ((VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT & flags) != 0) {
            printf("has VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT\n");
        }
        if ((VK_MEMORY_PROPERTY_PROTECTED_BIT & flags) != 0) {
            printf("has VK_MEMORY_PROPERTY_PROTECTED_BIT\n");
        }
        if ((VK_MEMORY_PROPERTY_HOST_COHERENT_BIT & flags) != 0) {
            printf("has VK_MEMORY_PROPERTY_HOST_COHERENT_BIT\n");
        }
        if ((VK_MEMORY_PROPERTY_HOST_CACHED_BIT & flags) != 0) {
            printf("has VK_MEMORY_PROPERTY_HOST_CACHED_BIT\n");
        }
        if ((VK_MEMORY_PROPERTY_LAZILY_ALLOCATED_BIT & flags) != 0) {
            printf("has VK_MEMORY_PROPERTY_LAZILY_ALLOCATED_BIT\n");
        }
        if ((VK_MEMORY_PROPERTY_PROTECTED_BIT & flags) != 0) {
            printf("has VK_MEMORY_PROPERTY_PROTECTED_BIT\n");
        }
        if ((VK_MEMORY_PROPERTY_DEVICE_COHERENT_BIT_AMD & flags) != 0) {
            printf("has VK_MEMORY_PROPERTY_DEVICE_COHERENT_BIT_AMD\n");
        }
        if ((VK_MEMORY_PROPERTY_DEVICE_UNCACHED_BIT_AMD & flags) != 0) {
            printf("has VK_MEMORY_PROPERTY_DEVICE_UNCACHED_BIT_AMD\n");
        }
        if ((VK_MEMORY_PROPERTY_RDMA_CAPABLE_BIT_NV & flags) != 0) {
            printf("has VK_MEMORY_PROPERTY_RDMA_CAPABLE_BIT_NV\n");
        }
    }
}

void test_physical_device_limits(void) {
}

typedef struct TestImageFreeMemoryCleanup {
    VkDevice device;
    VkImage image;
    VkDeviceMemory memory;
    VkImageView imageView;
} TestImageFreeMemoryCleanup;
void test_image_free_memory(void* user_ptr, sc_t id) {
    TestImageFreeMemoryCleanup* ptr = (TestImageFreeMemoryCleanup*) user_ptr;
    vkDestroyImageView(ptr->device, ptr->imageView, NULL);
    vkFreeMemory(ptr->device, ptr->memory, NULL);
    vkDestroyImage(ptr->device, ptr->image, NULL);
    free(ptr);
}
void test_image(void) {
    StaticCache localCleanup = StaticCache_init(1024);
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

    StaticCache_add(&localCleanup, test_image_free_memory, (void*) cleanupObject);

	VkClearColorValue clearValue = { { 1.0f, 0.0f, 0.0f, 1.0f } };
    // ok thing i'm hung up on as i go to sleep is this file was for testing with images
    // now it feels like the best thing to do is either write a script to figure out how to write my clear color image
    // to host memory then write to like file, or make image.c work like init.c and just make a render loop like before
    // idk what's best here tbh
	// VkImageSubresourceRange clearRange = rc_basic_image_subresource_range(VK_IMAGE_ASPECT_COLOR_BIT);
	// vkCmdClearColorImage(cmd, dest, VK_IMAGE_LAYOUT_GENERAL, &clearValue, 1, &clearRange);

    StaticCache_clean_up(&localCleanup);
}

void test_overallocate(void) {
    // VkMemoryAllocateInfo info = {
    //     .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
    //     .pNext = NULL,
    //     .allocationSize = 0,
    //     .memoryTypeIndex = 0,
    // };
    // VkDeviceMemory memory = { 0 };
    // VkResult result = vkAllocateMemory(device, &info, NULL, &memory);
    // printf("Allocate result: %d\n", result);
}

int main() {
    UNITY_BEGIN();
    RUN_TEST(test_log);
    RUN_TEST(test_image);
    RUN_TEST(test_overallocate);
    return UNITY_END();
}



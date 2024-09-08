#include "util/memory.h"
#include "render/context.h"
#include <assert.h>
// #include <dlfcn.h>
#include <stdio.h>
#include <unity.h>
#include <util/backtrace.h>

static void calc_VK_API_VERSION(uint32_t version, char* out_memory, uint32_t out_len) {
    assert(out_len >= 64);
    uint32_t variant = VK_API_VERSION_VARIANT(version);
    uint32_t major = VK_API_VERSION_MAJOR(version);
    uint32_t minor = VK_API_VERSION_MINOR(version);
    uint32_t patch = VK_API_VERSION_PATCH(version);
    if (variant != 0) {
        snprintf(out_memory, out_len, "Variant %u: %u.%u.%u", variant, major, minor, patch);
    } else {
        snprintf(out_memory, out_len, "%u.%u.%u", major, minor, patch);
    }
}

void setUp(void) {}
void tearDown(void) {}

// void test_minimal_instance_versions(void) {
//     void* vulkan_handle = dlopen("libvulkan.so", RTLD_LAZY);
//     assert(vulkan_handle != NULL);
//     PFN_vkGetInstanceProcAddr fp_vkGetInstanceProcAddr = (PFN_vkGetInstanceProcAddr) dlsym(vulkan_handle, "vkGetInstanceProcAddr");
//     assert(fp_vkGetInstanceProcAddr != NULL);
//     PFN_vkEnumerateInstanceVersion fp_vkEnumerateInstanceVersion = 
//         (PFN_vkEnumerateInstanceVersion) fp_vkGetInstanceProcAddr(VK_NULL_HANDLE, "vkEnumerateInstanceVersion");
//     assert(fp_vkEnumerateInstanceVersion != NULL);
//     uint32_t version;
//     VkResult res = fp_vkEnumerateInstanceVersion(&version);
//     assert(res == VK_SUCCESS);
//     char vk_api_version[64];
//     calc_VK_API_VERSION(version, vk_api_version, 64);
//     printf("Vulkan instance version %s\n", vk_api_version);
// }

void test_init_proc_addr(void) {
    assert(rc_proc_addr() != NULL);
}

void test_init_instance(void) {
    StaticCache cleanup = StaticCache_init(1000);
    VkInstance instance = VK_NULL_HANDLE;

    void* p = vkEnumerateInstanceVersion;
    {
        PFN_vkGetInstanceProcAddr proc_addr = rc_proc_addr();
        InitInstance init = rc_init_instance(proc_addr, false, &cleanup);
        instance = init.instance;
        assert(instance != VK_NULL_HANDLE);
    }
    StaticCache_clean_up(&cleanup);
}

// this test should flash because we have headless = false
void test_init_surface(void) {
    VkInstance instance = VK_NULL_HANDLE;
    StaticCache cleanup = StaticCache_init(1000);

    {
        PFN_vkGetInstanceProcAddr proc_addr = rc_proc_addr();
        InitInstance init = rc_init_instance(proc_addr, false, &cleanup);
        instance = init.instance;
        assert(instance != VK_NULL_HANDLE);
    }
    {
        const char* title = "Test window! \xF0\x9F\x87\xBA\xF0\x9F\x87\xB8 (US flag)";
        InitSurfaceParams params = {
            .instance = instance,
            .title = title,
            .titleLength = strlen(title),
            .size = DEFAULT_SURFACE_SIZE,
            .headless = false,
        };
        InitSurface ret = rc_init_surface(params, &cleanup);
        assert(ret.surface != VK_NULL_HANDLE);
    }
    StaticCache_clean_up(&cleanup);
}

void test_init_device(void) {
    StaticCache cleanup = StaticCache_init(1000);
    VkInstance instance = VK_NULL_HANDLE;
    VkSurfaceKHR surface = VK_NULL_HANDLE;
    WindowHandle windowHandle = {0};
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
            .headless = true,
        };
        InitSurface ret = rc_init_surface(params, &cleanup);
        surface = ret.surface;
        windowHandle = ret.windowHandle;
        assert(surface != NULL);
    }
    {
        // so we need to refactor the logic so that surface format is chosen when physical device is chosen
        InitDeviceParams params = {
            .instance = instance,
            .surface = surface,
        };
        InitDevice ret = rc_init_device(params, &cleanup);
        assert(ret.device != NULL);
    }
    StaticCache_clean_up(&cleanup);
}

void test_init_swapchain(void) {
    StaticCache cleanup = StaticCache_init(1000);
    VkInstance instance = VK_NULL_HANDLE;
    VkSurfaceKHR surface = VK_NULL_HANDLE;
    WindowHandle windowHandle = { 0 };
    VkDevice device = VK_NULL_HANDLE;
    VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
    VkExtent2D size = { 0 };
    VkSurfaceFormatKHR surfaceFormat = { 0 };
    uint32_t graphicsQueueFamily = 0;
    // VkQueue queue = VK_NULL_HANDLE;
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
            .headless = true,
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
            .oldImages = { 0 },
            .swapchainCleanupHandle = SC_ID_NONE,
        };
        InitSwapchain ret = rc_init_swapchain(params, &cleanup);
        assert(ret.swapchain != NULL);
    }
    StaticCache_clean_up(&cleanup);
}

int main() {
    init_exceptions(false);
    UNITY_BEGIN();
    // RUN_TEST(test_minimal_instance_versions);
    RUN_TEST(test_init_proc_addr);
    RUN_TEST(test_init_instance);
    RUN_TEST(test_init_surface);
    RUN_TEST(test_init_device);
    RUN_TEST(test_init_swapchain);
    return UNITY_END();
}


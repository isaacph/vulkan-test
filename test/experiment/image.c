#include <util/memory.h>
#include <render/context.h>
#include <assert.h>
#include <stdio.h>
#include <unity.h>
#include <util/backtrace.h>

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

void test_init(void) {
}

int main() {
    UNITY_BEGIN();
    RUN_TEST(test_init);
    return UNITY_END();
}



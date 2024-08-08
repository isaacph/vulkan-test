#include <stdio.h>
#include <render/context.h>
#include <unity.h>

StaticCache cleanup;

void setUp(void) {
    cleanup = StaticCache_init(1000);
}
void tearDown(void) {
    StaticCache_clean_up(&cleanup);
}

void test_init_instance(void) {
    VkInstance instance = VK_NULL_HANDLE;

    {
        PFN_vkGetInstanceProcAddr proc_addr = rc_proc_addr();
        InitInstance init = rc_init_instance(proc_addr, false, &cleanup);
        instance = init.instance;
    }
}

void test_init_surface(void) {
    VkInstance instance = VK_NULL_HANDLE;

    {
        PFN_vkGetInstanceProcAddr proc_addr = rc_proc_addr();
        InitInstance init = rc_init_instance(proc_addr, false, &cleanup);
        instance = init.instance;
    }
    {
    }
}

void test_init_device(void) {
    VkInstance instance = VK_NULL_HANDLE;
    {
        PFN_vkGetInstanceProcAddr proc_addr = rc_proc_addr();
        InitInstance init = rc_init_instance(proc_addr, false, &cleanup);
        instance = init.instance;
    }
    // prereq: surface
    // {
    //     InitDeviceParams params = {
    //         .instance = instance,
    //         .surface = 
    //     };
    //     InitDevice init = rc_init_device(
    // }
}


int main() {
    init_exceptions(false);
    UNITY_BEGIN();
    RUN_TEST(test_init_instance);
    return UNITY_END();
}


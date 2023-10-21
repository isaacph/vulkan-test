#include <stdio.h>
#include <windows.h>
#include <vulkan/vulkan_core.h>

// actually it doesn't segfault
void segfault() {
    // https://github.com/charles-lunarg/vk-bootstrap/blob/main/src/VkBootstrap.cpp#L61
    // https://learn.microsoft.com/en-us/windows/win32/api/libloaderapi/nf-libloaderapi-loadlibrarya
    HMODULE library;
    library = LoadLibraryA(TEXT("vulkan-1.dll"));
    if (library == NULL) {
        printf("vulkan-1.dll was not found\n");
        exit(1);
    }

    PFN_vkGetInstanceProcAddr fp_vkGetInstanceProcAddr;
    fp_vkGetInstanceProcAddr = (PFN_vkGetInstanceProcAddr) GetProcAddress(library, "vkGetInstanceProcAddr");
    if (fp_vkGetInstanceProcAddr == NULL) {
        printf("Error\n");
        exit(1);
    }
    printf("Found loader func: %p\n", fp_vkGetInstanceProcAddr);

    VkApplicationInfo appInfo = { .sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
                                  .pNext = NULL,
                                  .pApplicationName = "Test",
                                  .applicationVersion = 1,
                                  .pEngineName = NULL,
                                  .engineVersion = 0,
                                  .apiVersion = VK_MAKE_VERSION(1, 3, 236) };

    VkInstanceCreateInfo createInfo = { .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
                                        .pNext = NULL,
                                        .flags = 0,
                                        .pApplicationInfo = &appInfo,
                                        .enabledExtensionCount = 0,
                                        .ppEnabledExtensionNames = NULL,
                                        .enabledLayerCount = 0,
                                        .ppEnabledLayerNames = NULL };

    PFN_vkCreateInstance fp_vkCreateInstance = (PFN_vkCreateInstance)
        fp_vkGetInstanceProcAddr(NULL, "vkCreateInstance");
    VkInstance instance;
    VkResult res = fp_vkCreateInstance(&createInfo, NULL, &instance);

    printf("VkResult from test: %d\n", res);

    PFN_vkDestroyInstance fp_vkDestroyInstance = (PFN_vkDestroyInstance)
        fp_vkGetInstanceProcAddr(instance, "vkDestroyInstance");
    fp_vkDestroyInstance(instance, NULL);

    printf("Got to end of segfault test!\n");
}


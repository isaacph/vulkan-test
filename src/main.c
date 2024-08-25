#include <assert.h>
#include <dlfcn.h>
#include <stdio.h>
#define VK_NO_PROTOTYPES
#include <vulkan/vk_platform.h>
#include <vulkan/vulkan_core.h>

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

int main() {
    void* vulkan_handle = dlopen("libvulkan.so", RTLD_LAZY);
    assert(vulkan_handle != NULL);
    PFN_vkGetInstanceProcAddr fp_vkGetInstanceProcAddr = (PFN_vkGetInstanceProcAddr) dlsym(vulkan_handle, "vkGetInstanceProcAddr");
    assert(fp_vkGetInstanceProcAddr != NULL);
    PFN_vkEnumerateInstanceVersion fp_vkEnumerateInstanceVersion =
        (PFN_vkEnumerateInstanceVersion) fp_vkGetInstanceProcAddr(VK_NULL_HANDLE, "vkEnumerateInstanceVersion");
    assert(fp_vkEnumerateInstanceVersion != NULL);
    uint32_t version;
    VkResult res = fp_vkEnumerateInstanceVersion(&version);
    assert(res == VK_SUCCESS);
    char vk_api_version[64];
    calc_VK_API_VERSION(version, vk_api_version, 64);
    printf("Vulkan instance version %s\n", vk_api_version);
}

// #include "util/backtrace.h"
// #include "winmain.h"
// #include <stdio.h>
// 
// int main() {
// #if defined(_WIN32)
//     return winmain();
// #endif
//     printf("Init signals\n");
//     init_exceptions(false);
//     exception_msg("linux\n");
//     printf("linux\n");
// }

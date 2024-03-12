#include <stddef.h>
#include <stdio.h>
#include "util.h"
#include <vulkan/vulkan_core.h>
#include "../backtrace.h"
#define VK_API_VERSION_BUFFER_PRETTY_PRINT_LEN 64
#define RENDER_MAX_ERR_MSG_LENGTH 128

void calc_VK_API_VERSION(uint32_t version, char* out_memory, uint32_t out_len);
void print_VkExtensionProperties(uint32_t count, VkExtensionProperties* extensions);
void print_VkLayerProperties(uint32_t count, VkLayerProperties* layers);

// invariant: out_len >= VK_API_VERSION_BUFFER_PRETTY_PRINT_LEN (64)
void calc_VK_API_VERSION(uint32_t version, char* out_memory, uint32_t out_len) {
    if (out_len < VK_API_VERSION_BUFFER_PRETTY_PRINT_LEN) {
        exception_msg("Invalid output buffer length");
    }
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

void print_VkExtensionProperties(uint32_t count, VkExtensionProperties* extensions) {
    printf("-- Layer List --\n");
    for (int i = 0; i < count; ++i) {
        VkExtensionProperties* extension = &extensions[i];
        printf("%s v%u\n", extension->extensionName, extension->specVersion);
    }
    printf("-- End Layer List --\n");
}

void print_VkLayerProperties(uint32_t count, VkLayerProperties* layers) {
    char buffer[VK_API_VERSION_BUFFER_PRETTY_PRINT_LEN] = { 0 };
    printf("-- Instance Extension List --\n");
    for (int i = 0; i < count; ++i) {
        VkLayerProperties* layer = &layers[i];
        calc_VK_API_VERSION(layer->specVersion, buffer, VK_API_VERSION_BUFFER_PRETTY_PRINT_LEN);
        printf("%s specVer=%s implVer=%u\n%s\n",
            layer->layerName, buffer, layer->implementationVersion, layer->description);
    }
    printf("-- End Instance Extension List --\n");
}

// This check fails if not equal to VK_SUCCESS
// So if VK_INCOMPLETE for example is acceptable, use a different error handling method!
void check(VkResult res) {
    if (res != VK_SUCCESS) {
        char msg[RENDER_MAX_ERR_MSG_LENGTH];
        snprintf(msg, RENDER_MAX_ERR_MSG_LENGTH, "Vulkan Error: code %i", res);
        exception_msg(msg);
    }
}

void checkMalloc(void* ptr) {
    if (ptr == NULL) {
        const char* msg = "Malloc returned NULL, out of memory!";
        exception_msg(ptr);
    }
}

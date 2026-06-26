#include <stddef.h>
#include <stdio.h>
#include "util.h"
#include <vulkan/vulkan_core.h>
#include "../util/backtrace.h"
#include <assert.h>
#define VK_API_VERSION_BUFFER_PRETTY_PRINT_LEN 64
#define RENDER_MAX_ERR_MSG_LENGTH 128

void calc_VK_API_VERSION(uint32_t version, char* out_memory, uint32_t out_len);
void print_VkExtensionProperties(uint32_t count, VkExtensionProperties* extensions);
void print_VkLayerProperties(uint32_t count, VkLayerProperties* layers);

void calc_VK_API_VERSION(uint32_t version, char* out_memory, uint32_t out_len) {
    assert(out_len >= VK_API_VERSION_BUFFER_PRETTY_PRINT_LEN);
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
    printf("-- Extension List --\n");
    for (int i = 0; i < count; ++i) {
        VkExtensionProperties* extension = &extensions[i];
        printf("%s v%u\n", extension->extensionName, extension->specVersion);
    }
    printf("-- End Extension List --\n");
}

void print_VkLayerProperties(uint32_t count, VkLayerProperties* layers) {
    char buffer[VK_API_VERSION_BUFFER_PRETTY_PRINT_LEN] = { 0 };
    printf("-- Layer List --\n");
    for (int i = 0; i < count; ++i) {
        VkLayerProperties* layer = &layers[i];
        calc_VK_API_VERSION(layer->specVersion, buffer, VK_API_VERSION_BUFFER_PRETTY_PRINT_LEN);
        printf("%s specVer=%s implVer=%u -- %s\n",
            layer->layerName, buffer, layer->implementationVersion, layer->description);
    }
    printf("-- End Layer List --\n");
}

// This check fails if not equal to VK_SUCCESS
// So if VK_INCOMPLETE for example is acceptable, use a different error handling method!
void check(VkResult res) {
    if (res != VK_SUCCESS) {
        char msg[RENDER_MAX_ERR_MSG_LENGTH];
        snprintf(msg, RENDER_MAX_ERR_MSG_LENGTH, "Vulkan Error: code %i", res);
        if (res == VK_ERROR_DEVICE_LOST) {
            printf("Error detected as: VK_ERROR_DEVICE_LOST\n");
        } else if (res == VK_ERROR_OUT_OF_HOST_MEMORY) {
            printf("Error detected as: VK_ERROR_OUT_OF_HOST_MEMORY\n");
        } else if (res == VK_ERROR_OUT_OF_DEVICE_MEMORY) {
            printf("Error detected as: VK_ERROR_OUT_OF_DEVICE_MEMORY\n");
        } else if (res == VK_ERROR_INITIALIZATION_FAILED) {
            printf("Error detected as: VK_ERROR_INITIALIZATION_FAILED\n");
        } else if (res == VK_ERROR_LAYER_NOT_PRESENT) {
            printf("Error detected as: VK_ERROR_LAYER_NOT_PRESENT\n");
        } else if (res == VK_ERROR_INVALID_DEVICE_ADDRESS_EXT) {
            printf("Error detected as: VK_ERROR_INVALID_DEVICE_ADDRESS_EXT\n");
        }
        exception_msg(msg);
    }
}

bool validate_unicode(const char* unicode) {
    return false;
}

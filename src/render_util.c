#include "render_util.h"
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <vulkan/vulkan_core.h>
#include "util.h"
#include "backtrace.h"

const char* interpret_VK_API_VERSION(uint32_t version) {
    const long long MAX_VERSION_STR_LENGTH = 64;
    uint32_t variant = VK_API_VERSION_VARIANT(version);
    uint32_t major = VK_API_VERSION_MAJOR(version);
    uint32_t minor = VK_API_VERSION_MINOR(version);
    uint32_t patch = VK_API_VERSION_PATCH(version);
    char* string = malloc(MAX_VERSION_STR_LENGTH);
    if (string == NULL) exception();
    if (variant != 0) {
        snprintf(string, MAX_VERSION_STR_LENGTH, "Variant %u: %u.%u.%u", variant, major, minor, patch);
    } else {
        snprintf(string, MAX_VERSION_STR_LENGTH, "%u.%u.%u", major, minor, patch);
    }
    return string;
}

const char* interpret_VkExtensionProperties(uint32_t count, VkExtensionProperties* extensions) {
    const size_t MAX_EXTENSION_STR_LENGTH = 16 + VK_MAX_EXTENSION_NAME_SIZE;
    char* extensionList = malloc(MAX_EXTENSION_STR_LENGTH * count);
    if (extensionList == NULL) exception();
    int strPos = 0;
    for (int i = 0; i < count; ++i) {
        VkExtensionProperties* extension = &extensions[i];
        strPos += MIN(MAX_EXTENSION_STR_LENGTH,
                snprintf(extensionList + strPos, MAX_EXTENSION_STR_LENGTH,
                    "%s v%u\n", extension->extensionName, extension->specVersion)
                );
    }
    return extensionList;
}

const char* interpret_VkLayerProperties(uint32_t count, VkLayerProperties* layers) {
    const size_t MAX_LAYER_STR_LENGTH = 32 + VK_MAX_EXTENSION_NAME_SIZE + VK_MAX_DESCRIPTION_SIZE;
    char* layerList = malloc(MAX_LAYER_STR_LENGTH * count);
    if (layerList == NULL) exception();
    int strPos = 0;
    for (int i = 0; i < count; ++i) {
        VkLayerProperties* layer = &layers[i];
        const char* spec_ver = interpret_VK_API_VERSION(layer->specVersion);
        strPos += MIN(MAX_LAYER_STR_LENGTH,
                snprintf(layerList + strPos, MAX_LAYER_STR_LENGTH,
                    "%s specVer=%s implVer=%u\n%s\n",
                    layer->layerName, spec_ver, layer->implementationVersion, layer->description)
                );
        free((void*) spec_ver);
    }
    return layerList;
}

// void interpret_VK_API_VERSION(uint32_t version, char* out_memory, uint64_t memory_length) {
//     uint32_t variant = VK_API_VERSION_VARIANT(version);
//     uint32_t major = VK_API_VERSION_MAJOR(version);
//     uint32_t minor = VK_API_VERSION_MINOR(version);
//     uint32_t patch = VK_API_VERSION_PATCH(version);
//     if (variant != 0) {
//         snprintf(out_memory, memory_length, "Variant %u: %u.%u.%u", variant, major, minor, patch);
//     } else {
//         snprintf(out_memory, memory_length, "%u.%u.%u", major, minor, patch);
//     }
// }
// 
// void interpret_VkExtensionProperties(uint32_t count, VkExtensionProperties* extensions) {
//     const size_t MAX_EXTENSION_STR_LENGTH = 16 + VK_MAX_EXTENSION_NAME_SIZE;
//     char* extensionList = malloc(MAX_EXTENSION_STR_LENGTH * count);
//     if (extensionList == NULL) exception();
//     int strPos = 0;
//     for (int i = 0; i < count; ++i) {
//         VkExtensionProperties* extension = &extensions[i];
//         strPos += MIN(MAX_EXTENSION_STR_LENGTH,
//                 snprintf(extensionList + strPos, MAX_EXTENSION_STR_LENGTH,
//                     "%s v%u\n", extension->extensionName, extension->specVersion)
//                 );
//     }
// }
// 
// void interpret_VkLayerProperties(uint32_t count, VkLayerProperties* layers, char* out_memory, uint64_t memory_length) {
//     const size_t MAX_LAYER_STR_LENGTH = 32 + VK_MAX_EXTENSION_NAME_SIZE + VK_MAX_DESCRIPTION_SIZE;
//     char* layerList = malloc(MAX_LAYER_STR_LENGTH * count);
//     if (layerList == NULL) exception();
//     int strPos = 0;
//     for (int i = 0; i < count; ++i) {
//         VkLayerProperties* layer = &layers[i];
//         const char* spec_ver = interpret_VK_API_VERSION(layer->specVersion);
//         strPos += MIN(MAX_LAYER_STR_LENGTH,
//                 snprintf(layerList + strPos, MAX_LAYER_STR_LENGTH,
//                     "%s specVer=%s implVer=%u\n%s\n",
//                     layer->layerName, spec_ver, layer->implementationVersion, layer->description)
//                 );
//         free((void*) spec_ver);
//     }
//     return layerList;
// }

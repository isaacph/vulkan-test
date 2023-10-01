#include "render.h"
#include "render_util.h"
#include "util.h"
#include "backtrace.h"
#include <assert.h>
#include <libloaderapi.h>
#include <locale.h>
#include <minwindef.h>
#include <stdio.h>
#include <stdlib.h>
#include <vulkan/vulkan_core.h>
#include <windows.h>
#include <stdbool.h>

const char REQUIRED_EXTENSIONS[][VK_MAX_EXTENSION_NAME_SIZE]  = {
    "VK_KHR_surface", // rendering to monitors
    "VK_KHR_win32_surface", // Windows OS rendering
    "VK_KHR_display", // fullscreen
};

struct RenderContext init_render() {
    // https://github.com/charles-lunarg/vk-bootstrap/blob/main/src/VkBootstrap.cpp#L61
    // https://learn.microsoft.com/en-us/windows/win32/api/libloaderapi/nf-libloaderapi-loadlibrarya
    HMODULE library;
    library = LoadLibraryA(TEXT("vulkan-1.dll"));
    if (library == NULL) {
        exception_msg("vulkan-1.dll was not found");
    }

    PFN_vkGetInstanceProcAddr fp_vkGetInstanceProcAddr;
    fp_vkGetInstanceProcAddr = (PFN_vkGetInstanceProcAddr) GetProcAddress(library, "vkGetInstanceProcAddr");
    if (fp_vkGetInstanceProcAddr == NULL) exception();
    printf("Found loader func: %p\n", fp_vkGetInstanceProcAddr);

    // check VK_API_VERSION
    PFN_vkEnumerateInstanceVersion fp_vkEnumerateInstanceVersion =
        (PFN_vkEnumerateInstanceVersion) fp_vkGetInstanceProcAddr(VK_NULL_HANDLE,
                "vkEnumerateInstanceVersion");
    if (fp_vkEnumerateInstanceVersion == NULL) exception();
    uint32_t instanceVersion;
    if (fp_vkEnumerateInstanceVersion(&instanceVersion) != VK_SUCCESS) exception();
    const char* version_str = interpret_VK_API_VERSION(instanceVersion);
    printf("Vulkan instance version %s\n", version_str);
    free((void*) version_str);

    // get VkExtensionProperties
    PFN_vkEnumerateInstanceExtensionProperties fp_vkEnumerateInstanceExtensionProperties =
        (PFN_vkEnumerateInstanceExtensionProperties) fp_vkGetInstanceProcAddr(VK_NULL_HANDLE,
                "vkEnumerateInstanceExtensionProperties");
    uint32_t extensionsCount;
    VkExtensionProperties* extensions;
    if (fp_vkEnumerateInstanceExtensionProperties == NULL) exception();
    if (fp_vkEnumerateInstanceExtensionProperties(NULL, &extensionsCount, NULL) != VK_SUCCESS) exception();
    extensions = malloc(sizeof(VkExtensionProperties) * extensionsCount);
    if (extensions == NULL) exception();
    if (fp_vkEnumerateInstanceExtensionProperties(NULL, &extensionsCount, extensions) != VK_SUCCESS) exception();
    const char* ext_string = interpret_VkExtensionProperties(extensionsCount, extensions);
    printf("-- Extension List --\n%s-- End Extension List --\n", ext_string);
    free((void*) ext_string);

    // get VkLayerProperties
    PFN_vkEnumerateInstanceLayerProperties fp_vkEnumerateInstanceLayerProperties =
        (PFN_vkEnumerateInstanceLayerProperties) fp_vkGetInstanceProcAddr(VK_NULL_HANDLE,
                "vkEnumerateInstanceLayerProperties");
    uint32_t layerCount;
    VkLayerProperties* layers;
    if (fp_vkEnumerateInstanceLayerProperties == NULL) exception();
    if (fp_vkEnumerateInstanceLayerProperties(&layerCount, NULL) != VK_SUCCESS) exception();
    layers = malloc(sizeof(VkLayerProperties) * layerCount);
    if (layers == NULL) exception();
    if (fp_vkEnumerateInstanceLayerProperties(&layerCount, layers) != VK_SUCCESS) exception();
    const char* layers_string = interpret_VkLayerProperties(layerCount, layers);
    printf("-- Layer List --\n%s-- End Layer List --\n", layers_string);
    free((void*) layers_string);

    // check we have the required extensions
    for (int i = 0; i < sizeof(REQUIRED_EXTENSIONS) / VK_MAX_EXTENSION_NAME_SIZE; ++i) {
        const char* required = REQUIRED_EXTENSIONS[i];
        bool found = false;
        for (int j = 0; j < extensionsCount && !found; ++j) {
            if (strncmp(required, extensions[j].extensionName, VK_MAX_EXTENSION_NAME_SIZE) == 0) {
                found = true;
            }
        }
        if (!found) {
            char msg[VK_MAX_EXTENSION_NAME_SIZE + 64];
            sprintf(msg, "ERROR: missing required Vulkan extension: %s\n", required);
            exception_msg(msg);
        }
    }
    printf("All required extensions found.\n");

    struct RenderContext renderContext = {
        .instanceVersion = instanceVersion,
        .extensionsCount = extensionsCount,
        .extensions = extensions,
        .layersCount = layerCount,
        .layers = layers,
    };

    return renderContext;
}

void free_render(struct RenderContext *renderContext) {
    free(renderContext->extensions);
    free(renderContext->layers);
}


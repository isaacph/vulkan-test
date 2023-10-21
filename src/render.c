#include <volk.h>
#include <windows.h>
#include "app.h"
#include "render.h"
#include "render_util.h"
#include "backtrace.h"
#include <libloaderapi.h>
#include <locale.h>
#include <minwindef.h>
#include <stdio.h>
#include <stdlib.h>
#include <vulkan/vulkan_core.h>
#include <stdbool.h>
#define RENDER_MAX_ERR_MSG_LENGTH 128

const char ENABLE_EXTENSIONS[][VK_MAX_EXTENSION_NAME_SIZE] = {
    "VK_KHR_surface", // rendering to monitors
};
const size_t ENABLE_EXTENSIONS_COUNT = 0;

// uses VK_MAX_EXTENSION_NAME_SIZE according to:
// https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VkLayerProperties.html
const char ENABLE_LAYERS[][VK_MAX_EXTENSION_NAME_SIZE] = {
    "VK_LAYER_KHRONOS_validation",
};
const size_t ENABLE_LAYERS_COUNT = 0;

static void check(VkResult res) {
    if (res != VK_SUCCESS) {
        char msg[RENDER_MAX_ERR_MSG_LENGTH];
        snprintf(msg, RENDER_MAX_ERR_MSG_LENGTH, "Vulkan Error: code %i", res);
        exception_msg(msg);
    }
}

struct RenderContext rc_init() {
#if defined(_WIN32)
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
#endif

    // check VK_API_VERSION
    PFN_vkEnumerateInstanceVersion fp_vkEnumerateInstanceVersion =
        (PFN_vkEnumerateInstanceVersion) fp_vkGetInstanceProcAddr(VK_NULL_HANDLE,
                "vkEnumerateInstanceVersion");
    if (fp_vkEnumerateInstanceVersion == NULL) exception();
    uint32_t instanceVersion;
    check(fp_vkEnumerateInstanceVersion(&instanceVersion));
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
    check(fp_vkEnumerateInstanceExtensionProperties(NULL, &extensionsCount, NULL));
    extensions = malloc(sizeof(VkExtensionProperties) * extensionsCount);
    if (extensions == NULL) exception();
    check(fp_vkEnumerateInstanceExtensionProperties(NULL, &extensionsCount, extensions));
    const char* ext_string = interpret_VkExtensionProperties(extensionsCount, extensions);
    printf("-- Instance Extension List --\n%s-- Instance End Extension List --\n", ext_string);
    free((void*) ext_string);

    // get VkLayerProperties
    PFN_vkEnumerateInstanceLayerProperties fp_vkEnumerateInstanceLayerProperties =
        (PFN_vkEnumerateInstanceLayerProperties) fp_vkGetInstanceProcAddr(VK_NULL_HANDLE,
                "vkEnumerateInstanceLayerProperties");
    uint32_t layersCount;
    VkLayerProperties* layers;
    if (fp_vkEnumerateInstanceLayerProperties == NULL) exception();
    check(fp_vkEnumerateInstanceLayerProperties(&layersCount, NULL));
    layers = malloc(sizeof(VkLayerProperties) * layersCount);
    if (layers == NULL) exception();
    check(fp_vkEnumerateInstanceLayerProperties(&layersCount, layers));
    const char* layers_string = interpret_VkLayerProperties(layersCount, layers);
    printf("-- Layer List --\n%s-- End Layer List --\n", layers_string);
    free((void*) layers_string);

    // check we have the required extensions
    for (int i = 0; i < ENABLE_EXTENSIONS_COUNT; ++i) {
        const char* required = ENABLE_EXTENSIONS[i];
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

    // check we have the required layers
    for (int i = 0; i < ENABLE_LAYERS_COUNT; ++i) {
        const char* required = ENABLE_LAYERS[i];
        bool found = false;
        for (int j = 0; j < layersCount && !found; ++j) {
            if (strncmp(required, layers[j].layerName, VK_MAX_EXTENSION_NAME_SIZE) == 0) {
                found = true;
            }
        }
        if (!found) {
            char msg[VK_MAX_EXTENSION_NAME_SIZE + 64];
            sprintf(msg, "ERROR: missing required Vulkan layer: %s\n", required);
            exception_msg(msg);
        }
    }
    printf("All required extensions and layers found.\n");

    printf("Initializing volk\n");
    // initializes instance functions
    volkInitializeCustom(fp_vkGetInstanceProcAddr);

    printf("Initializing vkInstance\n");
    VkApplicationInfo app_info = {
        .sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
        .pNext = NULL,
        .pApplicationName = APP_NAME,
        .applicationVersion = APP_VERSION,
        .pEngineName = APP_ENGINE_NAME,
        .engineVersion = APP_ENGINE_VERSION,
        .apiVersion = VK_MAKE_API_VERSION(0, 1, 3, 236)
    };

    const char* const la[] = {
        "VK_LAYER_KHRONOS_validation",
    };

    VkInstanceCreateInfo instanceCreateInfo = {
        .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
        .flags = (VkInstanceCreateFlags) 0,
        .pApplicationInfo = &app_info,
        .enabledLayerCount = 1,
        .ppEnabledLayerNames = la,
        .enabledExtensionCount = 0,
        .ppEnabledExtensionNames = NULL,
        // .enabledLayerCount = ENABLE_LAYERS_COUNT,
        // .ppEnabledLayerNames = (const char* const*) ENABLE_LAYERS,
        // .enabledExtensionCount = ENABLE_EXTENSIONS_COUNT,
        // .ppEnabledExtensionNames = (const char* const*) ENABLE_EXTENSIONS,
    };

    VkAllocationCallbacks* allocationCallbacks = VK_NULL_HANDLE;
    
    VkInstance instance;
    check(vkCreateInstance(&instanceCreateInfo, allocationCallbacks, &instance));
    volkLoadInstance(instance);

    struct RenderContext renderContext = {
        .instance = instance,
        .allocationCallbacks = allocationCallbacks,
        .fp_vkGetInstanceProcAddr = fp_vkGetInstanceProcAddr,
    };

    printf("Done initializing\n");
    return renderContext;
}

void rc_cleanup(struct RenderContext *renderContext) {
    printf("Cleaning up Vulkan\n");
    vkDestroyInstance(renderContext->instance, renderContext->allocationCallbacks);
}


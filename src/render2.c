#include "app.h"
#include "render2.h"
#include "render_util.h"
#include <stdbool.h>
#define RENDER_MAX_ERR_MSG_LENGTH 128

// This check fails if not equal to VK_SUCCESS
// So if VK_INCOMPLETE for example is acceptable, use a different error handling method!
static void check(VkResult res) {
    if (res != VK_SUCCESS) {
        char msg[RENDER_MAX_ERR_MSG_LENGTH];
        snprintf(msg, RENDER_MAX_ERR_MSG_LENGTH, "Vulkan Error: code %i", res);
        exception_msg(msg);
    }
}

// I should implement my own memory management
// static void checkMalloc(void* ptr) {
//     if (ptr == NULL) {
//         const char* msg = "Malloc returned NULL, out of memory!";
//         exception_msg(ptr);
//     }
// }

const char* const ENABLE_EXTENSIONS[] = {
    "VK_KHR_surface", // rendering to monitors
    "VK_KHR_win32_surface",
    "VK_KHR_get_surface_capabilities2", // it doesn't seem to care whether we enable this but oh well
    "VK_EXT_swapchain_colorspace",
    "VK_EXT_debug_utils",
};
const size_t ENABLE_EXTENSIONS_COUNT = 4;

// uses VK_MAX_EXTENSION_NAME_SIZE according to:
// https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VkLayerProperties.html
const char* const ENABLE_LAYERS[] = {
    "VK_LAYER_KHRONOS_validation",
};
const size_t ENABLE_LAYERS_COUNT = 0;

const char* const ENABLE_DEVICE_EXTENSIONS[] = {
    "VK_KHR_swapchain",
};
const size_t ENABLE_DEVICE_EXTENSIONS_COUNT = 1;



RenderContext2 rc_init_instance(PFN_vkGetInstanceProcAddr fp_vkGetInstanceProcAddr) {

    // variables we are keeping around
    uint32_t extensionsCount;
    VkExtensionProperties* extensions;
    uint32_t layersCount = 0;
    VkLayerProperties* layers = NULL; // must be freed
    VkAllocationCallbacks* allocationCallbacks = VK_NULL_HANDLE;
    VkInstance instance = NULL;

    // initialize loader functions
    init_loader_functions(fp_vkGetInstanceProcAddr);

    // do preinit checks
    {
        // check VK_API_VERSION
        uint32_t instanceVersion;
        check(vkEnumerateInstanceVersion(&instanceVersion));
        interpret_VK_API_VERSION(instanceVersion);
        printf("Vulkan instance version %s\n", version_str);
        free((void*) version_str);

        // get VkExtensionProperties
        extensionsCount = 0;
        extensions = NULL;
        check(vkEnumerateInstanceExtensionProperties(NULL, &extensionsCount, NULL));
        extensions = malloc(sizeof(VkExtensionProperties) * extensionsCount);
        if (extensions == NULL) exception();
        check(vkEnumerateInstanceExtensionProperties(NULL, &extensionsCount, extensions));
        const char* ext_string = interpret_VkExtensionProperties(extensionsCount, extensions);
        printf("-- Instance Extension List --\n%s-- End Instance Extension List --\n", ext_string);
        free((void*) ext_string);
        // free(extensions);

        // get VkLayerProperties
        layersCount = 0;
        layers = NULL;
        check(vkEnumerateInstanceLayerProperties(&layersCount, NULL));
        layers = malloc(sizeof(VkLayerProperties) * layersCount);
        if (layers == NULL) exception();
        check(vkEnumerateInstanceLayerProperties(&layersCount, layers));
        const char* layers_string = interpret_VkLayerProperties(layersCount, layers);
        printf("-- Layer List --\n%s-- End Layer List --\n", layers_string);
        free((void*) layers_string);
        // free(layers);

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
                sprintf(msg, "ERROR: missing required Vulkan instance extension: %s\n", required);
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
        printf("All required instance extensions and layers found.\n");
    }

    // initialize vkInstance
    {
        // make and load instance
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
        VkInstanceCreateInfo instanceCreateInfo = {
            .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
            .flags = (VkInstanceCreateFlags) 0,
            .pApplicationInfo = &app_info,
            .enabledLayerCount = ENABLE_LAYERS_COUNT,
            .ppEnabledLayerNames = ENABLE_LAYERS,
            .enabledExtensionCount = ENABLE_EXTENSIONS_COUNT,
            .ppEnabledExtensionNames = ENABLE_EXTENSIONS,
        };
        instance = NULL;
        check(vkCreateInstance(&instanceCreateInfo, allocationCallbacks, &instance));
        printf("Created instance\n");

        // init instance functions
        init_instance_functions(instance);
    }

    RenderContext2 renderContext = {
        // make render context for later use
        .instance = instance,
        .physicalDevice = VK_NULL_HANDLE,
        .device = VK_NULL_HANDLE,
        .surface = VK_NULL_HANDLE, // defined by platform-specific code
        .allocationCallbacks = allocationCallbacks,
        .swapchain = VK_NULL_HANDLE,
        .surfaceFormat = {0},
        .swapchainExtent = {0},
    };

    free(extensions);
    free(layers);

    return renderContext;
}

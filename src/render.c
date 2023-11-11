#include "app.h"
#include "util.h"
#include "render.h"
#include "render_util.h"
#include <stdbool.h>
#define RENDER_MAX_ERR_MSG_LENGTH 128

const char* const ENABLE_EXTENSIONS[] = {
    "VK_KHR_surface", // rendering to monitors
    "VK_KHR_win32_surface",
};
const size_t ENABLE_EXTENSIONS_COUNT = 2;

// uses VK_MAX_EXTENSION_NAME_SIZE according to:
// https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VkLayerProperties.html
const char* const ENABLE_LAYERS[] = {
    "VK_LAYER_KHRONOS_validation",
};
const size_t ENABLE_LAYERS_COUNT = 1;

const char* const ENABLE_DEVICE_EXTENSIONS[] = {
    "Not an extension",
};
const size_t ENABLE_DEVICE_EXTENSIONS_COUNT = 0;

const VkStructureType ENABLE_FEATURES_STRUCTS[] = {
    0,
};
const size_t ENABLE_FEATURES_STRUCTS_COUNT = 0;

void validateEnabledFeatures(VkPhysicalDeviceFeatures2* features) {
    // check features
    // then check features->pNext
    // etc
}

const VkFormat USE_FORMAT = VK_FORMAT_R8_UNORM;
void validateFormatProperties(VkFormatProperties2* properties) {
}

const int REQUIRE_PHYSICAL_DEVICE_QUEUE_FAMILY_FLAG = VK_QUEUE_GRAPHICS_BIT;

// todo: add ability to enable features, fill out QueueCreateInfo
//https://registry.khronos.org/vulkan/specs/1.3-extensions/html/chap5.html#devsandqueues-device-creation
// https://registry.khronos.org/vulkan/specs/1.3-extensions/html/chap47.html

// This check fails if not equal to VK_SUCCESS
// So if VK_INCOMPLETE for example is acceptable, use a different error handling method!
static void check(VkResult res) {
    if (res != VK_SUCCESS) {
        char msg[RENDER_MAX_ERR_MSG_LENGTH];
        snprintf(msg, RENDER_MAX_ERR_MSG_LENGTH, "Vulkan Error: code %i", res);
        exception_msg(msg);
    }
}

static void checkMalloc(void* ptr) {
    if (ptr == NULL) {
        const char* msg = "Malloc returned NULL, out of memory!";
        exception_msg(ptr);
    }
}

struct RenderContext rc_init(PFN_vkGetInstanceProcAddr fp_vkGetInstanceProcAddr) {

    // variables we are keeping around
    uint32_t extensionsCount;
    VkExtensionProperties* extensions;
    uint32_t layersCount = 0;
    VkLayerProperties* layers = NULL; // must be freed
    VkAllocationCallbacks* allocationCallbacks = VK_NULL_HANDLE;
    VkInstance instance = NULL;
    VkPhysicalDevice chosenPhysicalDevice = NULL;
    VkDevice device = NULL;
    VkQueue queue = NULL;

    // initialize loader functions
    init_loader_functions(fp_vkGetInstanceProcAddr);

    // do preinit checks
    {
        // check VK_API_VERSION
        uint32_t instanceVersion;
        check(vkEnumerateInstanceVersion(&instanceVersion));
        const char* version_str = interpret_VK_API_VERSION(instanceVersion);
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

    // initialize vkInstance, vkDevice
    {
        const char* en_layers[] = {
            "VK_LAYER_KHRONOS_validation",
        };

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
            // .enabledLayerCount = 1,
            // .ppEnabledLayerNames = en_layers,
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

        // choose our physical device
        chosenPhysicalDevice = NULL;
        uint32_t pPhysicalDeviceCount = 0;
        VkPhysicalDevice* pPhysicalDevices = NULL;
        check(vkEnumeratePhysicalDevices(instance, &pPhysicalDeviceCount, NULL));
        if (pPhysicalDeviceCount == 0) {
            exception_msg("install a fucking graphics card");
        }
        pPhysicalDevices = (VkPhysicalDevice*) malloc(pPhysicalDeviceCount *
                sizeof(VkPhysicalDevice));
        checkMalloc(pPhysicalDevices);
        check(vkEnumeratePhysicalDevices(instance, &pPhysicalDeviceCount, pPhysicalDevices));
        for (uint32_t i = 0; i < pPhysicalDeviceCount; ++i) {
            VkPhysicalDevice device = pPhysicalDevices[i];
            VkPhysicalDeviceProperties properties = {0};
            vkGetPhysicalDeviceProperties(device, &properties);
            printf("Found graphics card: %s\n", properties.deviceName);
            if (i == 0) {
                chosenPhysicalDevice = device;
                printf("Choosing graphics card: %s\n", properties.deviceName);
                printf("Reason: Just choosing the first one for now, choice not implemented\n");
                break;
            }
        }
        free(pPhysicalDevices);

        // check device extensions
        // using chosenPhysicalDevice
        bool deviceExtensionFound
            [sizeof(ENABLE_DEVICE_EXTENSIONS) / sizeof(ENABLE_DEVICE_EXTENSIONS[0])]
            = {false};
        for (int i = -1; i < (int) layersCount; ++i) {
            const char *layerName = NULL;
            if (i != -1) {
                layerName = layers[i].layerName;
            }
            uint32_t propertyCount = 0;
            VkExtensionProperties* properties = {0};
            check(vkEnumerateDeviceExtensionProperties(chosenPhysicalDevice, layerName,
                        &propertyCount, NULL));
            properties = malloc(propertyCount * sizeof(VkExtensionProperties));
            checkMalloc(properties);
            check(vkEnumerateDeviceExtensionProperties(chosenPhysicalDevice, layerName,
                        &propertyCount, properties));

            // check required extensions are available
            for (int propertyIndex = 0; propertyIndex < (int) propertyCount;
                    ++propertyIndex) {
                VkExtensionProperties* property = &properties[propertyIndex];
                for (int requiredIndex = 0; requiredIndex < (int) ENABLE_DEVICE_EXTENSIONS_COUNT;
                    ++requiredIndex) {
                    if (!strncmp(property->extensionName,
                                ENABLE_DEVICE_EXTENSIONS[requiredIndex],
                                VK_MAX_EXTENSION_NAME_SIZE)) {
                        deviceExtensionFound[requiredIndex] = true;
                    }
                }
            }

            // log layer extensions
            if (layerName == NULL) layerName = "DEFAULT";
            printf("-- Layer %s extensions --\n", layerName);
            for (int i = 0; i < MIN(5, (int) propertyCount); ++i) {
                VkExtensionProperties* property = &properties[i];
                printf("%s v%u\n", property->extensionName, property->specVersion);
            }
            if (propertyCount > 5) {
                printf("%i more...\n", propertyCount - 5);
            }
            printf("-- End Device Extension List --\n");

            free(properties);
        }
        for (int requiredIndex = 0; requiredIndex < (int) ENABLE_DEVICE_EXTENSIONS_COUNT;
                ++requiredIndex) {
            if (!deviceExtensionFound[requiredIndex]) {
                const char *extensionName = ENABLE_DEVICE_EXTENSIONS[requiredIndex];
                char msg[VK_MAX_EXTENSION_NAME_SIZE + 64];
                snprintf(msg, VK_MAX_EXTENSION_NAME_SIZE + 64,
                        "missing required Vulkan device extension: %s\n",
                        extensionName);
                exception_msg(msg);
            }
        }
        printf("Contains all required Vulkan device extensions\n"); 

        // check for available features and make feature request
        VkPhysicalDeviceFeatures2 features = {0};
        {
            VkPhysicalDevice physDevice = chosenPhysicalDevice;
            features = (VkPhysicalDeviceFeatures2) {
                .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2,
                .pNext = NULL,
            };
            vkGetPhysicalDeviceFeatures2(physDevice, &features);
            validateEnabledFeatures(&features);
        }

        // check for format properties
        {
            VkFormatProperties2 formatProperties = {
                .sType = VK_STRUCTURE_TYPE_FORMAT_PROPERTIES_2,
                .pNext = NULL,
            };
            vkGetPhysicalDeviceFormatProperties2(chosenPhysicalDevice, USE_FORMAT, &formatProperties);
            validateFormatProperties(&formatProperties);
        }

        // find queue family index that contains required queue flags
        uint32_t queueFamilyIndex = 0;
        bool foundQueueFamily = false;
        {
            uint32_t count = 0;
            VkQueueFamilyProperties* properties = NULL;
            vkGetPhysicalDeviceQueueFamilyProperties(chosenPhysicalDevice, &count, NULL);
            properties = malloc(count * sizeof(VkQueueFamilyProperties));
            checkMalloc(properties);
            vkGetPhysicalDeviceQueueFamilyProperties(chosenPhysicalDevice, &count, properties);

            for (uint32_t i = 0; i < count; ++i) {
                printf("Queue family queueFlags: %i\n", properties[i].queueFlags);
                if ((properties[i].queueFlags & REQUIRE_PHYSICAL_DEVICE_QUEUE_FAMILY_FLAG) != 0) {
                    queueFamilyIndex = i;
                    foundQueueFamily = true;
                    break;
                }
            }

            free(properties);

            if (!foundQueueFamily) {
                exception_msg("Could not find PhysicalDeviceQueueFamily that supports the required flags");
            }
        }

        // finally create the device
        {
            float queuePriority = 1.0f;
            VkDeviceQueueCreateInfo queueCreateInfo = {
                .sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
                .pNext = NULL,
                .flags = 0,
                .queueFamilyIndex = queueFamilyIndex,
                .queueCount = 1,
                .pQueuePriorities = &queuePriority,
            };
            VkDeviceCreateInfo createInfo = {
                .sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
                .pNext = &features,
                .flags = 0,
                .queueCreateInfoCount = 1,
                .pQueueCreateInfos = &queueCreateInfo,
                .enabledLayerCount = 0,
                .ppEnabledLayerNames = NULL,
                .enabledExtensionCount = (uint32_t) ENABLE_DEVICE_EXTENSIONS_COUNT,
                .ppEnabledExtensionNames = ENABLE_DEVICE_EXTENSIONS,
                .pEnabledFeatures = NULL,
            };
            check(vkCreateDevice(
                chosenPhysicalDevice, &createInfo, allocationCallbacks, &device));
            printf("Device initialized\n");
        }

        // init all of the vk* functions that are per-device
        init_device_functions(device);
        printf("Device functions initialized\n");

        // get queue
        vkGetDeviceQueue(device, queueFamilyIndex, 0, &queue);
    }

    // make render context for later use
    struct RenderContext renderContext = {
        .instance = instance,
        .device = device,
        .queue = queue,
        .allocationCallbacks = allocationCallbacks,
        .surface = VK_NULL_HANDLE, // defined by platform-specific code
        .swapchain = VK_NULL_HANDLE,
    };

    free(extensions);
    free(layers);

    return renderContext;
}

void rc_swapchain_init(struct RenderContext* renderContext) {
    VkSwapchainKHR swapchain = VK_NULL_HANDLE;
    VkFormat swapchainFormat = 0;
}

void rc_cleanup(struct RenderContext *renderContext) {
    printf("Cleaning up Vulkan\n");
    vkDestroySurfaceKHR(renderContext->instance, renderContext->surface, renderContext->allocationCallbacks);
    vkDestroyDevice(renderContext->device, renderContext->allocationCallbacks);
    vkDestroyInstance(renderContext->instance, renderContext->allocationCallbacks);
}

// windows-specific init
#if defined(_WIN32)
#include <windows.h>
struct RenderContext rc_init_win32(HINSTANCE hInstance, HWND hwnd) {
    // load the Vulkan loader
    PFN_vkGetInstanceProcAddr fp_vkGetInstanceProcAddr;
    {
        // https://github.com/charles-lunarg/vk-bootstrap/blob/main/src/VkBootstrap.cpp#L61
        // https://learn.microsoft.com/en-us/windows/win32/api/libloaderapi/nf-libloaderapi-loadlibrarya
        HMODULE library;
        library = LoadLibraryW(L"vulkan-1.dll");
        if (library == NULL) {
            DWORD error = GetLastError();
            printf("Windows error: %lu\n", error);
            exception_msg("vulkan-1.dll was not found");
        }

        fp_vkGetInstanceProcAddr = NULL;
        fp_vkGetInstanceProcAddr = (PFN_vkGetInstanceProcAddr) GetProcAddress(library, "vkGetInstanceProcAddr");
        if (fp_vkGetInstanceProcAddr == NULL) exception();
        printf("Found loader func: %p\n", fp_vkGetInstanceProcAddr);

        // sanity check
        if (fp_vkGetInstanceProcAddr(VK_NULL_HANDLE, "vkEnumerateInstanceVersion") == NULL) {
            exception_msg("Invalid vulkan-1.dll");
        }
    }

    // set up the swapchain

    // call regular platform-agnostic init
    struct RenderContext context = rc_init(fp_vkGetInstanceProcAddr);

    // make win32 surface
    {
        VkWin32SurfaceCreateInfoKHR createInfo = {
            .sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR,
            .pNext = NULL,
            .flags = 0,
            .hinstance = hInstance,
            .hwnd = hwnd,
        };
        check(vkCreateWin32SurfaceKHR(context.instance, &createInfo, context.allocationCallbacks, &context.surface));
    }
    printf("Created win32 surface\n");

    return context;
}
#endif


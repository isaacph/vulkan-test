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
        char vk_api_version[64];
        calc_VK_API_VERSION(instanceVersion, vk_api_version, 64);
        printf("Vulkan instance version %s\n", vk_api_version);

        // get VkExtensionProperties
        extensionsCount = 0;
        extensions = NULL;
        check(vkEnumerateInstanceExtensionProperties(NULL, &extensionsCount, NULL));
        extensions = malloc(sizeof(VkExtensionProperties) * extensionsCount);
        if (extensions == NULL) exception();
        check(vkEnumerateInstanceExtensionProperties(NULL, &extensionsCount, extensions));
        print_VkExtensionProperties(extensionsCount, extensions);
        // free(extensions);

        // get VkLayerProperties
        layersCount = 0;
        layers = NULL;
        check(vkEnumerateInstanceLayerProperties(&layersCount, NULL));
        layers = malloc(sizeof(VkLayerProperties) * layersCount);
        if (layers == NULL) exception();
        check(vkEnumerateInstanceLayerProperties(&layersCount, layers));
        print_VkLayerProperties(layersCount, layers);
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

static void rc_init_device(RenderContext2* context) {
    if (context->surface == VK_NULL_HANDLE) {
        exception_msg("Surface must be initialized before device can be initialized (reason: used to decide which physical device supports the surface)\n");
    }
    if (context->physicalDevice != VK_NULL_HANDLE) {
        exception_msg("Physical device already chosen\n");
    }
    if (context->device != VK_NULL_HANDLE) {
        exception_msg("Device already chosen\n");
    }

    VkInstance instance = context->instance;
    VkAllocationCallbacks* allocationCallbacks = context->allocationCallbacks;
    VkSurfaceKHR surface = context->surface;
    VkPhysicalDevice chosenPhysicalDevice = NULL;
    VkDevice device = NULL;
    VkQueue queue = NULL;
    VkCommandPool commandPool = VK_NULL_HANDLE;
    VkCommandBuffer commandBuffer = VK_NULL_HANDLE;

    // we're going to need to enumerate layers again
    uint32_t layersCount = 0;
    VkLayerProperties* layers = NULL;
    check(vkEnumerateInstanceLayerProperties(&layersCount, NULL));
    layers = malloc(sizeof(VkLayerProperties) * layersCount);
    if (layers == NULL) exception();
    check(vkEnumerateInstanceLayerProperties(&layersCount, layers));

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
    uint32_t queueFamilyIndex = 0;
    for (uint32_t i = 0; i < pPhysicalDeviceCount; ++i) {
        VkPhysicalDevice device = pPhysicalDevices[i];
        VkPhysicalDeviceProperties properties = {0};
        vkGetPhysicalDeviceProperties(device, &properties);
        printf("Checking graphics card: %s\n", properties.deviceName);

        // if (!validateDeviceSurfaceCapabilities(device, surface)) {
        //     printf("Device/surface pair does not have sufficient capabilities");
        // }

        // check for queue family flag support
        // set first supported queue family to foundQFIndex
        uint32_t foundQFIndex = 0;
        {
            bool foundQueueFamily = false;
            uint32_t count = 0;
            VkQueueFamilyProperties* qfProperties = NULL;
            vkGetPhysicalDeviceQueueFamilyProperties(device, &count, NULL);
            qfProperties = malloc(count * sizeof(VkQueueFamilyProperties));
            checkMalloc(qfProperties);
            vkGetPhysicalDeviceQueueFamilyProperties(device, &count, qfProperties);

            for (uint32_t i = 0; i < count; ++i) {
                printf("Queue family %i queueFlags: %i\n", i, qfProperties[i].queueFlags);
                if ((qfProperties[i].queueFlags &
                            REQUIRE_PHYSICAL_DEVICE_QUEUE_FAMILY_FLAG) == 0) {
                    printf("Queue family %i does not have required "
                            "queue family flags\n", i);
                    continue;
                }

                // check if family supports our physical device surface
                VkBool32 supported;
                check(vkGetPhysicalDeviceSurfaceSupportKHR(
                    device, i, surface, &supported
                ));
                if (supported == VK_FALSE) {
                    printf("Queue family %i does not support rendering surface\n",
                            i);
                    continue;
                }
                if (vkGetPhysicalDeviceWin32PresentationSupportKHR(device, i)
                        == VK_FALSE) {
                    printf("Queue family %i does not support Win32 presentation", i);
                    continue;
                }

                foundQFIndex = i;
                foundQueueFamily = true;
                break;
            }

            free(qfProperties);

            if (!foundQueueFamily) {
                printf("Device name %s has not queue families with sufficient support",
                        properties.deviceName);
                continue;
            }
        }

        if (i == 0) {
            chosenPhysicalDevice = device;
            queueFamilyIndex = foundQFIndex;
            printf("Choosing graphics card: %s\n", properties.deviceName);
            printf("Reason: choosing the first card that has sufficient support, will implement manual choice later\n");
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
        int countLimit = 200;
        for (int i = 0; i < MIN(countLimit, (int) propertyCount); ++i) {
            VkExtensionProperties* property = &properties[i];
            printf("%s v%u\n", property->extensionName, property->specVersion);
        }
        if (propertyCount > countLimit) {
            printf("%i more...\n", propertyCount - countLimit);
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

    // init command pool
    VkCommandPoolCreateInfo commandPoolCreateInfo = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
        .queueFamilyIndex = renderContext->queueFamilyIndex,
        .flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
        .pNext = NULL,
    };
    check(vkCreateCommandPool(device, &commandPoolCreateInfo, renderContext->allocationCallbacks, &commandPool));

    VkCommandBufferAllocateInfo cmdAllocInfo = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
        .pNext = NULL,
        .commandPool = commandPool,
        .commandBufferCount = 1,
        .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
    };
    check(vkAllocateCommandBuffers(device, &cmdAllocInfo, &commandBuffer));

    renderContext->physicalDevice = chosenPhysicalDevice;
    renderContext->device = device;
    renderContext->queue = queue;
    renderContext->queueFamilyIndex = queueFamilyIndex;
    renderContext->commandPool = commandPool;
    renderContext->commandBuffer = commandBuffer;

    free(layers);
}

// windows-specific init
#if defined(_WIN32)
#include <windows.h>
RenderContext2 rc_init_win32(HINSTANCE hInstance, HWND hwnd) {
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

    // call instance init
    RenderContext2 context = rc_init_instance(fp_vkGetInstanceProcAddr);

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

    rc_init_device(&context);

    // set up the swapchain
    rc_configure_swapchain(&context);
    printf("Configured swapchain\n");
    rc_init_swapchain(&context, 100, 100);
    printf("Created swapchain\n");
    rc_init_render_pass(&context);
    printf("Created render pass\n");
    rc_init_framebuffers(&context);
    printf("Created framebuffers\n");
    rc_init_sync(&context);
    printf("Created synchronization primitives\n");
    rc_init_pipelines(&context);
    printf("Initialized pipelines\n");

    return context;
}
#endif

#include "app.h"
#include "util.h"
#include "render.h"
#include "render_util.h"
#include <stdbool.h>
#include <vulkan/vulkan_core.h>

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

static void checkMalloc(void* ptr) {
    if (ptr == NULL) {
        const char* msg = "Malloc returned NULL, out of memory!";
        exception_msg(ptr);
    }
}


const char* const ENABLE_EXTENSIONS[] = {
    "VK_KHR_surface", // rendering to monitors
    "VK_KHR_win32_surface",
    "VK_KHR_get_surface_capabilities2", // it doesn't seem to care whether we enable this but oh well
    "VK_EXT_swapchain_colorspace",
};
const size_t ENABLE_EXTENSIONS_COUNT = 4;

// uses VK_MAX_EXTENSION_NAME_SIZE according to:
// https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VkLayerProperties.html
const char* const ENABLE_LAYERS[] = {
    "VK_LAYER_KHRONOS_validation",
};
const size_t ENABLE_LAYERS_COUNT = 1;

const char* const ENABLE_DEVICE_EXTENSIONS[] = {
    "VK_KHR_swapchain",
};
const size_t ENABLE_DEVICE_EXTENSIONS_COUNT = 1;

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

bool validateDeviceSurfaceCapabilities(VkPhysicalDevice device, VkSurfaceKHR surface) {
}

// todo: add ability to enable features, fill out QueueCreateInfo
//https://registry.khronos.org/vulkan/specs/1.3-extensions/html/chap5.html#devsandqueues-device-creation
// https://registry.khronos.org/vulkan/specs/1.3-extensions/html/chap47.html

struct RenderContext rc_init_instance(PFN_vkGetInstanceProcAddr fp_vkGetInstanceProcAddr) {

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

    struct RenderContext renderContext = {
        // make render context for later use
        .instance = instance,
        .physicalDevice = VK_NULL_HANDLE,
        .device = VK_NULL_HANDLE,
        .queue = VK_NULL_HANDLE,
        .queueFamilyIndex = 0,
        .surface = VK_NULL_HANDLE, // defined by platform-specific code
        .allocationCallbacks = allocationCallbacks,
        .swapchain = VK_NULL_HANDLE,
        .surfaceFormat = {0},
    };

    free(extensions);
    free(layers);

    return renderContext;
}

bool validate_surface_capabilities(VkPhysicalDevice device, VkSurfaceKHR surface) {
    return true;
}

void rc_init_device(struct RenderContext* renderContext) {
    if (renderContext->surface == VK_NULL_HANDLE) {
        exception_msg("Surface must be initialized before device can be initialized (reason: used to decide which physical device supports the surface)\n");
    }
    if (renderContext->physicalDevice != VK_NULL_HANDLE) {
        exception_msg("Physical device already chosen\n");
    }
    if (renderContext->device != VK_NULL_HANDLE) {
        exception_msg("Device already chosen\n");
    }

    VkInstance instance = renderContext->instance;
    VkAllocationCallbacks* allocationCallbacks = renderContext->allocationCallbacks;
    VkSurfaceKHR surface = renderContext->surface;
    VkPhysicalDevice chosenPhysicalDevice = NULL;
    VkDevice device = NULL;
    VkQueue queue = NULL;

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

        if (!validateDeviceSurfaceCapabilities(device, surface)) {
            printf("Device/surface pair does not have sufficient capabilities");
        }

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

    renderContext->physicalDevice = chosenPhysicalDevice;
    renderContext->device = device;
    renderContext->queue = queue;
    renderContext->queueFamilyIndex = queueFamilyIndex;

    free(layers);
}

// must be called prior to calling rc_init_swapchain
void rc_configure_swapchain(struct RenderContext* renderContext) {
    if (renderContext->surface == VK_NULL_HANDLE) {
        exception_msg("Must create surface before creating swapchain\n");
    }
    if (renderContext->device == VK_NULL_HANDLE) {
        exception_msg("Must create device before creating swapchain\n");
    }

    VkPhysicalDevice physicalDevice = renderContext->physicalDevice;
    VkDevice device = renderContext->device;
    VkSwapchainKHR swapchain = VK_NULL_HANDLE;
    VkSurfaceKHR surface = renderContext->surface;
    VkSurfaceFormatKHR surfaceFormat = {0};

    // get formats, choose the basic one lol
    bool chosen = false;
    uint32_t pSurfaceFormatCount;
    VkSurfaceFormatKHR* pSurfaceFormats;
    check(vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, surface, &pSurfaceFormatCount, NULL));
    pSurfaceFormats = malloc(pSurfaceFormatCount * sizeof(VkSurfaceFormatKHR));
    checkMalloc(pSurfaceFormats);
    check(vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, surface, &pSurfaceFormatCount, pSurfaceFormats));
    for (uint32_t i = 0; i < pSurfaceFormatCount; ++i) {
        VkSurfaceFormatKHR* format = &pSurfaceFormats[i];
        if (format->format == VK_FORMAT_B8G8R8A8_SRGB &&
                format->colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
            surfaceFormat = *format;
            chosen = true;
            break;
        }
    }
    if (!chosen) {
        exception_msg("Missing required surface format");
    }

    renderContext->surfaceFormat = surfaceFormat;
}

// I think this is basically glViewport, but in this case we also receive a recommendation from the graphics card??????
void rc_init_swapchain(struct RenderContext* renderContext, uint32_t width, uint32_t height) {
    if (renderContext->surface == VK_NULL_HANDLE) {
        exception_msg("Must create surface before creating swapchain\n");
    }
    if (renderContext->device == VK_NULL_HANDLE) {
        exception_msg("Must create device before creating swapchain\n");
    }
    // if (renderContext->swapchain != VK_NULL_HANDLE) {
    //     exception_msg("Already created swapchain\n");
    // }
    if (renderContext->surfaceFormat.format == VK_FORMAT_UNDEFINED) {
        exception_msg("Must decide surface format before creating swapchain\n");
    }

    VkPhysicalDevice physicalDevice = renderContext->physicalDevice;
    VkDevice device = renderContext->device;
    VkSwapchainKHR swapchain = renderContext->swapchain;
    VkSurfaceKHR surface = renderContext->surface;
    VkSurfaceFormatKHR surfaceFormat = renderContext->surfaceFormat;

    // find surface extent and validate surface
    VkExtent2D extent = { 0 };
    {
        void* pNext = NULL;
        /* once I want to do fullscreen
        // will need to use the same parameters for these two structs
        // inside of VkSwapchainCreateInfoKHR for the capabilities reported here
        // to be equivalent
        VkSurfaceFullScreenExclusiveWin32InfoEXT b = {
            .sType = VK_STRUCTURE_TYPE_SURFACE_FULL_SCREEN_EXCLUSIVE_INFO_EXT,
            .pNext = pNext,
            .hmonitor = MonitorFromWindow(...),
        };
        pNext = &b;
        VkSurfaceFullScreenExclusiveInfoEXT a = {
            .sType = VK_STRUCTURE_TYPE_SURFACE_FULL_SCREEN_EXCLUSIVE_INFO_EXT,
            .pNext = pNext,
            .fullScreenExclusive = VK_FULL_SCREEN_EXCLUSIVE_DEFAULT_EXT,
        };
        pNext = &a;
        */
        // with VK_EXT_surface_maintenance1
        VkSurfacePresentModeEXT c4 = {
            .sType = VK_STRUCTURE_TYPE_SURFACE_PRESENT_MODE_EXT,
            .pNext = pNext,
            .presentMode = VK_PRESENT_MODE_FIFO_RELAXED_KHR,
        };
        pNext = &c4;
        // VK_KHR_get_surface_capabilities2
        VkPhysicalDeviceSurfaceInfo2KHR info = {
            .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SURFACE_INFO_2_KHR,
            .pNext = pNext,
            .surface = surface,
        };

        pNext = NULL;
        // doesn't seem useful rn
        // // with VK_KHR_surface_protected_capabilities
        // VkSurfaceProtectedCapabilitiesKHR c1 = {
        //     .sType = VK_STRUCTURE_TYPE_SURFACE_PROTECTED_CAPABILITIES_KHR,
        //     .pNext = pNext,
        // };
        // pNext = &c1;
        VkSurfaceCapabilities2KHR capabilities = {
            .sType = VK_STRUCTURE_TYPE_SURFACE_CAPABILITIES_2_KHR,
            .pNext = pNext,
        };
        check(vkGetPhysicalDeviceSurfaceCapabilities2KHR(physicalDevice, &info, &capabilities));
        // validate capabilities in capabilities.surfaceCapabilities
        // https://github.com/KhronosGroup/Vulkan-Samples/tree/main/samples/performance/swapchain_images
        if (3 < capabilities.surfaceCapabilities.minImageCount ||
            3 > capabilities.surfaceCapabilities.maxImageCount) {
            exception_msg("Device does not support 3 swapchain images\n");
        }

        extent = (VkExtent2D) {
            .width = width,
            .height = height,
        };
        if (extent.width < capabilities.surfaceCapabilities.minImageExtent.width ||
            extent.height < capabilities.surfaceCapabilities.minImageExtent.height) {
            printf("Warning: requested swapchain extent (%u x %u) is smaller than required extent (%u x %u)\n",
                    extent.width,
                    extent.height,
                    capabilities.surfaceCapabilities.minImageExtent.width,
                    capabilities.surfaceCapabilities.minImageExtent.height
                    );
            extent.width = MAX(extent.width, capabilities.surfaceCapabilities.minImageExtent.width);
            extent.height = MAX(extent.height, capabilities.surfaceCapabilities.minImageExtent.height);
        }
        if (extent.width > capabilities.surfaceCapabilities.minImageExtent.width ||
            extent.height > capabilities.surfaceCapabilities.minImageExtent.height) {
            printf("Warning: requested swapchain extent (%u x %u) is larger than required extent (%u x %u)\n",
                    extent.width,
                    extent.height,
                    capabilities.surfaceCapabilities.maxImageExtent.width,
                    capabilities.surfaceCapabilities.maxImageExtent.height
                    );
            extent.width = MIN(extent.width, capabilities.surfaceCapabilities.maxImageExtent.width);
            extent.height = MIN(extent.height, capabilities.surfaceCapabilities.maxImageExtent.height);
        }
    }
    
    VkSwapchainCreateInfoKHR createInfo = {
        .sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
        .pNext = NULL,
        .flags = 0,
        .surface = surface,
        // https://github.com/KhronosGroup/Vulkan-Samples/tree/main/samples/performance/swapchain_images
        .minImageCount = 3, // triple buffering is just better 
        .imageFormat = surfaceFormat.format,
        .imageColorSpace = surfaceFormat.colorSpace,
        .imageExtent = extent,
        .imageArrayLayers = 1, // not VR "stereoscopic 3D"
        // will we need dst/src later?
        .imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT, // we want to use color (not depth RN)
        .imageSharingMode = VK_SHARING_MODE_EXCLUSIVE,
        // we found the queue family earlier
        .queueFamilyIndexCount = 1,
        .pQueueFamilyIndices = &renderContext->queueFamilyIndex,
        .preTransform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR, // not sure if this will work but oh well
        .compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR, // no transparent windows... FOR NOW
        .presentMode = VK_PRESENT_MODE_FIFO_KHR,
        .oldSwapchain = swapchain,
    };
    vkCreateSwapchainKHR(device, &createInfo, renderContext->allocationCallbacks, &swapchain);

    renderContext->swapchain = swapchain;
}

void rc_cleanup(struct RenderContext *renderContext) {
    printf("Cleaning up Vulkan\n");
    if (renderContext->swapchain != VK_NULL_HANDLE) {
        vkDestroySwapchainKHR(
                renderContext->device,
                renderContext->swapchain,
                renderContext->allocationCallbacks);
    }
    if (renderContext->surface != VK_NULL_HANDLE) {
        vkDestroySurfaceKHR(
                renderContext->instance,
                renderContext->surface,
                renderContext->allocationCallbacks);
    }
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

    // call instance init
    struct RenderContext context = rc_init_instance(fp_vkGetInstanceProcAddr);

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

    return context;
}
#endif


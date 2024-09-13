#include "context.h"
#include "util.h"
#include <stdbool.h>
#include <vulkan/vulkan_core.h>
#include "util/memory.h"
#include <stdlib.h>
#include <stdio.h>
#include "util/backtrace.h"
#include <string.h>
#include <assert.h>

const char* const ENABLE_DEVICE_EXTENSIONS[] = {
    "VK_KHR_swapchain",
};
const size_t ENABLE_DEVICE_EXTENSIONS_COUNT = 1;

static void on_destroy_device(void* ptr, sc_t id) {
    VkDevice device = (VkDevice) ptr;
    vkDestroyDevice(device, NULL);
}

InitDevice rc2_init_device(InitDeviceParams params, StaticCache* cleanup) {
    if (params.surface == VK_NULL_HANDLE) {
        exception_msg("Surface must be initialized before device can be initialized (reason: used to decide which physical device supports the surface)\n");
    }

    VkInstance instance = params.instance;
    VkSurfaceKHR surface = params.surface;
    VkPhysicalDevice chosenPhysicalDevice = NULL;
    VkQueue graphicsQueue = NULL;
    VkSurfaceFormatKHR surfaceFormat = {0};
    VkSurfaceCapabilitiesKHR surfaceCapabilities = {0};

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
    uint32_t graphicsQueueFamily = 0;
    for (uint32_t i = 0; i < pPhysicalDeviceCount; ++i) {
        VkPhysicalDevice device = pPhysicalDevices[i];
        VkPhysicalDeviceProperties properties = {0};
        vkGetPhysicalDeviceProperties(device, &properties);
        printf("Checking graphics card: %s\n", properties.deviceName);

        // if (!validateDeviceSurfaceCapabilities(device, surface)) {
        //     printf("Device/surface pair does not have sufficient capabilities");
        // }
        // get formats, choose the basic one lol
        bool chosen = false;
        uint32_t pSurfaceFormatCount;
        VkSurfaceFormatKHR* pSurfaceFormats;
        check(vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &pSurfaceFormatCount, NULL));
        pSurfaceFormats = malloc(pSurfaceFormatCount * sizeof(VkSurfaceFormatKHR));
        checkMalloc(pSurfaceFormats);
        check(vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &pSurfaceFormatCount, pSurfaceFormats));
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
            printf("Queue family %i does not support the required surface "
                    "format VK_FORMAT_B8G8R8A8_SRGB, VK_COLOR_SPACE_SRGB_NONLINEAR_KHR\n",
                    i);
            continue;
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
                if ((qfProperties[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) == 0) {
                    printf("Queue family %i does not have graphics bit\n", i);
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
                // if (vkGetPhysicalDeviceWin32PresentationSupportKHR(device, i)
                //         == VK_FALSE) {
                //     printf("Queue family %i does not support Win32 presentation", i);
                //     continue;
                // }

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
            graphicsQueueFamily = foundQFIndex;
            printf("Choosing graphics card: %s\n", properties.deviceName);
            printf("Reason: choosing the first card that has sufficient support, will implement manual choice later\n");
            break;
        }
    }
    free(pPhysicalDevices);
    if (chosenPhysicalDevice == VK_NULL_HANDLE) {
        exception_msg("No suitable graphics card found\n");
    }

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

        // // log layer extensions
        // if (layerName == NULL) layerName = "DEFAULT";
        // printf("-- Layer %s extensions --\n", layerName);
        // int countLimit = 200;
        // for (int i = 0; i < MIN(countLimit, (int) propertyCount); ++i) {
        //     VkExtensionProperties* property = &properties[i];
        //     printf("%s v%u\n", property->extensionName, property->specVersion);
        // }
        // if (propertyCount > countLimit) {
        //     printf("%i more...\n", propertyCount - countLimit);
        // }
        // printf("-- End Device Extension List --\n");

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

    // grab the min/max supported surface sizes
    {
        void* pNext = NULL;
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
        check(vkGetPhysicalDeviceSurfaceCapabilities2KHR(chosenPhysicalDevice, &info, &capabilities));
        surfaceCapabilities = capabilities.surfaceCapabilities;
    }

    // check for available features and make feature request
    VkPhysicalDeviceFeatures2 features = {0};
    {
        VkPhysicalDevice physDevice = chosenPhysicalDevice;
        features = (VkPhysicalDeviceFeatures2) {
            .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2,
            .pNext = NULL,
        };
        vkGetPhysicalDeviceFeatures2(physDevice, &features);
        // TODO: check for enough physical device features?
        // validateEnabledFeatures(&features);
    }

    // check for format properties
    // TODO: we don't have a prechosen format. do an algo here instead of swapchain init to choose it better
    // {
    //     VkFormatProperties2 formatProperties = {
    //         .sType = VK_STRUCTURE_TYPE_FORMAT_PROPERTIES_2,
    //         .pNext = NULL,
    //     };
    //     vkGetPhysicalDeviceFormatProperties2(chosenPhysicalDevice, surfaceFormat.format, &formatProperties);
    //     // TODO: check for enough format properties?
    //     // validateFormatProperties(&formatProperties);
    // }

    // finally create the device
    VkDevice device = NULL;
    {
        float queuePriority = 1.0f;
        VkDeviceQueueCreateInfo queueCreateInfo = {
            .sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
            .pNext = NULL,
            .flags = 0,
            .queueFamilyIndex = graphicsQueueFamily,
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
            chosenPhysicalDevice, &createInfo, NULL, &device));
        printf("Device initialized\n");
    }
    assert(device != NULL);

    // init all of the vk* functions that are per-device
    init_device_functions(device);
    printf("Device functions initialized\n");

    // get queue
    vkGetDeviceQueue(device, graphicsQueueFamily, 0, &graphicsQueue);

    free(layers);

    StaticCache_add(cleanup, on_destroy_device, (void*) device);
    return (InitDevice) {
        .physicalDevice = chosenPhysicalDevice,
        .device = device,
        .graphicsQueue = graphicsQueue,
        .graphicsQueueFamily = graphicsQueueFamily,
        .surfaceFormat = surfaceFormat,
        .surfaceCapabilities = surfaceCapabilities,
    };
}

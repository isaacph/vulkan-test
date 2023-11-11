#define RC_FUNCTION_DECLARATION
#include "render_functions.h"
#include "vulkan/vulkan_core.h"

static void check(void* res) {
    if (res == NULL) {
        exception_msg("Vulkan function loader returned NULL");
    }
}

void init_loader_functions(PFN_vkGetInstanceProcAddr fp_vkGetInstanceProcAddr) {
    vkGetInstanceProcAddr = fp_vkGetInstanceProcAddr;
    PFN_vkGetInstanceProcAddr load = vkGetInstanceProcAddr;
    check(vkEnumerateInstanceVersion = (PFN_vkEnumerateInstanceVersion)load(VK_NULL_HANDLE, "vkEnumerateInstanceVersion"));
    check(vkEnumerateInstanceExtensionProperties = (PFN_vkEnumerateInstanceExtensionProperties)load(VK_NULL_HANDLE, "vkEnumerateInstanceExtensionProperties"));
    check(vkEnumerateInstanceLayerProperties = (PFN_vkEnumerateInstanceLayerProperties)load(VK_NULL_HANDLE, "vkEnumerateInstanceLayerProperties"));
    check(vkCreateInstance = (PFN_vkCreateInstance)load(VK_NULL_HANDLE, "vkCreateInstance"));
}

void init_instance_functions(VkInstance instance) {
    PFN_vkGetInstanceProcAddr load = vkGetInstanceProcAddr;
    check(vkEnumeratePhysicalDevices = (PFN_vkEnumeratePhysicalDevices)load(instance, "vkEnumeratePhysicalDevices"));
    check(vkEnumerateDeviceExtensionProperties = (PFN_vkEnumerateDeviceExtensionProperties)load(instance, "vkEnumerateDeviceExtensionProperties"));
    check(vkGetPhysicalDeviceFeatures2 = (PFN_vkGetPhysicalDeviceFeatures2)load(instance, "vkGetPhysicalDeviceFeatures2"));
    check(vkGetPhysicalDeviceFormatProperties2 = (PFN_vkGetPhysicalDeviceFormatProperties2)load(instance, "vkGetPhysicalDeviceFormatProperties2"));
    check(vkGetPhysicalDeviceQueueFamilyProperties = (PFN_vkGetPhysicalDeviceQueueFamilyProperties)load(instance, "vkGetPhysicalDeviceQueueFamilyProperties"));
    check(vkCreateDevice = (PFN_vkCreateDevice)load(instance, "vkCreateDevice"));
    check(vkGetDeviceProcAddr = (PFN_vkGetDeviceProcAddr)load(instance, "vkGetDeviceProcAddr"));
    check(vkGetPhysicalDeviceProperties = (PFN_vkGetPhysicalDeviceProperties)load(instance, "vkGetPhysicalDeviceProperties"));
    check(vkDestroyInstance = (PFN_vkDestroyInstance)load(instance, "vkDestroyInstance"));
    check(vkDestroyDevice = (PFN_vkDestroyDevice)load(instance, "vkDestroyDevice"));
#if defined(_WIN32)
    check(vkCreateWin32SurfaceKHR = (PFN_vkCreateWin32SurfaceKHR)load(instance, "vkCreateWin32SurfaceKHR"));
#endif
}

void init_device_functions(VkDevice device) {
    PFN_vkGetDeviceProcAddr load = vkGetDeviceProcAddr;
    check(vkGetDeviceQueue = (PFN_vkGetDeviceQueue)load(device, "vkGetDeviceQueue"));
}


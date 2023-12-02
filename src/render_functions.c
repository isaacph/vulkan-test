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
    check(vkDestroySurfaceKHR = (PFN_vkDestroySurfaceKHR)load(instance, "vkDestroySurfaceKHR"));
    check(vkGetPhysicalDeviceSurfaceSupportKHR = (PFN_vkGetPhysicalDeviceSurfaceSupportKHR)load(instance, "vkGetPhysicalDeviceSurfaceSupportKHR"));
    check(vkGetPhysicalDeviceSurfaceCapabilities2KHR = (PFN_vkGetPhysicalDeviceSurfaceCapabilities2KHR)load(instance, "vkGetPhysicalDeviceSurfaceCapabilities2KHR"));
    check(vkGetPhysicalDeviceSurfaceFormatsKHR = (PFN_vkGetPhysicalDeviceSurfaceFormatsKHR)load(instance, "vkGetPhysicalDeviceSurfaceFormatsKHR"));
#if defined(_WIN32)
    check(vkCreateWin32SurfaceKHR = (PFN_vkCreateWin32SurfaceKHR)load(instance, "vkCreateWin32SurfaceKHR"));
    check(vkGetPhysicalDeviceWin32PresentationSupportKHR = (PFN_vkGetPhysicalDeviceWin32PresentationSupportKHR)load(instance, "vkGetPhysicalDeviceWin32PresentationSupportKHR"));
#endif
}

void init_device_functions(VkDevice device) {
    PFN_vkGetDeviceProcAddr load = vkGetDeviceProcAddr;
    check(vkGetDeviceQueue = (PFN_vkGetDeviceQueue)load(device, "vkGetDeviceQueue"));
    check(vkCreateSwapchainKHR = (PFN_vkCreateSwapchainKHR)load(device, "vkCreateSwapchainKHR"));
    check(vkDestroySwapchainKHR = (PFN_vkDestroySwapchainKHR)load(device, "vkDestroySwapchainKHR"));
    check(vkCreateCommandPool = (PFN_vkCreateCommandPool)load(device, "vkCreateCommandPool"));
    check(vkDestroyCommandPool = (PFN_vkDestroyCommandPool)load(device, "vkDestroyCommandPool"));
    check(vkAllocateCommandBuffers = (PFN_vkAllocateCommandBuffers)load(device, "vkAllocateCommandBuffers"));
    check(vkCreateRenderPass = (PFN_vkCreateRenderPass)load(device, "vkCreateRenderPass"));
    check(vkDestroyRenderPass = (PFN_vkDestroyRenderPass)load(device, "vkDestroyRenderPass"));
    check(vkCreateFramebuffer = (PFN_vkCreateFramebuffer)load(device, "vkCreateFramebuffer"));
    check(vkDestroyFramebuffer = (PFN_vkDestroyFramebuffer)load(device, "vkDestroyFramebuffer"));
    check(vkCreateImageView = (PFN_vkCreateImageView)load(device, "vkCreateImageView"));
    check(vkDestroyImageView = (PFN_vkDestroyImageView)load(device, "vkDestroyImageView"));
    check(vkGetSwapchainImagesKHR = (PFN_vkGetSwapchainImagesKHR)load(device, "vkGetSwapchainImagesKHR"));
    check(vkCreateFence = (PFN_vkCreateFence)load(device, "vkCreateFence"));
    check(vkCreateSemaphore = (PFN_vkCreateSemaphore)load(device, "vkCreateSemaphore"));
    check(vkDestroyFence = (PFN_vkDestroyFence)load(device, "vkDestroyFence"));
    check(vkDestroySemaphore = (PFN_vkDestroySemaphore)load(device, "vkDestroySemaphore"));
    check(vkWaitForFences = (PFN_vkWaitForFences)load(device, "vkWaitForFences"));
    check(vkResetFences = (PFN_vkResetFences)load(device, "vkResetFences"));
    check(vkAcquireNextImageKHR = (PFN_vkAcquireNextImageKHR)load(device, "vkAcquireNextImageKHR"));
    check(vkResetCommandBuffer = (PFN_vkResetCommandBuffer)load(device, "vkResetCommandBuffer"));
    check(vkBeginCommandBuffer = (PFN_vkBeginCommandBuffer)load(device, "vkBeginCommandBuffer"));
    check(vkCmdBeginRenderPass = (PFN_vkCmdBeginRenderPass)load(device, "vkCmdBeginRenderPass"));
    check(vkCmdEndRenderPass = (PFN_vkCmdEndRenderPass)load(device, "vkCmdEndRenderPass"));
    check(vkEndCommandBuffer = (PFN_vkEndCommandBuffer)load(device, "vkEndCommandBuffer"));
    check(vkQueueSubmit = (PFN_vkQueueSubmit)load(device, "vkQueueSubmit"));
    check(vkQueuePresentKHR = (PFN_vkQueuePresentKHR)load(device, "vkQueuePresentKHR"));
}


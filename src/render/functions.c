#define RC_FUNCTION_DECLARATION
#include "functions.h"
#include <stdlib.h>
#include "util/backtrace.h"

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
    check(vkGetPhysicalDeviceMemoryProperties = (PFN_vkGetPhysicalDeviceMemoryProperties)load(instance, "vkGetPhysicalDeviceMemoryProperties"));
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
    check(vkQueueSubmit2 = (PFN_vkQueueSubmit2)load(device, "vkQueueSubmit2"));
    check(vkQueuePresentKHR = (PFN_vkQueuePresentKHR)load(device, "vkQueuePresentKHR"));
    check(vkCreateShaderModule = (PFN_vkCreateShaderModule)load(device, "vkCreateShaderModule"));
    check(vkDestroyShaderModule = (PFN_vkDestroyShaderModule)load(device, "vkDestroyShaderModule"));
    check(vkCreateGraphicsPipelines = (PFN_vkCreateGraphicsPipelines)load(device, "vkCreateGraphicsPipelines"));
    check(vkCreatePipelineLayout = (PFN_vkCreatePipelineLayout)load(device, "vkCreatePipelineLayout"));
    check(vkDestroyPipeline = (PFN_vkDestroyPipeline)load(device, "vkDestroyPipeline"));
    check(vkDestroyPipelineLayout = (PFN_vkDestroyPipelineLayout)load(device, "vkDestroyPipelineLayout"));
    check(vkCmdBindPipeline = (PFN_vkCmdBindPipeline)load(device, "vkCmdBindPipeline"));
    check(vkCmdDraw = (PFN_vkCmdDraw)load(device, "vkCmdDraw"));
    check(vkCmdClearColorImage = (PFN_vkCmdClearColorImage)load(device, "vkCmdClearColorImage"));
    check(vkFreeCommandBuffers = (PFN_vkFreeCommandBuffers)load(device, "vkFreeCommandBuffers"));
    check(vkCmdPipelineBarrier2 = (PFN_vkCmdPipelineBarrier2)load(device, "vkCmdPipelineBarrier2"));
    check(vkCreateImage = (PFN_vkCreateImage)load(device, "vkCreateImage"));
    check(vkDestroyImage = (PFN_vkDestroyImage)load(device, "vkDestroyImage"));
    check(vkAllocateMemory = (PFN_vkAllocateMemory)load(device, "vkAllocateMemory"));
    check(vkGetImageMemoryRequirements = (PFN_vkGetImageMemoryRequirements)load(device, "vkGetImageMemoryRequirements"));
    check(vkBindImageMemory = (PFN_vkBindImageMemory)load(device, "vkBindImageMemory"));
    check(vkFreeMemory = (PFN_vkFreeMemory)load(device, "vkFreeMemory"));
    check(vkCmdBlitImage2 = (PFN_vkCmdBlitImage2)load(device, "vkCmdBlitImage2"));
    check(vkCreateDescriptorPool = (PFN_vkCreateDescriptorPool)load(device, "vkCreateDescriptorPool"));
    check(vkDestroyDescriptorPool = (PFN_vkDestroyDescriptorPool)load(device, "vkDestroyDescriptorPool"));
    check(vkCreateDescriptorSetLayout = (PFN_vkCreateDescriptorSetLayout)load(device, "vkCreateDescriptorSetLayout"));
    check(vkDestroyDescriptorSetLayout = (PFN_vkDestroyDescriptorSetLayout)load(device, "vkDestroyDescriptorSetLayout"));
    check(vkAllocateDescriptorSets = (PFN_vkAllocateDescriptorSets)load(device, "vkAllocateDescriptorSets"));
    check(vkFreeDescriptorSets = (PFN_vkFreeDescriptorSets)load(device, "vkFreeDescriptorSets"));
    check(vkUpdateDescriptorSets = (PFN_vkUpdateDescriptorSets)load(device, "vkUpdateDescriptorSets"));
    check(vkCmdBindDescriptorSets = (PFN_vkCmdBindDescriptorSets)load(device, "vkCmdBindDescriptorSets"));
    check(vkCmdDispatch = (PFN_vkCmdDispatch)load(device, "vkCmdDispatch"));
    check(vkCreateComputePipelines = (PFN_vkCreateComputePipelines)load(device, "vkCreateComputePipelines"));
    check(vkCmdBeginRendering = (PFN_vkCmdBeginRendering)load(device, "vkCmdBeginRendering"));
    check(vkCmdEndRendering = (PFN_vkCmdEndRendering)load(device, "vkCmdEndRendering"));
    check(vkCmdSetViewport = (PFN_vkCmdSetViewport)load(device, "vkCmdSetViewport"));
    check(vkCmdSetScissor = (PFN_vkCmdSetScissor)load(device, "vkCmdSetScissor"));
}


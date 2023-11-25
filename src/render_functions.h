#ifndef RENDER_FUNCTIONS_H_INCLUDED
#define RENDER_FUNCTIONS_H_INCLUDED
// we define the prototypes ourselves
#define VK_NO_PROTOTYPES
#include <vulkan/vk_platform.h>
#include <vulkan/vulkan_core.h>

#if defined(_WIN32)
// some useful stuff for win32
#ifndef UNICODE
#define UNICODE
#endif
#ifndef _UNICODE
#define _UNICODE
#endif
#include <windows.h>

// // a minimal windows include
// // credit https://github.com/zeux/volk/blob/master/volk.h
// /* Instead of including vulkan.h directly, we include individual parts of the SDK
//  * This is necessary to avoid including <windows.h> which is very heavy - it takes
//  * 200ms to parse without WIN32_LEAN_AND_MEAN and 100ms to parse with it. vulkan_win32.h
//  * only needs a few symbols that are easy to redefine ourselves.
//  */
// typedef unsigned long DWORD;
// typedef const wchar_t* LPCWSTR;
// typedef void* HANDLE;
// typedef struct HINSTANCE__* HINSTANCE;
// typedef struct HWND__* HWND;
// typedef struct HMONITOR__* HMONITOR;
// typedef struct _SECURITY_ATTRIBUTES SECURITY_ATTRIBUTES;

#include <vulkan/vulkan_win32.h>
#endif

// init functions
void init_loader_functions(PFN_vkGetInstanceProcAddr fp_vkGetInstanceProcAddr);
void init_instance_functions(VkInstance instance);
void init_device_functions(VkDevice device);

// main vulkan API prototypes are below
// using a macro EXTERN to define these as "extern" via header but as linkable variables
// via render_functions.c
#ifdef RC_FUNCTION_DECLARATION
#define EXTERN extern
#define INIT = NULL
#else
#define EXTERN
#define INIT
#endif

// loader functions
EXTERN PFN_vkGetInstanceProcAddr vkGetInstanceProcAddr INIT;
EXTERN PFN_vkEnumerateInstanceVersion vkEnumerateInstanceVersion INIT;
EXTERN PFN_vkEnumerateInstanceExtensionProperties vkEnumerateInstanceExtensionProperties INIT;
EXTERN PFN_vkEnumerateInstanceLayerProperties vkEnumerateInstanceLayerProperties INIT;
EXTERN PFN_vkCreateInstance vkCreateInstance INIT;

// instance functions
EXTERN PFN_vkEnumeratePhysicalDevices vkEnumeratePhysicalDevices INIT;
EXTERN PFN_vkEnumerateDeviceExtensionProperties vkEnumerateDeviceExtensionProperties INIT;
EXTERN PFN_vkGetPhysicalDeviceFeatures2 vkGetPhysicalDeviceFeatures2 INIT;
EXTERN PFN_vkGetPhysicalDeviceFormatProperties2 vkGetPhysicalDeviceFormatProperties2 INIT;
EXTERN PFN_vkGetPhysicalDeviceQueueFamilyProperties vkGetPhysicalDeviceQueueFamilyProperties INIT;
EXTERN PFN_vkCreateDevice vkCreateDevice INIT;
EXTERN PFN_vkDestroyInstance vkDestroyInstance INIT;
EXTERN PFN_vkGetDeviceProcAddr vkGetDeviceProcAddr INIT;
EXTERN PFN_vkGetPhysicalDeviceProperties vkGetPhysicalDeviceProperties INIT;
EXTERN PFN_vkGetPhysicalDeviceSurfaceSupportKHR vkGetPhysicalDeviceSurfaceSupportKHR INIT;
EXTERN PFN_vkGetPhysicalDeviceSurfaceCapabilities2KHR vkGetPhysicalDeviceSurfaceCapabilities2KHR INIT;
EXTERN PFN_vkDestroyDevice vkDestroyDevice INIT;
EXTERN PFN_vkDestroySurfaceKHR vkDestroySurfaceKHR INIT;
EXTERN PFN_vkGetPhysicalDeviceSurfaceFormatsKHR vkGetPhysicalDeviceSurfaceFormatsKHR INIT;
#if defined(_WIN32)
EXTERN PFN_vkCreateWin32SurfaceKHR vkCreateWin32SurfaceKHR INIT;
EXTERN PFN_vkGetPhysicalDeviceWin32PresentationSupportKHR vkGetPhysicalDeviceWin32PresentationSupportKHR INIT;
#endif

// device functions
EXTERN PFN_vkGetDeviceQueue vkGetDeviceQueue INIT;
EXTERN PFN_vkCreateSwapchainKHR vkCreateSwapchainKHR INIT;
EXTERN PFN_vkDestroySwapchainKHR vkDestroySwapchainKHR INIT;

#undef EXTERN

#endif // RENDER_FUNCTIONS_H_INCLUDED

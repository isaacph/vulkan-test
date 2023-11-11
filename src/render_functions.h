#ifndef RENDER_FUNCTIONS_H_INCLUDED
#define RENDER_FUNCTIONS_H_INCLUDED
// we define the prototypes ourselves
#define VK_NO_PROTOTYPES
#include <vulkan/vk_platform.h>
#include <vulkan/vulkan_core.h>

#if defined(_WIN32)
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
#else
#define EXTERN
#endif

// loader functions
EXTERN PFN_vkGetInstanceProcAddr vkGetInstanceProcAddr;
EXTERN PFN_vkEnumerateInstanceVersion vkEnumerateInstanceVersion;
EXTERN PFN_vkEnumerateInstanceExtensionProperties vkEnumerateInstanceExtensionProperties;
EXTERN PFN_vkEnumerateInstanceLayerProperties vkEnumerateInstanceLayerProperties;
EXTERN PFN_vkCreateInstance vkCreateInstance;
EXTERN PFN_vkDestroyInstance vkDestroyInstance;

// instance functions
EXTERN PFN_vkEnumeratePhysicalDevices vkEnumeratePhysicalDevices;
EXTERN PFN_vkEnumerateDeviceExtensionProperties vkEnumerateDeviceExtensionProperties;
EXTERN PFN_vkGetPhysicalDeviceFeatures2 vkGetPhysicalDeviceFeatures2;
EXTERN PFN_vkGetPhysicalDeviceFormatProperties2 vkGetPhysicalDeviceFormatProperties2;
EXTERN PFN_vkGetPhysicalDeviceQueueFamilyProperties vkGetPhysicalDeviceQueueFamilyProperties;
EXTERN PFN_vkCreateDevice vkCreateDevice;
EXTERN PFN_vkDestroyDevice vkDestroyDevice;
EXTERN PFN_vkGetDeviceProcAddr vkGetDeviceProcAddr;
EXTERN PFN_vkGetPhysicalDeviceProperties vkGetPhysicalDeviceProperties;
#if defined(_WIN32)
EXTERN PFN_vkCreateWin32SurfaceKHR vkCreateWin32SurfaceKHR;
#endif

// device functions
EXTERN PFN_vkGetDeviceQueue vkGetDeviceQueue;

#undef EXTERN

#endif // RENDER_FUNCTIONS_H_INCLUDED

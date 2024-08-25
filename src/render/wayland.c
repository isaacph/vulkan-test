#include "context.h"
#ifdef __linux__
#include "wayland.h"
#include <dlfcn.h>
#include <stdio.h>
#include "../util/backtrace.h"

// this is slightly overwhelming so let's break it down
// 1. load a DLL and get proc address for linux
// 2. figure out all the wayland extensions we need to display a window
// 3. initialize wayland
// 4. initialize each extension

static void* vulkan_handle = NULL;

PFN_vkGetInstanceProcAddr rc_proc_addr() {
    PFN_vkGetInstanceProcAddr fp_vkGetInstanceProcAddr = NULL;

    if (vulkan_handle == NULL) {
        vulkan_handle = dlopen("libvulkan.so", RTLD_LAZY);
        if (!vulkan_handle) {
            char error_msg[1000] = {0};
            snprintf(error_msg, 1000, "Failed to load Vulkan library: %s\n", dlerror());
            exception_msg(error_msg);
        }
        dlerror(); // clear error
    } else {
        printf("Vulkan handle was already loaded\n");
    }

    fp_vkGetInstanceProcAddr = (PFN_vkGetInstanceProcAddr) dlsym(vulkan_handle, "vkGetInstanceProcAddr");
    if (!fp_vkGetInstanceProcAddr) {
        char error_msg[1000] = {0};
        snprintf(error_msg, 1000, "Failed to load Vulkan loader function: %s\n", dlerror());
        exception_msg(error_msg);
    }
    dlerror(); // clear error
    // no reason to close. if we closed we would lose the symbol above anyway

    printf("Found loader func: %p\n", fp_vkGetInstanceProcAddr);
    // sanity check
    if (fp_vkGetInstanceProcAddr(VK_NULL_HANDLE, "vkEnumerateInstanceVersion") == NULL) {
        exception_msg("Invalid vulkan-1.dll");
    }
    return fp_vkGetInstanceProcAddr;
}

// InitSurface rc_init_surface(InitSurfaceParams params, StaticCache* cleanup) {
//     HRESULT hr = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);
//     if (FAILED(hr)) {
//         fprintf(stderr, "Error initializing COM library: %ld", hr);
//         exception();
//     }
// 
//     HINSTANCE hInstance = GetModuleHandle(NULL);
// 
//     // Register the window class.
//     const wchar_t CLASS_NAME[]  = L"Sample Window Class";
//     
//     WNDCLASSW wc = {0};
//     wc.lpfnWndProc = WindowProc;
//     wc.hInstance = hInstance;
//     wc.lpszClassName = CLASS_NAME;
//     wc.hIcon = LoadIcon(0, IDI_APPLICATION);
//     wc.hCursor = LoadCursor(0, IDC_ARROW);
//     wc.hbrBackground = GetStockObject(WHITE_BRUSH);
//     RegisterClassW(&wc);
// 
//     // Create the window.
//     HWND hwnd = CreateWindowEx(
//         0,                              // Optional window styles.
//         CLASS_NAME,                     // Window class
//         L"Learn to Program Windows",    // Window text
//         WS_OVERLAPPEDWINDOW,            // Window style
// 
//         // Size and position
//         CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
// 
//         NULL,       // Parent window    
//         NULL,       // Menu
//         hInstance,  // Instance handle
//         NULL        // Additional application data
//         );
// 
//     if (hwnd == NULL)
//     {
//         DWORD lastError = GetLastError();
//         printf("last error: %lu\n", lastError);
//         exception_msg("Invalid window handle");
//     }
// 
//     ShowWindow(hwnd, SW_SHOW);
// 
//     VkSurfaceKHR surface = VK_NULL_HANDLE;
//     VkWin32SurfaceCreateInfoKHR createInfo = {
//         .sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR,
//         .pNext = NULL,
//         .flags = 0,
//         .hinstance = hInstance,
//         .hwnd = hwnd,
//     };
//     check(vkCreateWin32SurfaceKHR(params.instance, &createInfo, NULL, &surface));
// 
//     return (InitSurface) {
//         .windowHandle = (WindowHandle) {
//             .hInstance = NULL,
//             .hwnd = NULL,
//         },
//         .surface = surface,
//     };
// }

#endif

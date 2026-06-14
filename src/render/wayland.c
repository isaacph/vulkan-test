#include "context.h"
#include "util/memory.h"
#include <assert.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#ifdef __linux__
#include "wayland.h"
#include <dlfcn.h>
#include <stdio.h>
#include "../util/backtrace.h"
#include <wayland-client-core.h>
#include <wayland-client-protocol.h>
#include <wayland-client.h>

// this is slightly overwhelming so let's break it down
// 1. load a DLL and get proc address for linux - complete
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

typedef struct LocalState {
    struct wl_display *display;
    struct wl_registry *registry;
    struct wl_compositor *compositor;
    struct wl_surface *surface;
} LocalState;
static void LocalState_cleanup(void *ptr, sc_t id_ignored) {
    LocalState* ls = (LocalState*) ptr;
    free(ls);
}

static void LocalState_handle_global(void *data, struct wl_registry *wl_registry, uint32_t name, const char *interface, uint32_t version) {
    LocalState* ls = (LocalState*) data;
    if (!strcmp(interface, wl_compositor_interface.name)) {
        ls->compositor = wl_registry_bind(wl_registry, name, &wl_compositor_interface, 4 /* override version to latest at time of writing */);
        printf("Global added to name %d: %s\n", name, interface);
    }
}

static void LocalState_global_remove(void *data, struct wl_registry *wl_registry, uint32_t name) {
    printf("Global name %d removed\n", name);
}

static const struct wl_registry_listener registry_listener = {
    .global = LocalState_handle_global,
    .global_remove = LocalState_global_remove,
};

InitSurface rc_init_surface(InitSurfaceParams params, StaticCache* cleanup) {
    LocalState* ls = (LocalState*) calloc(1, sizeof(LocalState));
    StaticCache_add(cleanup, LocalState_cleanup, (void*) ls);
    // Connect to the Wayland display named name. If name is NULL, its value
    // will be replaced with the WAYLAND_DISPLAY environment variable if it is
    // set, otherwise display "wayland-0" will be used.
    ls->display = wl_display_connect(NULL);
    if (ls->display == NULL) {
        exception_msg("Unable to connect to Wayland");
    }
    printf("Wayland connection established\n");
    ls->registry = wl_display_get_registry(ls->display);
    if (ls->registry == NULL) {
        exception_msg("Missing Wayland registry");
    }
    wl_registry_add_listener(ls->registry, &registry_listener, (void*) ls);
    if (wl_display_roundtrip(ls->display) == -1) {
        printf("Received error code from wl_display_roundtrip\n");
    }
    if (ls->compositor == NULL) {
        exception_msg("Wayland has misbehaved and not supplied a wl_compositor global");
    }
    ls->surface = wl_compositor_create_surface(ls->compositor);
    if (ls->surface == NULL) {
        exception_msg("Wayland compositor refused to create a surface");
    }
    printf("Wayland surface created\n");

    return (InitSurface) {
        .windowHandle = (WindowHandle) {
            .display = ls->display,
            .surface = ls->surface,
        },
        .size = (VkExtent2D) {
            .width = 0,
            .height = 0,
        },
        .surface = NULL,
    };
}

WindowUpdate rc_window_update(WindowHandle* windowHandle) {
    int rc = wl_display_dispatch(windowHandle->display);
    if (rc == -1) {
        printf("Failed to dispatch Wayland display. errno = %d\n", errno);
    }
    return (WindowUpdate) {
        .windowClosed = false,
        .newSize = 0,
        .resize = false,
        .shouldDraw = false,
    };
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

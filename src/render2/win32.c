// windows-specific init
#if defined(_WIN32)
#include "context.h"
#include "util.h"
#include <stdbool.h>
#include "win32.h"
#include <windows.h>
#include <stdio.h>
#include "util/utf.h"
#include <assert.h>

static LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lparam);
static void onDestroyWin32(void* ignored, sc_t id);
typedef struct SurfaceDestroyInfo {
    VkInstance instance;
    VkSurfaceKHR surface;
    HWND window;
    bool windowOwned;
} SurfaceDestroyInfo;

PFN_vkGetInstanceProcAddr rc2_proc_addr() {
    PFN_vkGetInstanceProcAddr fp_vkGetInstanceProcAddr = NULL;

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
    return fp_vkGetInstanceProcAddr;
}

InitSurface rc2_init_surface(InitSurfaceParams params, StaticCache* cleanup) {
    // window size
    int width = CW_USEDEFAULT, height = CW_USEDEFAULT;
    if (params.size.width != DEFAULT_SURFACE_SIZE.width || params.size.height != DEFAULT_SURFACE_SIZE.height) {
        width = params.size.width;
        height = params.size.height;
    }

    HINSTANCE hInstance = params.handle.hInstance;
    HWND hwnd = params.handle.hwnd;
    bool windowOwned = params.handle.hInstance == NULL;

    if (windowOwned) {
        HRESULT hr = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);
        if (FAILED(hr)) {
            fprintf(stderr, "Error initializing COM library: %ld", hr);
            exception();
        }

        printf("Window owned\n");
        hInstance = GetModuleHandle(NULL);

        // Register the window class.
        const wchar_t CLASS_NAME[]  = L"Sample Window Class";
        
        WNDCLASSW wc = {0};
        wc.lpfnWndProc = WindowProc;
        wc.hInstance = hInstance;
        wc.lpszClassName = CLASS_NAME;
        wc.hIcon = LoadIcon(0, IDI_APPLICATION);
        wc.hCursor = LoadCursor(0, IDC_ARROW);
        wc.hbrBackground = GetStockObject(WHITE_BRUSH);
        RegisterClassW(&wc);

        // window name
        wchar_t windowName[40];
        int windowNameLength;
        assert(!utf8_to_utf16(params.title, params.titleLength, windowName, 40, &windowNameLength));

        // window handle
        hwnd = CreateWindowEx(
            0,                              // Optional window styles.
            CLASS_NAME,                     // Window class
            windowName,    // Window text
            WS_OVERLAPPEDWINDOW,            // Window style

            // Size and position
            CW_USEDEFAULT, CW_USEDEFAULT, width, height,

            NULL,       // Parent window    
            NULL,       // Menu
            hInstance,  // Instance handle
            NULL        // Additional application data
            );

        if (hwnd == NULL)
        {
            DWORD lastError = GetLastError();
            printf("last error: %lu\n", lastError);
            exception_msg("Invalid window handle");
        }
    }

    WindowUserData* userData = checkMalloc(malloc(sizeof(WindowUserData)));
    *userData = (WindowUserData) {
        .quit = false,
        .shouldDraw = true,
        .propagateResize = false,
        .resizeQueued = false,
        .newWidth = width,
        .newHeight = height,
    };
    SetWindowLongPtr(hwnd, GWLP_USERDATA, (unsigned long long) userData);
    WindowUserData* testUserData = (WindowUserData*) GetWindowLongPtr(hwnd, GWLP_USERDATA);
    printf("test-ud: %p\n", testUserData);

    // this seems to call WindowProc with a WM_SIZE event, so userData needs to be set
    // before this is set
    RECT windowSize;
    GetClientRect(hwnd, &windowSize);
    width = windowSize.right - windowSize.left;
    height = windowSize.bottom - windowSize.top;
    printf("%d, %d\n", width, height);

    VkSurfaceKHR surface = VK_NULL_HANDLE;
    VkWin32SurfaceCreateInfoKHR createInfo = {
        .sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR,
        .pNext = NULL,
        .flags = 0,
        .hinstance = hInstance,
        .hwnd = hwnd,
    };
    check(vkCreateWin32SurfaceKHR(params.instance, &createInfo, NULL, &surface));
    // add to destroy cache
    SurfaceDestroyInfo* destroyInfo = checkMalloc(malloc(sizeof(SurfaceDestroyInfo)));
    *destroyInfo = (SurfaceDestroyInfo) {
        .instance = params.instance,
        .surface = surface,
        .window = hwnd,
        .windowOwned = windowOwned,
    };
    StaticCache_add(cleanup, onDestroyWin32, (void*) destroyInfo);

    // this needs to happen after most of the other init, otherwise we end up sending WindowProc
    // calls with a NULL userData pointer
    if (windowOwned && !params.headless) {
        printf("Show window %p\n", hwnd);
        ShowWindow(hwnd, SW_SHOW);
    }

    return (InitSurface) {
        .windowHandle = (WindowHandle) {
            .hInstance = hInstance,
            .hwnd = hwnd,
            .userData = userData,
        },
        .surface = surface,
        .size = (VkExtent2D) {
            .width = width,
            .height = height,
        },
    };
}

static void onDestroyWin32(void* castToSurface, sc_t id) {
    SurfaceDestroyInfo* destroyInfo = (SurfaceDestroyInfo*) castToSurface;

    vkDestroySurfaceKHR(destroyInfo->instance, destroyInfo->surface, NULL);

    if (destroyInfo->windowOwned) {
        DestroyWindow(destroyInfo->window);
        CoUninitialize();
        free(destroyInfo);
    }
}

static LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    // printf("Event: %u\n", uMsg);
    RECT rect;
    WindowUserData* userData = (WindowUserData*) GetWindowLongPtr(hwnd, GWLP_USERDATA);
    switch (uMsg) {
    case WM_SYSCOMMAND:
        // printf("Sys: %Iu\n", wParam);
        if (wParam == SC_MINIMIZE) {
            userData->resizeQueued = true;
        } else if (wParam == SC_RESTORE) {
            userData->resizeQueued = true;
        }
        return DefWindowProc(hwnd, uMsg, wParam, lParam);
    case WM_DESTROY:
        PostQuitMessage(0);
        userData->quit = true;
        userData->shouldDraw = false;
        return 0;
    case WM_LBUTTONDOWN:
        printf("Received left mouse button\n");
        return 0;
    case WM_PAINT:
        ValidateRect(hwnd, NULL);
        return 0;
    case WM_SIZE:
        if (wParam != SIZE_MINIMIZED && (userData->resizeQueued || wParam == SIZE_MAXIMIZED)) {
            userData->resizeQueued = true;
            userData->newHeight = (int) (lParam >> 16) & 0xFFFF;
            userData->newWidth = (int) lParam & 0xFFFF;
            printf("ud: %p\n", userData);
            printf("rq: %b\n", userData->resizeQueued);
            printf("new size: %d x %d\n", userData->newWidth, userData-> newHeight);
        }
        return 0;
    case WM_ENTERSIZEMOVE:
        userData->resizeQueued = true;
        printf("Enter size move\n");
        return 0;
    case WM_EXITSIZEMOVE:
        userData->resizeQueued = true;
        printf("Exit size move\n");
        return 0;
    }
    return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

// returns true if should quit
WindowUpdate rc2_window_update(WindowHandle* windowHandle) {
    MSG msg = {0};
    while (PeekMessage(&msg, windowHandle->hwnd, 0, 0, PM_REMOVE) != 0) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
    bool resize = false;
    VkExtent2D resizeExtent = { 0 };
    if (windowHandle->userData->propagateResize) {
        windowHandle->userData->resizeQueued = false;
        windowHandle->userData->propagateResize = false;
        resizeExtent = (VkExtent2D) {
            .width = windowHandle->userData->newWidth,
            .height = windowHandle->userData->newHeight,
        };
        resize = true;
    }
    return (WindowUpdate) {
        .windowClosed = windowHandle->userData->quit,
        .shouldDraw = windowHandle->userData->shouldDraw,
        .requireResize = resize,
        .resize = resizeExtent,
    };
}

// RenderContext rc2_init_win32(HINSTANCE hInstance, HWND hwnd) {
//     // load the Vulkan loader
//     PFN_vkGetInstanceProcAddr fp_vkGetInstanceProcAddr = rc2_proc_addr();
//     StaticCache cleanup = StaticCache_init(1000);
// 
//     RenderContext context = {
//         // make render context for later use
//         .instance = VK_NULL_HANDLE,
//         .physicalDevice = VK_NULL_HANDLE,
//         .device = VK_NULL_HANDLE,
//         .surface = VK_NULL_HANDLE, // defined by platform-specific code
//         .swapchain = VK_NULL_HANDLE,
//         .surfaceFormat = {0},
//         .swapchainExtent = {0},
//         .graphicsQueue = VK_NULL_HANDLE,
//         .graphicsQueueFamily = 0,
//         .frameNumber = 0,
//         .frames = {0},
//         .images = {0},
//     };
//     FrameData emptyFrame = {
//         .commandPool = VK_NULL_HANDLE,
//         .mainCommandBuffer = VK_NULL_HANDLE,
//         .swapchainSemaphore = VK_NULL_HANDLE,
//         .renderSemaphore = VK_NULL_HANDLE,
//         .renderFence = VK_NULL_HANDLE,
//     };
//     SwapchainImageData emptyImage = {
//         .swapchainImage = VK_NULL_HANDLE,
//         .swapchainImageView = VK_NULL_HANDLE,
//     };
//     for (int i = 0; i < RC_SWAPCHAIN_LENGTH; ++i) {
//         context.frames[i] = emptyFrame;
//         context.images[i] = emptyImage;
//     }
// 
//     // call instance init
//     {
//         InitInstance initInstance = rc2_init_instance(fp_vkGetInstanceProcAddr, true, &cleanup);
//         context.instance = initInstance.instance;
//     }
// 
//     // make win32 surface
//     {
//         VkWin32SurfaceCreateInfoKHR createInfo = {
//             .sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR,
//             .pNext = NULL,
//             .flags = 0,
//             .hinstance = hInstance,
//             .hwnd = hwnd,
//         };
//         check(vkCreateWin32SurfaceKHR(context.instance, &createInfo, NULL, &context.surface));
//     }
//     printf("Created win32 surface\n");
// 
//     // make device
//     {
//         InitDevice initDevice = rc2_init_device((InitDeviceParams) {
//             .instance = context.instance,
//             .surface = context.surface,
//             .surfaceFormat = context.surfaceFormat,
//         });
//         context.physicalDevice = initDevice.physicalDevice;
//         context.device = initDevice.device;
//         context.graphicsQueue = initDevice.queue;
//         context.graphicsQueueFamily = initDevice.graphicsQueueFamily;
//     }
// 
//     // set up the swapchain
//     rc2_configure_swapchain(&context);
//     printf("Configured swapchain\n");
//     rc2_init_swapchain(&context, 100, 100);
//     printf("Created swapchain\n");
//     // rc2_init_render_pass(&context);
//     // printf("Created render pass\n");
//     // rc2_init_framebuffers(&context);
//     // printf("Created framebuffers\n");
//     rc2_init_loop(&context);
//     printf("Created things for the loop\n");
//     // rc2_init_pipelines(&context);
//     // printf("Initialized pipelines\n");
// 
//     return context;
// }



#endif

#include "render2/win32.h"
#include <assert.h>
#include "util.h"
#include "util/memory.h"
#ifndef UNICODE
#define UNICODE
#endif
#ifndef _UNICODE
#define _UNICODE
#endif
#include <windows.h>
#include <stdio.h>
#include "render2/context.h"
#include "render/context.h"
#include "backtrace.h"
#include <math.h>

// making a bunch of modes
// all modes will have the actual game in a separate process
// the original process is there to handle crashes, handle logging, and start the game
// winmain mode will start in winmain, and redirect console to a log file
// crt mode will start in _main, and redirect console to the original console:q
//

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lparam);
RenderContext renderContext;
WindowHandle windowHandle;
sc_t swapchainCleanupHandle;
StaticCache cleanup;
bool resizeMode = false;
bool resetDrawBounds = false;
bool disableDraw = false;
bool running = true;
int frameNumber = 0;

void draw(HWND hwnd) {
    if (resetDrawBounds || windowHandle.userData->resizeQueued) {
        printf("resize?\n");
        RECT rect;
        printf("%p vs %p\n", windowHandle.hwnd, hwnd);
        if (GetClientRect(windowHandle.hwnd, &rect)) {
            uint32_t width = rect.right - rect.left;
            uint32_t height = rect.bottom - rect.top;
            // rc_size_change(&renderContext, width, height);
            InitSwapchainParams params = {
                .extent = (VkExtent2D) {
                    .width = width,
                    .height = height,
                },
                .physicalDevice = renderContext.physicalDevice,
                .graphicsQueueFamily = renderContext.graphicsQueueFamily,
                .device = renderContext.device,
                .surface = renderContext.surface,
                .surfaceFormat = renderContext.surfaceFormat,
                .oldSwapchain = renderContext.swapchain,
                .swapchainCleanupHandle = swapchainCleanupHandle,
            };
            InitSwapchain ret = rc2_init_swapchain(params, &cleanup);
            renderContext.swapchain = ret.swapchain;
            swapchainCleanupHandle = ret.swapchainCleanupHandle;
            for (int i = 0; i < RC_SWAPCHAIN_LENGTH; ++i) {
                renderContext.images[i] = ret.images[i];
            }
        }
        resetDrawBounds = false;
        windowHandle.userData->resizeQueued = false;
    } else if (windowHandle.userData->shouldDraw) {
        // rc_draw(&renderContext);
        ++frameNumber;
        float flash = fabs(sin(frameNumber / 120.));
        DrawParams params = {
			.device = renderContext.device,
			.swapchain = renderContext.swapchain,
			.frame = renderContext.frames[frameNumber % FRAME_OVERLAP],
			.color = flash,
			.graphicsQueue = renderContext.graphicsQueue,
        };
        for (int i = 0; i < RC_SWAPCHAIN_LENGTH; ++i) {
            params.swapchainImages[i] = renderContext.images[i];
        }
        rc2_draw(params);
    }
}

// int WinMain(
//         HINSTANCE hInstance,
//         HINSTANCE hPrevInstance,
//         LPSTR lpCmdLine,
//         int nCmdShow) {
int winmain() {
    swapchainCleanupHandle = SC_ID_NONE;
    init_exceptions(false);
    cleanup = StaticCache_init(1000);

    // HRESULT hr = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);
    // if (FAILED(hr)) {
    //     printf("Error initializing COM library: %ld", hr);
    //     return 1;
    // }

    // HINSTANCE hInstance = GetModuleHandle(NULL);
    // windowHandle.hInstance = hInstance;

    // // Register the window class.
    // const wchar_t CLASS_NAME[]  = L"Sample Window Class";
    // 
    // WNDCLASSW wc = {0};
    // wc.lpfnWndProc = WindowProc;
    // wc.hInstance = hInstance;
    // wc.lpszClassName = CLASS_NAME;
    // wc.hIcon = LoadIcon(0, IDI_APPLICATION);
    // wc.hCursor = LoadCursor(0, IDC_ARROW);
    // wc.hbrBackground = GetStockObject(WHITE_BRUSH);
    // RegisterClassW(&wc);

    // // Create the window.
    // HWND hwnd = CreateWindowEx(
    //     0,                              // Optional window styles.
    //     CLASS_NAME,                     // Window class
    //     L"Learn to Program Windows",    // Window text
    //     WS_OVERLAPPEDWINDOW,            // Window style

    //     // Size and position
    //     CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,

    //     NULL,       // Parent window    
    //     NULL,       // Menu
    //     hInstance,  // Instance handle
    //     NULL        // Additional application data
    //     );
    // windowHandle.hwnd = hwnd;
    // printf("Assigned hwnd to %p\n", hwnd);

    // if (hwnd == NULL)
    // {
    //     DWORD lastError = GetLastError();
    //     printf("last error: %lu\n", lastError);
    //     exception_msg("Invalid window handle");
    //     return 0;
    // }

    // ShowWindow(hwnd, SW_SHOW);

    // renderContext = rc_init_win32(hInstance, hwnd);
    renderContext = (RenderContext) { 0 };
    {
        {
            PFN_vkGetInstanceProcAddr proc_addr = rc2_proc_addr();
            InitInstance init = rc2_init_instance(proc_addr, false, &cleanup);
            renderContext.instance = init.instance;
            assert(renderContext.instance != VK_NULL_HANDLE);
        }
        {
            const char* title = "Test window! \xF0\x9F\x87\xBA\xF0\x9F\x87\xB8";
            InitSurfaceParams params = {
                .instance = renderContext.instance,
                .title = title,
                .titleLength = strlen(title),
                .size = DEFAULT_SURFACE_SIZE,
                .headless = false,
                .handle = windowHandle,
            };
            InitSurface ret = rc2_init_surface(params, &cleanup);
            renderContext.surface = ret.surface;
            windowHandle = ret.windowHandle;
            renderContext.drawExtent = ret.size;
            assert(renderContext.surface != NULL);
            assert(renderContext.drawExtent.width != 0);
            assert(renderContext.drawExtent.height != 0);
            assert(renderContext.drawExtent.width != DEFAULT_SURFACE_SIZE.width && renderContext.drawExtent.height != DEFAULT_SURFACE_SIZE.height);
        }
        {
            // so we need to refactor the logic so that surface format is chosen when physical device is chosen
            InitDeviceParams params = {
                .instance = renderContext.instance,
                .surface = renderContext.surface,
            };
            InitDevice ret = rc2_init_device(params, &cleanup);
            renderContext.device = ret.device;
            renderContext.surfaceFormat = ret.surfaceFormat;
            renderContext.graphicsQueueFamily = ret.graphicsQueueFamily;
            renderContext.physicalDevice = ret.physicalDevice;
            renderContext.graphicsQueue = ret.graphicsQueue;
            assert(ret.device != NULL);
        }
        {
            InitSwapchainParams params = {
                .extent = renderContext.drawExtent,

                .device = renderContext.device,
                .physicalDevice = renderContext.physicalDevice,
                .surface = renderContext.surface,
                .surfaceFormat = renderContext.surfaceFormat,
                .graphicsQueueFamily = renderContext.graphicsQueueFamily,

                .oldSwapchain = VK_NULL_HANDLE,
                .swapchainCleanupHandle = swapchainCleanupHandle,
            };
            InitSwapchain ret = rc2_init_swapchain(params, &cleanup);
            renderContext.swapchain = ret.swapchain;
            for (int i = 0; i < RC_SWAPCHAIN_LENGTH; ++i) {
                renderContext.images[i] = ret.images[i];
            }
            swapchainCleanupHandle  = ret.swapchainCleanupHandle;
            assert(renderContext.swapchain != NULL);
        }
        {
            InitLoopParams params = {
                .device = renderContext.device,
                .graphicsQueueFamily = renderContext.graphicsQueueFamily,
            };
            InitLoop ret = rc2_init_loop(params, &cleanup);
            for (int i = 0; i < FRAME_OVERLAP; ++i) {
                assert(ret.frames[i].commandPool != VK_NULL_HANDLE);
                renderContext.frames[i] = ret.frames[i];
            }
        }
        DrawParams params = {
            .device = renderContext.device,
            .swapchain = renderContext.swapchain,
            .graphicsQueue = renderContext.graphicsQueue,
            .swapchainImages = { 0 },
        };
        for (int i = 0; i < RC_SWAPCHAIN_LENGTH; ++i) {
            params.swapchainImages[i] = renderContext.images[i];
        }
    }

    // bool running = true;
    // while (running) {
    //     WindowUpdate update = rc2_window_update(&windowHandle);
    //     running = !update.windowClosed;
    //     // assert(update.shouldDraw); // break this by minimizing :)

    //     if (update.requireResize) {
    //         renderContext.drawExtent = update.resize;
    //         printf("new size: %d x %d\n", renderContext.drawExtent.width, renderContext.drawExtent.height);
    //         InitSwapchainParams params = {
    //             .extent = renderContext.drawExtent,

    //             .device = renderContext.device,
    //             .physicalDevice = renderContext.physicalDevice,
    //             .surface = renderContext.surface,
    //             .surfaceFormat = renderContext.surfaceFormat,
    //             .graphicsQueueFamily = renderContext.graphicsQueueFamily,

    //             .oldSwapchain = renderContext.swapchain,
    //             .swapchainCleanupHandle = swapchainCleanupHandle,
    //         };
    //         InitSwapchain ret = rc2_init_swapchain(params, &cleanup);
    //         renderContext.swapchain = ret.swapchain;
    //         for (int i = 0; i < RC_SWAPCHAIN_LENGTH; ++i) {
    //             renderContext.images[i] = ret.images[i];
    //         }
    //         swapchainCleanupHandle = ret.swapchainCleanupHandle;
    //     }

    //     if (update.shouldDraw) {
    //         printf("draw\n");
    //         DrawParams params = {
    //             .device = renderContext.device,
    //             .swapchain = renderContext.swapchain,
    //             .graphicsQueue = renderContext.graphicsQueue,
    //             .swapchainImages = { 0 },
    //             .frame = renderContext.frames[frameNumber % 2],
    //             .color = fabs(sin(frameNumber / 120.f)),
    //         };
    //         for (int i = 0; i < RC_SWAPCHAIN_LENGTH; ++i) {
    //             params.swapchainImages[i] = renderContext.images[i];
    //         }
    //         rc2_draw(params);
    //     }
    // }

    // Run the message loop.

    MSG msg = {0};
    while (running && !windowHandle.userData->quit) {
        draw(windowHandle.hwnd);
        while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE) != 0) {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }

    // rc_destroy(&renderContext);
    CoUninitialize();

    StaticCache_clean_up(&cleanup);
    return 0;
}

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    // printf("Event: %u\n", uMsg);
    RECT rect;
    switch (uMsg) {
    case WM_SYSCOMMAND:
        // printf("Sys: %Iu\n", wParam);
        if (wParam == SC_MINIMIZE) {
            disableDraw = true;
        } else if (wParam == SC_RESTORE) {
            disableDraw = false;
        }
        return DefWindowProc(hwnd, uMsg, wParam, lParam);
    case WM_DESTROY:
        PostQuitMessage(0);
        running = false;
        disableDraw = true;
        return 0;
    case WM_LBUTTONDOWN:
        printf("Received left mouse button\n");
        return 0;
    case WM_PAINT:
        ValidateRect(hwnd, NULL);
        return 0;
    case WM_SIZE:
        if (wParam != SIZE_MINIMIZED && (resizeMode || wParam == SIZE_MAXIMIZED)) {
            resetDrawBounds = true;
        }
        return 0;
    case WM_ENTERSIZEMOVE:
        resizeMode = true;
        return 0;
    case WM_EXITSIZEMOVE:
        resizeMode = true;
        return 0;
    }
    return DefWindowProc(hwnd, uMsg, wParam, lParam);
}


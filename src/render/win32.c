#include "context.h"
#include "util.h"
#include <stdbool.h>

// windows-specific init
#if defined(_WIN32)
#include <windows.h>
RenderContext rc_init_win32(HINSTANCE hInstance, HWND hwnd) {
    // load the Vulkan loader
    PFN_vkGetInstanceProcAddr fp_vkGetInstanceProcAddr;
    {
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
    }

    RenderContext context = {
        // make render context for later use
        .instance = VK_NULL_HANDLE,
        .physicalDevice = VK_NULL_HANDLE,
        .device = VK_NULL_HANDLE,
        .surface = VK_NULL_HANDLE, // defined by platform-specific code
        .swapchain = VK_NULL_HANDLE,
        .surfaceFormat = {0},
        .swapchainExtent = {0},
        .graphicsQueue = VK_NULL_HANDLE,
        .graphicsQueueFamily = 0,
        .frameNumber = 0,
        .frames = {0},
        .images = {0},
    };
    FrameData emptyFrame = {
        .commandPool = VK_NULL_HANDLE,
        .mainCommandBuffer = VK_NULL_HANDLE,
        .swapchainSemaphore = VK_NULL_HANDLE,
        .renderSemaphore = VK_NULL_HANDLE,
        .renderFence = VK_NULL_HANDLE,
    };
    SwapchainImageData emptyImage = {
        .swapchainImage = VK_NULL_HANDLE,
        .swapchainImageView = VK_NULL_HANDLE,
    };
    for (int i = 0; i < RC_SWAPCHAIN_LENGTH; ++i) {
        context.frames[i] = emptyFrame;
        context.images[i] = emptyImage;
    }

    // call instance init
    {
        InitInstance initInstance = rc_init_instance(fp_vkGetInstanceProcAddr);
        context.instance = initInstance.instance;
    }

    // make win32 surface
    {
        VkWin32SurfaceCreateInfoKHR createInfo = {
            .sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR,
            .pNext = NULL,
            .flags = 0,
            .hinstance = hInstance,
            .hwnd = hwnd,
        };
        check(vkCreateWin32SurfaceKHR(context.instance, &createInfo, NULL, &context.surface));
    }
    printf("Created win32 surface\n");

    // make device
    {
        InitDevice initDevice = rc_init_device((InitDeviceParams) {
            .instance = context.instance,
            .surface = context.surface,
            .surfaceFormat = context.surfaceFormat,
        });
        context.physicalDevice = initDevice.physicalDevice;
        context.device = initDevice.device;
        context.graphicsQueue = initDevice.queue;
        context.graphicsQueueFamily = initDevice.graphicsQueueFamily;
    }

    // set up the swapchain
    rc_configure_swapchain(&context);
    printf("Configured swapchain\n");
    rc_init_swapchain(&context, 100, 100);
    printf("Created swapchain\n");
    // rc_init_render_pass(&context);
    // printf("Created render pass\n");
    // rc_init_framebuffers(&context);
    // printf("Created framebuffers\n");
    rc_init_loop(&context);
    printf("Created things for the loop\n");
    // rc_init_pipelines(&context);
    // printf("Initialized pipelines\n");

    return context;
}
#endif

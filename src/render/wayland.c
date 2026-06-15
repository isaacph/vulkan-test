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
#include <xdg_shell.h>
#include <sys/mman.h>        /* shm_open */
#include <sys/stat.h>        /* For mode constants */
#include <fcntl.h>           /* For O_* constants */
#include <unistd.h>          /* For ftruncate */
#include "../util/uuid.h"
#include <poll.h>

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
    struct wl_shm *shm;
    struct wl_buffer *buffer;
    struct xdg_wm_base *xdg_base;
    struct xdg_surface *xdg_surface;
    struct xdg_toplevel *xdg_toplevel;
    int wayland_fd;
    bool configured;
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
    if (!strcmp(interface, wl_shm_interface.name)) {
        ls->shm = wl_registry_bind(wl_registry, name, &wl_shm_interface, 1);
        printf("Global added to name %d: %s\n", name, interface);
    }
    if (!strcmp(interface, xdg_wm_base_interface.name)) {
        ls->xdg_base = wl_registry_bind(wl_registry, name, &xdg_wm_base_interface, 1);
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

static void xdg_wm_base_ping(void *data, struct xdg_wm_base *xdg_base, uint32_t serial) {
    xdg_wm_base_pong(xdg_base, serial);
}

static const struct xdg_wm_base_listener xdg_wm_base_listener = {
    .ping = xdg_wm_base_ping,
};

static void wl_buffer_release(void *data, struct wl_buffer *wl_buffer) {
    wl_buffer_destroy(wl_buffer);
}

static const struct wl_buffer_listener wl_buffer_listener = {
    .release = wl_buffer_release,
};

static void xdg_surface_configure(void *data, struct xdg_surface *xdg_surface, uint32_t serial) {
    LocalState *ls = (LocalState *)data;
    int ret = 0;
    // TODO: only ack once we have applied state changes requested by the xdg surface
    printf("Preparing to ACK\n");
    ls->configured = true;
    xdg_surface_ack_configure(xdg_surface, serial);

    // TODO: move to helper
    if (ls->shm == NULL) {
        exception_msg("Wayland has misbehaved and not supplied a wl_shm global");
    }
    // to find valid pixel formats: wl_shm_add_listener();
    // however: "Two formats are required to be supported: ARGB8888, and XRGB8888, which are 24-bit color, with and without an alpha channel respectively."
    char uuid[128], base_uuid[] = "/wl_shm-";
    memset(uuid, 0, sizeof(uuid));
    memcpy(uuid, base_uuid, sizeof(base_uuid));
    uuid_to_string(generate_uuid(), uuid + sizeof(base_uuid) - 1, 36);
    int fd = shm_open(uuid, O_EXCL | O_CREAT | O_RDWR, 0600);
    if (fd == -1) {
        exception_format("Failed to open shared memory file. errno = %d", errno);
    }
    // hardcoding for now - TODO
    const int width = 1920, height = 1080, bytes_per_pixel = 4, num_buffers = 2;
    const int shm_size = width * height * bytes_per_pixel * num_buffers;
    ret = 0;
    do {
        ret = ftruncate(fd, shm_size);
    } while (ret == 0 && errno == EINTR);
    if (ret == -1) {
        exception_format("Failed to allocate shared memory buffer of size %d: errno = %d", shm_size, errno);
    }
    uint8_t *shm_data = mmap(NULL, shm_size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (shm_data == MAP_FAILED) {
        exception_format("Failed to map shared memory buffer to memory. errno = %d", errno);
    }
    struct wl_shm_pool *shm_pool = wl_shm_create_pool(ls->shm, fd, shm_size);
    ls->buffer = wl_shm_pool_create_buffer(shm_pool, 0, width, height, width * bytes_per_pixel, WL_SHM_FORMAT_ARGB8888);
    wl_buffer_add_listener(ls->buffer, &wl_buffer_listener, ls);
    wl_shm_pool_destroy(shm_pool);
    close(fd);
    uint32_t *pixels = (uint32_t *)&shm_data[0];
    memset(pixels, 0, width * height * bytes_per_pixel);
    // checkerboard pattern for testing
    for (int y = 0; y < height; ++y) {
      for (int x = 0; x < width; ++x) {
        if ((x + y / 8 * 8) % 16 < 8) {
          pixels[y * width + x] = 0xFF666666;
        } else {
          pixels[y * width + x] = 0xFFEEEEEE;
        }
      }
    }

    // we cannot commit the surface yet. We have to wait until the configure call!

    munmap(shm_data, shm_size);

    wl_surface_attach(ls->surface, ls->buffer, 0, 0);
    // wl_surface_damage(ls->surface, 0, 0, INT32_MAX, INT32_MAX);
    wl_surface_commit(ls->surface);
    printf("Commited the surface\n");
}

static const struct xdg_surface_listener xdg_surface_listener = {
	.configure = xdg_surface_configure,
};

InitSurface rc_init_surface(InitSurfaceParams params, StaticCache* cleanup) {
    int ret = 0;
    LocalState* ls = (LocalState*) calloc(1, sizeof(LocalState));
    StaticCache_add(cleanup, LocalState_cleanup, (void*) ls);
    // Connect to the Wayland display named name. If name is NULL, its value
    // will be replaced with the WAYLAND_DISPLAY environment variable if it is
    // set, otherwise display "wayland-0" will be used.
    ls->display = wl_display_connect(NULL);
    if (ls->display == NULL) {
        exception_msg("Unable to connect to Wayland");
    }
    ls->wayland_fd = wl_display_get_fd(ls->display);
    printf("Wayland connection established\n");
    ls->registry = wl_display_get_registry(ls->display);
    if (ls->registry == NULL) {
        exception_msg("Missing Wayland registry");
    }
    wl_registry_add_listener(ls->registry, &registry_listener, (void*) ls);
    printf("Forcing roundtrip\n");
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

    if (ls->xdg_base == NULL) {
        exception_msg("Wayland has misbehaved and not supplied a xdg_wm_base global");
    }
    xdg_wm_base_add_listener(ls->xdg_base, &xdg_wm_base_listener, (void *)ls);
    ls->xdg_surface = xdg_wm_base_get_xdg_surface(ls->xdg_base, ls->surface);
    xdg_surface_add_listener(ls->xdg_surface, &xdg_surface_listener, (void *)ls);

    ls->xdg_toplevel = xdg_surface_get_toplevel(ls->xdg_surface);
    xdg_toplevel_set_title(ls->xdg_toplevel, "Test wayland app");
    xdg_toplevel_set_app_id(ls->xdg_toplevel, "isaacph-vulkan-test");
    wl_surface_commit(ls->surface);


    printf("Wayland surface created\n");
    return (InitSurface) {
        .windowHandle = (WindowHandle) {
            .localState = ls,
        },
        .size = (VkExtent2D) {
            .width = 0,
            .height = 0,
        },
        .surface = NULL,
    };
}

WindowUpdate rc_window_update(WindowHandle* windowHandle) {
    LocalState *ls = windowHandle->localState;
    int rc = 0;
    // replace this whole block with wl_display_dispatch to block instead of loop
    {
        struct pollfd fds[1];
        fds[0].fd = ls->wayland_fd;
        fds[0].events = POLLIN;
        while (wl_display_prepare_read(ls->display) != 0) {
            wl_display_dispatch_pending(ls->display);
        }
        rc = wl_display_flush(ls->display);
        if (rc == -1) {
            printf("Display file descriptor was overloaded. Need to try again\n");
            return (WindowUpdate) {
                .windowClosed = false,
                .newSize = 0,
                .resize = false,
                .shouldDraw = false,
            };
        }
        rc = poll(fds, 1, 0);
        if (rc > 0 && (fds[0].revents & POLLIN)) {
            rc = wl_display_read_events(ls->display);
            if (rc == -1) {
                exception_format("Failed to read events from wayland display. errno = %d\n", errno);
            }
            rc = wl_display_dispatch_pending(ls->display);
            if (rc == -1) {
                exception_format("Failed to dispatch events from wayland display. errno = %d\n", errno);
            }
        } else {
            wl_display_cancel_read(ls->display);
        }
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

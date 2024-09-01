#include <sodium/randombytes.h>
#include <stdio.h>
#include <string.h>
#include <unity.h>
#include <util/backtrace.h>
#include <wayland-client-core.h>
#include <wayland-client-protocol.h>
#include <wayland-client.h>
#include <assert.h>
#include <sodium.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>
#include <errno.h>
#include <xdg_shell.h>

void setUp(void) {
}
void tearDown(void) {
}

void test_wl_connect(void) {
    // Connect to the Wayland display named name. If name is NULL, its value
    // will be replaced with the WAYLAND_DISPLAY environment variable if it is
    // set, otherwise display "wayland-0" will be used.
    struct wl_display *display = wl_display_connect(NULL);
    assert(display != NULL);
    wl_display_disconnect(display);
}

void test_wl_get_registry_global(void *data,
           struct wl_registry *wl_registry,
           uint32_t name,
           const char *interface,
           uint32_t version) {
    // use for debugging object versions
    // printf("New global object: %s at %d, v%d\n", interface, name, version);
}
void test_wl_get_registry_global_remove(void *data,
              struct wl_registry *wl_registry,
              uint32_t name) {
    // printf("Removed global object: %d\n", name);
}
void test_wl_get_registry(void) {
    struct wl_display *display = wl_display_connect(NULL);
    assert(display != NULL);
    struct wl_registry *registry = wl_display_get_registry(display);
    assert(registry != NULL);
    struct wl_registry_listener registry_listener = {
        .global = test_wl_get_registry_global,
        .global_remove = test_wl_get_registry_global_remove,
    };
    wl_registry_add_listener(registry, &registry_listener, NULL);
    wl_display_roundtrip(display);
    wl_display_disconnect(display);
}

static char to_hex(uint32_t val) {
    if (val < 10) return '0' + val;
    return 'a' + val - 10;
}
static int create_shm_file(int size) {
    // create shm file name
    char file_name[] = "/wl_shm-XXXXXXXX";
    uint32_t random_bytes = randombytes_random();
    for (int i = 0; i < sizeof("XXXXXXXX") - 1; ++i) {
        file_name[sizeof(file_name) - i - 2] =
            to_hex((random_bytes & (0xF << (i * 4))) >> (i * 4));
    }
    // create shm file
    int fd = shm_open(file_name, O_RDWR | O_CREAT | O_EXCL, 0600);
    int ret;
    if (fd < 0) {
        fprintf(stderr, "Error creating shm %s: %d\n", file_name, errno);
        exception();
    } else {
        ret = shm_unlink(file_name);
        if (ret != 0) {
            fprintf(stderr, "Error pre-emptively unlinking shm %s: %d\n", file_name, errno);
            exception();
        }
    }
    printf("Created shm file: %s\n", file_name);
    int count = 0;
    do {
        ret = ftruncate(fd, size);
        ++count;
    } while (ret < 0 && errno == EINTR
            && count < 100); // prevent infinite loop
    if (ret < 0) {
        fprintf(stderr, "Failed to resize shm file %s: %d\n", file_name, errno);
        close(fd);
        exception();
    }
    return fd;
}
typedef struct test_wl_compositor_state {
    struct wl_compositor* compositor;
    uint32_t compositor_name;
    struct wl_surface* surface;
    struct wl_shm* shm;
    uint32_t shm_name;
    struct wl_shm_pool* shm_pool;
    int shm_fd;
    struct wl_buffer* display_buffer;
    struct xdg_wm_base* xdg_wm_base;
    struct xdg_surface* xdg_surface;
    struct xdg_toplevel* xdg_toplevel;
    bool finished_init;
} test_wl_compositor_state;
static void test_wl_compositor_post_configure(void *data,
			  struct xdg_surface *xdg_surface,
			  uint32_t serial) {
    test_wl_compositor_state* state = (test_wl_compositor_state*) data;
    // since we configure the listener from "post_init", we know that we have already finished
    // initializing the surface and display buffer. so we can just immediately ack and commit
    xdg_surface_ack_configure(xdg_surface, serial);
    // it's possible we don't need this for this test because we already committed earlier
    wl_surface_commit(state->surface);

    state->finished_init = true;
    printf("XDG configure called\n");
}
static const struct xdg_surface_listener test_wl_compositor_post_xdg_surface_listener = {
    .configure = &test_wl_compositor_post_configure,
};
static void test_wl_compositor_post_init(test_wl_compositor_state* state) {
    // check if all required pieces are initialized
    if (state->surface == NULL || state->shm == NULL || state->xdg_wm_base == NULL) {
        return;
    }
    state->xdg_surface = xdg_wm_base_get_xdg_surface(state->xdg_wm_base, state->surface);
    // it's possible we could get a bit faster by attaching the display buffer as soon as surface + shm are both ready
    state->xdg_toplevel = xdg_surface_get_toplevel(state->xdg_surface);
    xdg_surface_add_listener(state->xdg_surface, &test_wl_compositor_post_xdg_surface_listener, (void*) state);

    // now try making the surface buffer
    const int width = 1920, height = 1080;
    const int stride = width * 4;
    const int shm_pool_size = height * stride * 2;
    int fd = create_shm_file(shm_pool_size);
    state->shm_fd = fd;
    uint8_t *pool_data = mmap(NULL, shm_pool_size,
        PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (pool_data == MAP_FAILED) {
        fprintf(stderr, "Failed to map shared memory page: %d", errno);
        exception();
    }
    state->shm_pool = wl_shm_create_pool(state->shm, fd, shm_pool_size);
    if (state->shm_pool == NULL) {
        exception_msg("Failed to create Wayland shared memory pool");
    }
    int index = 0;
    int offset = height * stride * index;
    struct wl_buffer *buffer = wl_shm_pool_create_buffer(state->shm_pool, offset,
        width, height, stride, WL_SHM_FORMAT_XRGB8888);
    if (buffer == NULL) {
        exception_msg("Failed to create Wayland display buffer");
    }
    uint32_t *pixels = (uint32_t *)&pool_data[offset];
    memset(pixels, UINT32_MAX, width * height * 4);
    state->display_buffer = buffer;
    // checker board pattern
    // uint32_t *pixels = (uint32_t *)&pool_data[offset];
    // for (int y = 0; y < height; ++y) {
    //   for (int x = 0; x < width; ++x) {
    //     if ((x + y / 8 * 8) % 16 < 8) {
    //       pixels[y * width + x] = 0xFF666666;
    //     } else {
    //       pixels[y * width + x] = 0xFFEEEEEE;
    //     }
    //   }
    // }
    wl_surface_attach(state->surface, state->display_buffer, 0, 0);
    wl_surface_damage(state->surface, 0, 0, UINT32_MAX, UINT32_MAX);
    wl_surface_commit(state->surface);
    printf("Finished init!\n");
}
static void test_wl_compositor_global(void *data,
           struct wl_registry *wl_registry,
           uint32_t name,
           const char *interface,
           uint32_t version) {
    test_wl_compositor_state* state = (test_wl_compositor_state*) data;
    if (!strncmp(interface, wl_compositor_interface.name, strlen(wl_compositor_interface.name))) {
        assert(version >= 6);
        state->compositor = wl_registry_bind(wl_registry, name, &wl_compositor_interface, 6);
        state->compositor_name = name;
        state->surface = wl_compositor_create_surface(state->compositor);
        printf("Created wl_surface: %p\n", state->surface);
        test_wl_compositor_post_init(state);
    }
    if (!strncmp(interface, wl_shm_interface.name, strlen(wl_shm_interface.name))) {
        assert(version >= 1);
        state->shm = wl_registry_bind(wl_registry, name, &wl_shm_interface, 1);
        state->shm_name = name;

        // don't make a buffer yet:
        // xdg_wm_base#9: error 3: xdg_surface must not have a buffer at creation
        test_wl_compositor_post_init(state);
    }
    if (!strncmp(interface, xdg_wm_base_interface.name, strlen(xdg_wm_base_interface.name))) {
        printf("v: %d\n", version);
        assert(version >= 2);
        state->xdg_wm_base = wl_registry_bind(wl_registry, name, &xdg_wm_base_interface, 2);
        test_wl_compositor_post_init(state);
    }
}
static void test_wl_compositor_global_remove(void *data,
              struct wl_registry *wl_registry,
              uint32_t name) {
    test_wl_compositor_state* state = (test_wl_compositor_state*) data;
    assert(name != state->compositor_name);
    assert(name != state->shm_name);
}
static void test_wl_compositor(void) {
    struct wl_display *display = wl_display_connect(NULL);
    assert(display != NULL);
    struct wl_registry *registry = wl_display_get_registry(display);
    assert(registry != NULL);
    struct wl_registry_listener registry_listener = {
        .global = test_wl_compositor_global,
        .global_remove = test_wl_compositor_global_remove,
    };
    test_wl_compositor_state state = { 0 };
    wl_registry_add_listener(registry, &registry_listener, &state);
    wl_display_roundtrip(display);
    while (!state.finished_init) {
        wl_display_roundtrip(display);
        sleep(1);
    }
    wl_display_disconnect(display);
    close(state.shm_fd); // clean up resources
}

int main() {
    init_exceptions(false);
    UNITY_BEGIN();
    RUN_TEST(test_wl_connect);
    RUN_TEST(test_wl_get_registry);
    RUN_TEST(test_wl_compositor);
    return UNITY_END();
}

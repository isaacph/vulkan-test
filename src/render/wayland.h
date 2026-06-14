#ifndef WAYLAND_H_INCLUDED
#define WAYLAND_H_INCLUDED
#if !defined(_WIN32)
#include <wayland-client-core.h>
#include <wayland-client-protocol.h>
#include <wayland-client.h>

typedef struct WindowHandle {
    struct wl_display *display;
    struct wl_surface *surface;
} WindowHandle;

#endif
#endif // WAYLAND_H_INCLUDED

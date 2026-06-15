#ifndef WAYLAND_H_INCLUDED
#define WAYLAND_H_INCLUDED
#if !defined(_WIN32)
#include <wayland-client-core.h>
#include <wayland-client-protocol.h>
#include <wayland-client.h>

struct LocalState;

typedef struct WindowHandle {
    struct LocalState *localState;
} WindowHandle;

#endif
#endif // WAYLAND_H_INCLUDED

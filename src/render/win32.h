#ifndef WIN32_H_INCLUDED
#define WIN32_H_INCLUDED
#include <stdbool.h>
#if defined(_WIN32)
#ifndef UNICODE
#define UNICODE
#endif
#ifndef _UNICODE
#define _UNICODE
#endif
#include <windows.h>

typedef struct WindowUserData {
    bool quit;
    bool shouldDraw;
    bool propagateResize;
    bool resizeQueued;
    int newWidth;
    int newHeight;
} WindowUserData;
typedef struct WindowHandle {
    HINSTANCE hInstance;
    HWND hwnd;
    WindowUserData* userData;
} WindowHandle;

#endif // _WIN32
#endif // WIN32_H_INCLUDED

#ifndef WIN32_H_INCLUDED
#define WIN32_H_INCLUDED
#if defined(_WIN32)
#ifndef UNICODE
#define UNICODE
#endif
#ifndef _UNICODE
#define _UNICODE
#endif
#include <windows.h>

typedef struct WindowHandle {
    HINSTANCE hInstance;
    HWND hwnd;
} WindowHandle;


#endif // _WIN32
#endif // WIN32_H_INCLUDED

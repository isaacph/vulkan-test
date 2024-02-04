#include "util.h"
#ifndef UNICODE
#define UNICODE
#endif
#ifndef _UNICODE
#define _UNICODE
#endif
#include <windows.h>
#include <stdio.h>
#include "render.h"
#include "backtrace.h"
#include "ccomexample.h"

// making a bunch of modes
// all modes will have the actual game in a separate process
// the original process is there to handle crashes, handle logging, and start the game
// winmain mode will start in winmain, and redirect console to a log file
// crt mode will start in _main, and redirect console to the original console:q
//

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lparam);
struct RenderContext renderContext;
bool resizeMode = false;
bool resetDrawBounds = false;
bool disableDraw = false;
bool running = true;

void draw(HWND hwnd) {
    if (resetDrawBounds) {
        RECT rect;
        if (GetClientRect(hwnd, &rect)) {
            uint32_t width = rect.right - rect.left;
            uint32_t height = rect.bottom - rect.top;
            rc_size_change(&renderContext, width, height);
        }
        resetDrawBounds = false;
    }
    rc_draw(&renderContext);
}

// int WinMain(
//         HINSTANCE hInstance,
//         HINSTANCE hPrevInstance,
//         LPSTR lpCmdLine,
//         int nCmdShow) {
int main() {
    HRESULT hr = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);
    if (FAILED(hr)) {
        printf("Error initializing COM library: %ld", hr);
        return 1;
    }

    init_exceptions(false);

    HINSTANCE hInstance = GetModuleHandle(NULL);

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

    // Create the window.
    HWND hwnd = CreateWindowEx(
        0,                              // Optional window styles.
        CLASS_NAME,                     // Window class
        L"Learn to Program Windows",    // Window text
        WS_OVERLAPPEDWINDOW,            // Window style

        // Size and position
        CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,

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
        return 0;
    }

    renderContext = rc_init_win32(hInstance, hwnd);

    ShowWindow(hwnd, SW_SHOW);

    // Run the message loop.

    MSG msg = {0};
    while (running) {
        if (!disableDraw) {
            draw(hwnd);
        } else {
        }
        while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE) != 0) {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }

    rc_cleanup(&renderContext);
    CoUninitialize();
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


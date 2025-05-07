/**
 * \file win32_window.c
 * \author Isaiah Lateer
 * 
 * Source file for the window functions.
 */

#include "platform.h"

#ifdef OPENGL_CONTEXT_WINDOWS_PLATFORM

#define CLASS_NAME TEXT("window_class")

#include "window.h"

#include <stdio.h>

#include <windows.h>

static bool quit = false;

/**
 * Handles messages sent to the window.
 * 
 * \param[in] window Native window handle.
 * \param[in] message Message.
 * \param[in] wparam Additional message information.
 * \param[in] lparam Additional message information.
 * \return Message dependent resulting value.
 */
LRESULT CALLBACK window_procedure(HWND window, UINT message, WPARAM wparam,
    LPARAM lparam) {
    LRESULT result = 0;

    switch (message) {
    case WM_CLOSE:
        quit = true;
        break;
    default:
        result = DefWindowProc(window, message, wparam, lparam);
        break;
    }
    
    return result;
}

typedef struct window_data {
    HINSTANCE instance;
    HWND window;
} window_data;

/**
 * Creates a window.
 * 
 * \return New window.
 */
window* create_window() {
    window* window = malloc(sizeof(struct window));
    window_data* data = malloc(sizeof(window_data));
    window->data = data;
    
    data->instance = GetModuleHandle(NULL);
    if (!data->instance) {
        fprintf(stderr, "[ERROR] Failed to get module handle.\n");
        return false;
    }
    
    WNDCLASSEX window_class;
    BOOL result = GetClassInfoEx(data->instance, CLASS_NAME, &window_class);
    if (!result) {
        HICON icon = LoadIcon(data->instance, IDI_APPLICATION);

        window_class.cbSize = sizeof(window_class);
        window_class.style = CS_VREDRAW | CS_HREDRAW | CS_OWNDC;
        window_class.lpfnWndProc = window_procedure;
        window_class.hInstance = data->instance;
        window_class.hIcon = icon;
        window_class.hCursor = LoadCursor(NULL, IDC_ARROW);
        window_class.hbrBackground = (HBRUSH) GetStockObject(BLACK_BRUSH);
        window_class.lpszClassName = CLASS_NAME;
        window_class.hIconSm = icon;

        ATOM atom = RegisterClassEx(&window_class);
        if (!atom) {
            fprintf(stderr, "[ERROR] Failed to register window class.\n");

            free(data);
            free(window);

            return NULL;
        }
    }

    data->window = CreateWindowEx(0, CLASS_NAME, TEXT("OpenGL Context"),
        WS_OVERLAPPEDWINDOW, 20, 20, 400, 300, NULL, NULL, data->instance,
        NULL);
    if (!data->window) {
        fprintf(stderr, "[ERROR] Failed to create window.\n");

        UnregisterClass(CLASS_NAME, data->instance);

        free(data);
        free(window);

        return false;
    }

    ShowWindow(data->window, SW_SHOW);

    printf("[INFO] Window created.\n");

    return window;
}

/**
 * Destroys a window.
 * 
 * \param[in] window Window data.
 */
void destroy_window(window* window) {
    window_data* data = window->data;

    DestroyWindow(data->window);
    UnregisterClass(CLASS_NAME, data->instance);

    free(window->data);
    free(window);

    printf("[INFO] Window destroyed.\n");
}

/**
 * Polls events sent to the window.
 * 
 * \param[in] window Window.
 * \return Whether the application should close.
 */
bool poll_events(window* window) {
    window_data* data = window->data;

    MSG message = { NULL };
    while (PeekMessage(&message, data->window, 0, 0, PM_REMOVE)) {
        TranslateMessage(&message);
        DispatchMessage(&message);
    }

    return quit;
}

#else
typedef int win32_window_c;
#endif

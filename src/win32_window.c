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
    HDC device_context;
    HGLRC rendering_context;
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

    WNDCLASSEX window_class = { 0 };
    BOOL result = GetClassInfoEx(data->instance, CLASS_NAME, &window_class);
    if (!result) {
        HICON icon = LoadIcon(data->instance, IDI_APPLICATION);

        window_class = (WNDCLASSEX) { 0 };
        window_class.cbSize = sizeof(window_class);
        window_class.style = CS_VREDRAW | CS_HREDRAW | CS_OWNDC;
        window_class.lpfnWndProc = window_procedure;
        window_class.hInstance = data->instance;
        window_class.hIcon = icon;
        window_class.hCursor = LoadCursor(NULL, IDC_ARROW);
        window_class.hbrBackground = GetStockObject(BLACK_BRUSH);
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

    data->device_context = GetDC(data->window);
    if (!data->device_context) {
        fprintf(stderr, "[ERROR] Failed to get device context.\n");

        DestroyWindow(data->window);
        UnregisterClass(CLASS_NAME, data->instance);

        free(data);
        free(window);

        return NULL;
    }

    PIXELFORMATDESCRIPTOR descriptor = {
        sizeof(descriptor),
        1,
        PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL,
        PFD_TYPE_RGBA,
        24,
        0,
        0,
        0,
        0,
        0,
        0,
        8,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        PFD_MAIN_PLANE,
        0,
        0,
        0,
        0
    };

    int format = ChoosePixelFormat(data->device_context, &descriptor);
    if (!format) {
        fprintf(stderr, "[ERROR] Failed to choose pixel format.\n");

        ReleaseDC(data->window, data->device_context);
        DestroyWindow(data->window);
        UnregisterClass(CLASS_NAME, data->instance);

        free(data);
        free(window);

        return NULL;
    }

    result = DescribePixelFormat(data->device_context, format,
        sizeof(descriptor), &descriptor);
    if (!result) {
        fprintf(stderr, "[ERROR] Failed to describe pixel format.\n");

        ReleaseDC(data->window, data->device_context);
        DestroyWindow(data->window);
        UnregisterClass(CLASS_NAME, data->instance);

        free(data);
        free(window);

        return NULL;
    }

    result = SetPixelFormat(data->device_context, format, &descriptor);
    if (!result) {
        fprintf(stderr, "[ERROR] Failed to set pixel format.\n");

        ReleaseDC(data->window, data->device_context);
        DestroyWindow(data->window);
        UnregisterClass(CLASS_NAME, data->instance);

        free(data);
        free(window);

        return NULL;
    }

    data->rendering_context = wglCreateContext(data->device_context);
    if (!data->rendering_context) {
        fprintf(stderr, "[ERROR] Failed to create rendering context.\n");

        ReleaseDC(data->window, data->device_context);
        DestroyWindow(data->window);
        UnregisterClass(CLASS_NAME, data->instance);

        free(data);
        free(window);

        return NULL;
    }

    result = wglMakeCurrent(data->device_context, data->rendering_context);
    if (!result) {
        fprintf(stderr, "[ERROR] Failed to set rendering context.\n");

        wglDeleteContext(data->rendering_context);
        ReleaseDC(data->window, data->device_context);
        DestroyWindow(data->window);
        UnregisterClass(CLASS_NAME, data->instance);

        free(data);
        free(window);

        return NULL;
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

    wglMakeCurrent(NULL, NULL);
    wglDeleteContext(data->rendering_context);
    ReleaseDC(data->window, data->device_context);
    DestroyWindow(data->window);
    UnregisterClass(CLASS_NAME, data->instance);

    free(data);
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

    MSG message = { 0 };
    while (PeekMessage(&message, data->window, 0, 0, PM_REMOVE)) {
        TranslateMessage(&message);
        DispatchMessage(&message);
    }

    return quit;
}

/**
 * Swaps buffers.
 * 
 * \param[in] window Window.
 */
void swap_buffer(window* window) {
    window_data* data = window->data;
    SwapBuffers(data->device_context);
}

#else
typedef int win32_window_c;
#endif

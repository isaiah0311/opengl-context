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

#include <GL/gl.h>
#include <GL/wglext.h>

#include "version.h"

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
static LRESULT CALLBACK window_procedure(HWND window, UINT message,
    WPARAM wparam, LPARAM lparam) {
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

/**
 * Gets the address for an OpenGL procedure.
 * 
 * \param[in] name Procedure name.
 * \return Procedure address.
 */
static PROC get_procedure(const char* name) {
    PROC procedure = wglGetProcAddress(name);
    if (procedure == (PROC) -1 || procedure == (PROC) 1 ||
        procedure == (PROC) 2 || procedure == (PROC) 3) {
        return NULL;
    }

    return procedure;
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
    HWND dummy_window = CreateWindowEx(0, TEXT("STATIC"),
        TEXT("OpenGL Context"), 0, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
        CW_USEDEFAULT, NULL, NULL, NULL, NULL);
    if (!dummy_window) {
        fprintf(stderr, "[ERROR] Failed to create dummy window.\n");
        return NULL;
    }

    HDC dummy_device_context = GetDC(dummy_window);
    if (!dummy_device_context) {
        fprintf(stderr, "[ERROR] Failed to get dummy device context.\n");

        DestroyWindow(dummy_window);

        return NULL;
    }

    PIXELFORMATDESCRIPTOR dummy_descriptor = {
        sizeof(dummy_descriptor),
        1,
        PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER,
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

    int dummy_format = ChoosePixelFormat(dummy_device_context,
        &dummy_descriptor);
    if (!dummy_format) {
        fprintf(stderr, "[ERROR] Failed to choose dummy pixel format.\n");

        ReleaseDC(dummy_window, dummy_device_context);
        DestroyWindow(dummy_window);

        return NULL;
    }

    int result = DescribePixelFormat(dummy_device_context, dummy_format,
        sizeof(dummy_descriptor), &dummy_descriptor);
    if (!result) {
        fprintf(stderr, "[ERROR] Failed to describe dummy pixel format.\n");

        ReleaseDC(dummy_window, dummy_device_context);
        DestroyWindow(dummy_window);

        return NULL;
    }

    result = SetPixelFormat(dummy_device_context, dummy_format,
        &dummy_descriptor);
    if (!result) {
        fprintf(stderr, "[ERROR] Failed to set dummy pixel format.\n");

        ReleaseDC(dummy_window, dummy_device_context);
        DestroyWindow(dummy_window);

        return NULL;
    }

    HGLRC dummy_rendering_context = wglCreateContext(dummy_device_context);
    if (!dummy_rendering_context) {
        fprintf(stderr, "[ERROR] Failed to create dummy rendering context.\n");

        ReleaseDC(dummy_window, dummy_device_context);
        DestroyWindow(dummy_window);

        return NULL;
    }

    result = wglMakeCurrent(dummy_device_context, dummy_rendering_context);
    if (!result) {
        fprintf(stderr, "[ERROR] Failed to set dummy rendering context.\n");

        wglDeleteContext(dummy_rendering_context);
        ReleaseDC(dummy_window, dummy_device_context);
        DestroyWindow(dummy_window);

        return NULL;
    }

    PFNWGLCHOOSEPIXELFORMATARBPROC wglChoosePixelFormatARB =
        (PFNWGLCHOOSEPIXELFORMATARBPROC)
        get_procedure("wglChoosePixelFormatARB");

    PFNWGLCREATECONTEXTATTRIBSARBPROC wglCreateContextAttribsARB =
        (PFNWGLCREATECONTEXTATTRIBSARBPROC)
        get_procedure("wglCreateContextAttribsARB");

    window* window = malloc(sizeof(struct window));
    window_data* data = malloc(sizeof(window_data));
    window->data = data;

    data->instance = GetModuleHandle(NULL);
    if (!data->instance) {
        fprintf(stderr, "[ERROR] Failed to get module handle.\n");

        wglMakeCurrent(NULL, NULL);
        wglDeleteContext(dummy_rendering_context);
        ReleaseDC(dummy_window, dummy_device_context);
        DestroyWindow(dummy_window);
        
        return NULL;
    }

    WNDCLASSEX window_class = { 0 };
    result = GetClassInfoEx(data->instance, CLASS_NAME, &window_class);
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

            wglMakeCurrent(NULL, NULL);
            wglDeleteContext(dummy_rendering_context);
            ReleaseDC(dummy_window, dummy_device_context);
            DestroyWindow(dummy_window);

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

        wglMakeCurrent(NULL, NULL);
        wglDeleteContext(dummy_rendering_context);
        ReleaseDC(dummy_window, dummy_device_context);
        DestroyWindow(dummy_window);

        free(data);
        free(window);

        return NULL;
    }

    data->device_context = GetDC(data->window);
    if (!data->device_context) {
        fprintf(stderr, "[ERROR] Failed to get device context.\n");

        DestroyWindow(data->window);
        UnregisterClass(CLASS_NAME, data->instance);

        wglMakeCurrent(NULL, NULL);
        wglDeleteContext(dummy_rendering_context);
        ReleaseDC(dummy_window, dummy_device_context);
        DestroyWindow(dummy_window);

        free(data);
        free(window);

        return NULL;
    }

    if (!wglChoosePixelFormatARB || !wglCreateContextAttribsARB) {
        wglMakeCurrent(NULL, NULL);
        wglDeleteContext(dummy_rendering_context);
        ReleaseDC(dummy_window, dummy_device_context);
        DestroyWindow(dummy_window);

        PIXELFORMATDESCRIPTOR descriptor = {
            sizeof(descriptor),
            1,
            PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER,
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
    } else {
        const int pixel_attributes[] = {
            WGL_DRAW_TO_WINDOW_ARB, GL_TRUE,
            WGL_SUPPORT_OPENGL_ARB, GL_TRUE,
            WGL_DOUBLE_BUFFER_ARB, GL_TRUE,
            WGL_PIXEL_TYPE_ARB, WGL_TYPE_RGBA_ARB,
            WGL_COLOR_BITS_ARB, 24,
            WGL_ALPHA_BITS_ARB, 8,
            0
        };
    
        int format;
        UINT format_count;
    
        result = wglChoosePixelFormatARB(data->device_context,
            pixel_attributes, NULL, 1, &format, &format_count);
        if (!result || !format_count) {
            fprintf(stderr, "[ERROR] Failed to choose pixel format.\n");
            
            ReleaseDC(data->window, data->device_context);
            DestroyWindow(data->window);
            UnregisterClass(CLASS_NAME, data->instance);

            wglMakeCurrent(NULL, NULL);
            wglDeleteContext(dummy_rendering_context);
            ReleaseDC(dummy_window, dummy_device_context);
            DestroyWindow(dummy_window);
    
            free(data);
            free(window);
    
            return NULL;
        }
    
        PIXELFORMATDESCRIPTOR descriptor = {
            sizeof(descriptor),
            0,
            0,
            0,
            0,
            0,
            0,
            0,
            0,
            0,
            0,
            0,
            0,
            0,
            0,
            0,
            0,
            0,
            0,
            0,
            0,
            0,
            0,
            0,
            0,
            0
        };
    
        result = DescribePixelFormat(data->device_context, format,
            sizeof(descriptor), &descriptor);
        if (!result) {
            fprintf(stderr, "[ERROR] Failed to describe pixel format.\n");
    
            ReleaseDC(data->window, data->device_context);
            DestroyWindow(data->window);
            UnregisterClass(CLASS_NAME, data->instance);

            wglMakeCurrent(NULL, NULL);
            wglDeleteContext(dummy_rendering_context);
            ReleaseDC(dummy_window, dummy_device_context);
            DestroyWindow(dummy_window);
    
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

            wglMakeCurrent(NULL, NULL);
            wglDeleteContext(dummy_rendering_context);
            ReleaseDC(dummy_window, dummy_device_context);
            DestroyWindow(dummy_window);
    
            free(data);
            free(window);
    
            return NULL;
        }

        const version versions[] = {
            { 4, 6 },
            { 4, 5 },
            { 4, 4 },
            { 4, 3 },
            { 4, 2 },
            { 4, 1 },
            { 4, 0 },
            { 3, 3 },
            { 3, 2 },
            { 3, 1 },
            { 3, 0 },
            { 2, 1 },
            { 2, 0 },
            { 1, 5 },
            { 1, 4 },
            { 1, 3 },
            { 1, 2 },
            { 1, 1 },
            { 1, 0 }
        };

        const int version_count = sizeof(versions) / sizeof(version);

        for (int i = 0; i < version_count; ++i) {
            const int context_attributes[] = {
                WGL_CONTEXT_MAJOR_VERSION_ARB, versions[i].major,
                WGL_CONTEXT_MINOR_VERSION_ARB, versions[i].minor,
                WGL_CONTEXT_PROFILE_MASK_ARB, WGL_CONTEXT_CORE_PROFILE_BIT_ARB,
                0
            };
    
            data->rendering_context =
                wglCreateContextAttribsARB(data->device_context, NULL,
                context_attributes);
            if (data->rendering_context) {
                break;
            }
        }

        wglMakeCurrent(NULL, NULL);
        wglDeleteContext(dummy_rendering_context);
        ReleaseDC(dummy_window, dummy_device_context);
        DestroyWindow(dummy_window);

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

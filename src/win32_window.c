/**
 * \file win32_window.c
 * \author Isaiah Lateer
 * 
 * Source file for the window functions.
 */

#include "platform.h"

#ifdef OPENGL_CONTEXT_WINDOWS_PLATFORM

#include "window.h"

#include <stdio.h>

#include <windows.h>

#include <gl/GL.h>
#include <GL/glext.h>
#include <GL/wglext.h>

#include "version.h"

#define CLASS_NAME TEXT("window_class")

typedef struct window {
    HINSTANCE instance;
    HWND window;
    HDC device_context;
    HGLRC rendering_context;
    bool quit;
} window;

/**
 * Handles messages sent to the window.
 * 
 * \param[in] window Window.
 * \param[in] message Message.
 * \param[in] wparam Additional message information.
 * \param[in] lparam Additional message information.
 * \return Message dependent resulting value.
 */
static LRESULT CALLBACK window_procedure(HWND window, UINT message,
    WPARAM wparam, LPARAM lparam) {
    LRESULT result = 0;

    LONG_PTR user_data = GetWindowLongPtr(window, GWLP_USERDATA);
    bool* quit = (bool*) user_data;

    switch (message) {
    case WM_CLOSE:
        *quit = true;
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

/**
 * Creates a window.
 * 
 * \param[in] title Window title.
 * \param[in] width Window width.
 * \param[in] height Window height.
 * \return New window.
 */
window* create_window(const char* title, unsigned width, unsigned height) {
#if defined(UNICODE) || defined(_UNICODE)
    const size_t dummy_char_count = strlen(title) + 1;
    wchar_t* dummy_wtitle = malloc(dummy_char_count * sizeof(wchar_t));
    mbstowcs(dummy_wtitle, title, dummy_char_count);
#endif

    HWND dummy_window = CreateWindowEx(0, TEXT("STATIC"),
#if defined(UNICODE) || defined(_UNICODE)
        dummy_wtitle,
#else
        dummy_title,
#endif
        0, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, NULL,
        NULL, NULL, NULL);

#if defined(UNICODE) || defined(_UNICODE)
    free(dummy_wtitle);
#endif

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
        sizeof(PIXELFORMATDESCRIPTOR),
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
        sizeof(PIXELFORMATDESCRIPTOR), &dummy_descriptor);
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
    memset(window, 0, sizeof(struct window));

    window->instance = GetModuleHandle(NULL);
    if (!window->instance) {
        fprintf(stderr, "[ERROR] Failed to get module handle.\n");

        wglMakeCurrent(NULL, NULL);
        wglDeleteContext(dummy_rendering_context);
        ReleaseDC(dummy_window, dummy_device_context);
        DestroyWindow(dummy_window);

        free(window);
        
        return NULL;
    }

    WNDCLASSEX window_class = { 0 };
    result = GetClassInfoEx(window->instance, CLASS_NAME, &window_class);
    if (!result) {
        window_class.cbSize = sizeof(WNDCLASSEX);
        window_class.style = CS_VREDRAW | CS_HREDRAW | CS_OWNDC;
        window_class.lpfnWndProc = window_procedure;
        window_class.cbClsExtra = 0;
        window_class.cbWndExtra = 0;
        window_class.hInstance = window->instance;
        window_class.hIcon = LoadIcon(window->instance, IDI_APPLICATION);
        window_class.hCursor = LoadCursor(NULL, IDC_ARROW);
        window_class.hbrBackground = GetStockObject(BLACK_BRUSH);
        window_class.lpszMenuName = NULL;
        window_class.lpszClassName = CLASS_NAME;
        window_class.hIconSm = LoadIcon(window->instance, IDI_APPLICATION);

        ATOM atom = RegisterClassEx(&window_class);
        if (!atom) {
            fprintf(stderr, "[ERROR] Failed to register window class.\n");

            wglMakeCurrent(NULL, NULL);
            wglDeleteContext(dummy_rendering_context);
            ReleaseDC(dummy_window, dummy_device_context);
            DestroyWindow(dummy_window);

            free(window);

            return NULL;
        }
    }

#if defined(UNICODE) || defined(_UNICODE)
    const size_t char_count = strlen(title) + 1;
    wchar_t* wtitle = malloc(char_count * sizeof(wchar_t));
    mbstowcs(wtitle, title, char_count);
#endif

    window->window = CreateWindowEx(0, CLASS_NAME,
#if defined(UNICODE) || defined(_UNICODE)
        wtitle,
#else
        title,
#endif
        WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT, width, height, NULL,
        NULL, window->instance, NULL);

#if defined(UNICODE) || defined(_UNICODE)
    free(wtitle);
#endif

    if (!window->window) {
        fprintf(stderr, "[ERROR] Failed to create window.\n");

        UnregisterClass(CLASS_NAME, window->instance);

        wglMakeCurrent(NULL, NULL);
        wglDeleteContext(dummy_rendering_context);
        ReleaseDC(dummy_window, dummy_device_context);
        DestroyWindow(dummy_window);

        free(window);

        return NULL;
    }

    window->device_context = GetDC(window->window);
    if (!window->device_context) {
        fprintf(stderr, "[ERROR] Failed to get device context.\n");

        DestroyWindow(window->window);
        UnregisterClass(CLASS_NAME, window->instance);

        wglMakeCurrent(NULL, NULL);
        wglDeleteContext(dummy_rendering_context);
        ReleaseDC(dummy_window, dummy_device_context);
        DestroyWindow(dummy_window);

        free(window);

        return NULL;
    }

    if (!wglChoosePixelFormatARB || !wglCreateContextAttribsARB) {
        wglMakeCurrent(NULL, NULL);
        wglDeleteContext(dummy_rendering_context);
        ReleaseDC(dummy_window, dummy_device_context);
        DestroyWindow(dummy_window);

        PIXELFORMATDESCRIPTOR descriptor = {
            sizeof(PIXELFORMATDESCRIPTOR),
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

        int format = ChoosePixelFormat(window->device_context, &descriptor);
        if (!format) {
            fprintf(stderr, "[ERROR] Failed to choose pixel format.\n");

            ReleaseDC(window->window, window->device_context);
            DestroyWindow(window->window);
            UnregisterClass(CLASS_NAME, window->instance);

            free(window);

            return NULL;
        }

        result = DescribePixelFormat(window->device_context, format,
            sizeof(PIXELFORMATDESCRIPTOR), &descriptor);
        if (!result) {
            fprintf(stderr, "[ERROR] Failed to describe pixel format.\n");

            ReleaseDC(window->window, window->device_context);
            DestroyWindow(window->window);
            UnregisterClass(CLASS_NAME, window->instance);

            free(window);

            return NULL;
        }

        result = SetPixelFormat(window->device_context, format, &descriptor);
        if (!result) {
            fprintf(stderr, "[ERROR] Failed to set pixel format.\n");

            ReleaseDC(window->window, window->device_context);
            DestroyWindow(window->window);
            UnregisterClass(CLASS_NAME, window->instance);

            free(window);

            return NULL;
        }

        window->rendering_context = wglCreateContext(window->device_context);
        if (!window->rendering_context) {
            fprintf(stderr, "[ERROR] Failed to create rendering context.\n");

            ReleaseDC(window->window, window->device_context);
            DestroyWindow(window->window);
            UnregisterClass(CLASS_NAME, window->instance);

            free(window);

            return NULL;
        }

        result = wglMakeCurrent(window->device_context,
            window->rendering_context);
        if (!result) {
            fprintf(stderr, "[ERROR] Failed to set rendering context.\n");

            wglDeleteContext(window->rendering_context);
            ReleaseDC(window->window, window->device_context);
            DestroyWindow(window->window);
            UnregisterClass(CLASS_NAME, window->instance);

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
    
        result = wglChoosePixelFormatARB(window->device_context,
            pixel_attributes, NULL, 1, &format, &format_count);
        if (!result || format_count == 0) {
            fprintf(stderr, "[ERROR] Failed to choose pixel format.\n");
            
            ReleaseDC(window->window, window->device_context);
            DestroyWindow(window->window);
            UnregisterClass(CLASS_NAME, window->instance);

            wglMakeCurrent(NULL, NULL);
            wglDeleteContext(dummy_rendering_context);
            ReleaseDC(dummy_window, dummy_device_context);
            DestroyWindow(dummy_window);

            free(window);
    
            return NULL;
        }
    
        PIXELFORMATDESCRIPTOR descriptor = {
            sizeof(PIXELFORMATDESCRIPTOR),
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
    
        result = DescribePixelFormat(window->device_context, format,
            sizeof(PIXELFORMATDESCRIPTOR), &descriptor);
        if (!result) {
            fprintf(stderr, "[ERROR] Failed to describe pixel format.\n");
    
            ReleaseDC(window->window, window->device_context);
            DestroyWindow(window->window);
            UnregisterClass(CLASS_NAME, window->instance);

            wglMakeCurrent(NULL, NULL);
            wglDeleteContext(dummy_rendering_context);
            ReleaseDC(dummy_window, dummy_device_context);
            DestroyWindow(dummy_window);
    
            free(window);
    
            return NULL;
        }
    
        result = SetPixelFormat(window->device_context, format, &descriptor);
        if (!result) {
            fprintf(stderr, "[ERROR] Failed to set pixel format.\n");
    
            ReleaseDC(window->window, window->device_context);
            DestroyWindow(window->window);
            UnregisterClass(CLASS_NAME, window->instance);

            wglMakeCurrent(NULL, NULL);
            wglDeleteContext(dummy_rendering_context);
            ReleaseDC(dummy_window, dummy_device_context);
            DestroyWindow(dummy_window);
    
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
    
            window->rendering_context =
                wglCreateContextAttribsARB(window->device_context, NULL,
                context_attributes);
            if (window->rendering_context) {
                break;
            }
        }

        wglMakeCurrent(NULL, NULL);
        wglDeleteContext(dummy_rendering_context);
        ReleaseDC(dummy_window, dummy_device_context);
        DestroyWindow(dummy_window);

        if (!window->rendering_context) {
            fprintf(stderr, "[ERROR] Failed to create rendering context.\n");
            
            ReleaseDC(window->window, window->device_context);
            DestroyWindow(window->window);
            UnregisterClass(CLASS_NAME, window->instance);
    
            free(window);
    
            return NULL;
        }

        result = wglMakeCurrent(window->device_context,
            window->rendering_context);
        if (!result) {
            fprintf(stderr, "[ERROR] Failed to set rendering context.\n");
            
            wglDeleteContext(window->rendering_context);
            ReleaseDC(window->window, window->device_context);
            DestroyWindow(window->window);
            UnregisterClass(CLASS_NAME, window->instance);
    
            free(window);
    
            return NULL;
        }
    }

    SetWindowLongPtr(window->window, GWLP_USERDATA, (LONG_PTR) &window->quit);
    ShowWindow(window->window, SW_SHOW);

    printf("[INFO] Window created.\n");
    printf("[INFO] OpenGL version: %s\n", glGetString(GL_VERSION));
    printf("[INFO] OpenGL renderer: %s\n", glGetString(GL_RENDERER));
    printf("[INFO] OpenGL vendor: %s\n", glGetString(GL_VENDOR));

    return window;
}

/**
 * Destroys a window.
 * 
 * \param[in] window Window.
 */
void destroy_window(window* window) {
    wglMakeCurrent(NULL, NULL);
    wglDeleteContext(window->rendering_context);
    ReleaseDC(window->window, window->device_context);
    DestroyWindow(window->window);
    UnregisterClass(CLASS_NAME, window->instance);

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
    MSG message = { 0 };
    while (PeekMessage(&message, window->window, 0, 0, PM_REMOVE)) {
        TranslateMessage(&message);
        DispatchMessage(&message);
    }

    return window->quit;
}

/**
 * Swaps buffers.
 * 
 * \param[in] window Window.
 */
void swap_buffer(window* window) {
    SwapBuffers(window->device_context);
}

#endif

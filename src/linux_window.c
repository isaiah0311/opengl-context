/**
 * \file linux_window.c
 * \author Isaiah Lateer
 * 
 * Source file for the window functions.
 */

#include "platform.h"

#ifdef OPENGL_CONTEXT_LINUX_PLATFORM

#include "window.h"

#include <stdlib.h>
#include <stdio.h>

#include <X11/Xlib.h>

#include <GL/glx.h>

#include "version.h"

/**
 * Checks if the event belongs to the current window.
 * 
 * \param[in] display Connection to the X server.
 * \param[in] event Event.
 * \param[in] arg Window pointer.
 * \return Whether the event belongs to the current window.
 */
static int predicate(Display* display, XEvent* event, XPointer arg) {
    return event->xany.window == *(Window*) arg;
}

/**
 * Handles errors.
 *
 * \param[in] display Display.
 * \param[in] error Error event.
 * \return Error code.
 */
static int error_handler(Display* display, XErrorEvent* error) {
    return 0;
}

typedef struct window_data {
    Display* display;
    XVisualInfo* visual_info;
    Colormap colormap;
    Window window;
    Atom wm_delete_window;
    GLXContext context;
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
    
    data->display = XOpenDisplay(NULL);
    if (!data->display) {
        fprintf(stderr, "[ERROR] Failed to open display.\n");

        free(data);
        free(window);

        return NULL;
    }

    int major_version, minor_version;
    Bool result = glXQueryVersion(data->display, &major_version,
        &minor_version);
    if (!result) {
        XCloseDisplay(data->display);

        free(data);
        free(window);

        return NULL;
    }

    const int screen = DefaultScreen(data->display);

    GLXFBConfig framebuffer = { 0 };

    if (((major_version == 1) && (minor_version < 3)) || (major_version < 1)) {
        int visual_attributes[] = {
            GLX_RGBA,
            GLX_DOUBLEBUFFER,
            GLX_RED_SIZE, 8,
            GLX_GREEN_SIZE, 8,
            GLX_BLUE_SIZE, 8,
            GLX_ALPHA_SIZE, 8,
            None
        };
        
        data->visual_info =
            glXChooseVisual(data->display, screen, visual_attributes);
    } else {
        const int framebuffer_attributes[] = {
            GLX_DOUBLEBUFFER, True,
            GLX_RED_SIZE, 8,
            GLX_GREEN_SIZE, 8,
            GLX_BLUE_SIZE, 8,
            GLX_ALPHA_SIZE, 8,
            GLX_DRAWABLE_TYPE, GLX_WINDOW_BIT,
            GLX_RENDER_TYPE, GLX_RGBA_BIT,
            GLX_X_RENDERABLE, True,
            None
        };

        int framebuffer_count;
        GLXFBConfig* framebuffers = glXChooseFBConfig(data->display, screen,
            framebuffer_attributes, &framebuffer_count);
        if (!framebuffers || !framebuffer_count) {
            fprintf(stderr,
                "[ERROR] Failed to choose a framebuffer configuration.\n");

            XCloseDisplay(data->display);

            free(data);
            free(window);

            return NULL;
        }

        int best_framebuffer = 0;
        int highest_samples = 0;

        for (int i = 0; i < framebuffer_count; ++i) {
            int sample_buffers;
            result = glXGetFBConfigAttrib(data->display, framebuffers[i],
                GLX_SAMPLE_BUFFERS, &sample_buffers);
            if (result != Success) {
                fprintf(stderr, "[ERROR] Failed to get framebuffer "
                    "configuration attribute.\n");
                continue;
            }

            int samples;
            result = glXGetFBConfigAttrib(data->display, framebuffers[i],
                GLX_SAMPLES, &samples);
            if (result != Success) {
                fprintf(stderr, "[ERROR] Failed to get framebuffer "
                    "configuration attribute.\n");
                continue;
            }

            if (sample_buffers && samples > highest_samples) {
                best_framebuffer = i;
                highest_samples = samples;
            }
        }

        framebuffer = framebuffers[best_framebuffer];
        XFree(framebuffers);

        data->visual_info =
            glXGetVisualFromFBConfig(data->display, framebuffer);
    }

    if (!data->visual_info) {
        fprintf(stderr, "[ERROR] Failed to get visual information.\n");

        XCloseDisplay(data->display);

        free(data);
        free(window);

        return NULL;
    }

    const Window parent = RootWindow(data->display, screen);

    data->colormap = XCreateColormap(data->display, parent,
        data->visual_info->visual, AllocNone);

    XSetWindowAttributes window_attributes = {
        0,
        BlackPixel(data->display, screen),
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
        data->colormap,
        0
    };

    data->window = XCreateWindow(data->display, parent, 20, 20, 400, 300, 0,
        data->visual_info->depth, InputOutput, data->visual_info->visual,
        CWBackPixel | CWColormap, &window_attributes);

    result = XStoreName(data->display, data->window, "OpenGL Context");
    if (!result) {
        fprintf(stderr, "[ERROR] Failed to name window.\n");

        XDestroyWindow(data->display, data->window);
        XFreeColormap(data->display, data->colormap);
        XFree(data->visual_info);
        XCloseDisplay(data->display);

        free(data);
        free(window);

        return NULL;
    }

    data->wm_delete_window = XInternAtom(data->display, "WM_DELETE_WINDOW", 0);

    result = XSetWMProtocols(data->display, data->window,
        &data->wm_delete_window, 1);
    if (!result) {
        fprintf(stderr, "[ERROR] Failed to set WM protocol.\n");

        XDestroyWindow(data->display, data->window);
        XFreeColormap(data->display, data->colormap);
        XFree(data->visual_info);
        XCloseDisplay(data->display);

        free(data);
        free(window);

        return NULL;
    }

    result = XMapWindow(data->display, data->window);
    if (!result) {
        fprintf(stderr, "[ERROR] Failed to map window.\n");

        XDestroyWindow(data->display, data->window);
        XFreeColormap(data->display, data->colormap);
        XFree(data->visual_info);
        XCloseDisplay(data->display);

        free(data);
        free(window);

        return NULL;
    }

    PFNGLXCREATECONTEXTATTRIBSARBPROC glXCreateContextAttribsARB =
        (PFNGLXCREATECONTEXTATTRIBSARBPROC)
        glXGetProcAddress((const GLubyte*) "glXCreateContextAttribsARB");

    if (glXCreateContextAttribsARB) {
        XErrorHandler new_handler = error_handler;
        XErrorHandler old_handler = XSetErrorHandler(new_handler);

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
                GLX_CONTEXT_MAJOR_VERSION_ARB, versions[i].major,
                GLX_CONTEXT_MINOR_VERSION_ARB, versions[i].minor,
                GLX_CONTEXT_PROFILE_MASK_ARB, GLX_CONTEXT_CORE_PROFILE_BIT_ARB,
                None
            };

            data->context = glXCreateContextAttribsARB(data->display,
                framebuffer, NULL, True, context_attributes);
            if (data->context) {
                break;
            }
        }

        XSetErrorHandler(old_handler);
    } else if (((major_version == 1) && (minor_version < 3))
        || (major_version < 1)) {
        data->context = glXCreateContext(data->display, data->visual_info, NULL,
            True);
    } else {
        data->context = glXCreateNewContext(data->display, framebuffer,
            GLX_RGBA_TYPE, NULL, True);
    }

    if (!data->context) {
        fprintf(stderr, "[ERROR] Failed to create context.\n");

        XUnmapWindow(data->display, data->window);
        XDestroyWindow(data->display, data->window);
        XFreeColormap(data->display, data->colormap);
        XFree(data->visual_info);
        XCloseDisplay(data->display);

        free(data);
        free(window);

        return NULL;
    }

    result = glXMakeCurrent(data->display, data->window, data->context);
    if (!result) {
        fprintf(stderr, "[ERROR] Failed to set context.\n");

        glXDestroyContext(data->display, data->context);
        XUnmapWindow(data->display, data->window);
        XDestroyWindow(data->display, data->window);
        XFreeColormap(data->display, data->colormap);
        XFree(data->visual_info);
        XCloseDisplay(data->display);

        free(data);
        free(window);

        return NULL;
    }

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

    glXMakeCurrent(data->display, None, NULL);
    glXDestroyContext(data->display, data->context);
    XUnmapWindow(data->display, data->window);
    XDestroyWindow(data->display, data->window);
    XFreeColormap(data->display, data->colormap);
    XFree(data->visual_info);
    XCloseDisplay(data->display);

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
    bool quit = false;
    window_data* data = window->data;

    XEvent event = { 0 };
    while (XCheckIfEvent(data->display, &event, predicate,
        (XPointer) &data->window)) {
        switch (event.type) {
        case ClientMessage: {
            if (event.xclient.data.l[0] == data->wm_delete_window) {
                quit = true;
            }

            break;
        } default:
            break;
        }
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
    glXSwapBuffers(data->display, data->window);
}

#else
typedef int linux_window_c;
#endif

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

static int predicate(Display* display, XEvent* event, XPointer arg) {
    return event->xany.window == *(Window*) arg;
}

typedef struct window_data {
    Display* display;
    XVisualInfo* visual_info;
    Colormap colormap;
    Window window;
    GLXContext context;
    Atom wm_delete_window;
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

    const int screen = DefaultScreen(data->display);
    const Window parent = RootWindow(data->display, screen);

    int visual_attributes[] = {
        GLX_RGBA,
        GLX_RED_SIZE, 8,
        GLX_GREEN_SIZE, 8,
        GLX_BLUE_SIZE, 8,
        GLX_ALPHA_SIZE, 8,
        None
    };

    data->visual_info = glXChooseVisual(data->display, screen,
        visual_attributes);
    if (!data->visual_info) {
        fprintf(stderr, "[ERROR] Failed to get visual information.\n");

        XCloseDisplay(data->display);

        free(data);
        free(window);

        return NULL;
    }

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

    Bool result = XStoreName(data->display, data->window, "OpenGL Context");
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

    data->context = glXCreateContext(data->display, data->visual_info, NULL,
        True);
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

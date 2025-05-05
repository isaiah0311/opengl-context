/**
 * \file win32_window.c
 * \author Isaiah Lateer
 * 
 * Source file for the window struct and functions.
 */

#include "platform.h"

#ifdef OPENGL_CONTEXT_LINUX_PLATFORM

#include "window.h"

#include <stdlib.h>
#include <stdio.h>

#include <X11/Xlib.h>

typedef struct window_data {
    Display* display;
    Window window;
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
        return false;
    }

    const int screen = DefaultScreen(data->display);
    const Window parent = RootWindow(data->display, screen);
    const int depth = DefaultDepth(data->display, screen);
    Visual* visual = DefaultVisual(data->display, screen);

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
        0,
        0
    };

    data->window = XCreateWindow(data->display, parent, 20, 20, 400, 300, 0,
        depth, InputOutput, visual, CWBackPixel, &window_attributes);

    int result = XMapWindow(data->display, data->window);
    if (!result) {
        fprintf(stderr, "[ERROR] Failed to map window.\n");

        XDestroyWindow(data->display, data->window);
        XCloseDisplay(data->display);

        return false;
    }

    result = XStoreName(data->display, data->window, "OpenGL Context");
    if (!result) {
        fprintf(stderr, "[ERROR] Failed to name window.\n");

        XUnmapWindow(data->display, data->window);
        XDestroyWindow(data->display, data->window);
        XCloseDisplay(data->display);

        return false;
    }

    data->wm_delete_window = XInternAtom(data->display, "WM_DELETE_WINDOW", 0);

    result = XSetWMProtocols(data->display, data->window,
        &data->wm_delete_window, 1);
    if (!result) {
        fprintf(stderr, "[ERROR] Failed to set WM protocol.\n");

        XUnmapWindow(data->display, data->window);
        XDestroyWindow(data->display, data->window);
        XCloseDisplay(data->display);

        return false;
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

    XUnmapWindow(data->display, data->window);
    XDestroyWindow(data->display, data->window);
    XCloseDisplay(data->display);

    free(window->data);
    free(window);

    printf("[INFO] Window destroyed.\n");
}

/**
 * Polls events sent to all windows.
 * 
 * \param[in] window Any window.
 */
void poll_events(window* window) {
    window_data* data = window->data;

    while (XPending(data->display)) {
        XEvent event = {};
        XNextEvent(data->display, &event);
    }
}

#else
typedef int linux_window_c;
#endif

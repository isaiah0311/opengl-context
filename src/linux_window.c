/**
 * \file linux_window.c
 * \author Isaiah Lateer
 * 
 * Source file for the window functions.
 */

#include "platform.h"

#ifdef OPENGL_CONTEXT_LINUX_PLATFORM

#include "window.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <X11/Xlib.h>

#include <GL/glx.h>
#include <GL/glext.h>
#include <GL/glxext.h>

#include "version.h"

typedef struct window {
    Display* display;
    XVisualInfo* visual_info;
    Colormap colormap;
    Window window;
    Atom wm_delete_window;
    GLXContext context;
} window;

static bool error = false;

/**
 * Sets a flag when an error has occurred.
 * 
 * \param[in] display Connection to the X server.
 * \param[in] event Error event.
 * \return Result.
 */
int true_error_handler(Display *display, XErrorEvent *event) {
    error = true;
    return 0;
}

/**
 * Does nothing.
 * 
 * \param[in] display Connection to the X server.
 * \param[in] event Error event.
 * \return Result.
 */
int false_error_handler(Display *display, XErrorEvent *event) {
    return 0;
}

/**
 * Checks if an event belongs to the given window.
 * 
 * \param[in] display Connection to the X server.
 * \param[in] event Event.
 * \param[in] arg Window pointer.
 * \return Whether the event belongs to the given window.
 */
static int predicate(Display* display, XEvent* event, XPointer arg) {
    return event->xany.window == *(Window*) arg;
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
    window* window = malloc(sizeof(struct window));
    memset(window, 0, sizeof(struct window));

    XErrorHandler prev_error_handler = XSetErrorHandler(true_error_handler);
    
    window->display = XOpenDisplay(NULL);
    if (!window->display || error) {
        fprintf(stderr, "[ERROR] Failed to open display.\n");

        XSetErrorHandler(prev_error_handler);

        free(window);

        return NULL;
    }

    int major_version, minor_version;
    Bool result = glXQueryVersion(window->display, &major_version,
        &minor_version);
    if (!result || error) {
        XCloseDisplay(window->display);
        XSetErrorHandler(prev_error_handler);

        free(window);

        return NULL;
    }

    const int screen = DefaultScreen(window->display);
    const Window parent = RootWindow(window->display, screen);

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
        
        window->visual_info =
            glXChooseVisual(window->display, screen, visual_attributes);
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
        GLXFBConfig* framebuffers = glXChooseFBConfig(window->display, screen,
            framebuffer_attributes, &framebuffer_count);
        if (!framebuffers || framebuffer_count == 0 || error) {
            fprintf(stderr,
                "[ERROR] Failed to choose a framebuffer configuration.\n");

            XCloseDisplay(window->display);
            XSetErrorHandler(prev_error_handler);

            free(window);

            return NULL;
        }

        int best_framebuffer = 0;
        int highest_samples = 0;

        for (int i = 0; i < framebuffer_count; ++i) {
            int sample_buffers;
            result = glXGetFBConfigAttrib(window->display, framebuffers[i],
                GLX_SAMPLE_BUFFERS, &sample_buffers);
            if (result != Success || error) {
                fprintf(stderr, "[ERROR] Failed to get framebuffer "
                    "configuration attribute.\n");
                continue;
            }

            int samples;
            result = glXGetFBConfigAttrib(window->display, framebuffers[i],
                GLX_SAMPLES, &samples);
            if (result != Success || error) {
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

        window->visual_info =
            glXGetVisualFromFBConfig(window->display, framebuffer);
    }

    if (!window->visual_info) {
        fprintf(stderr, "[ERROR] Failed to get visual information.\n");

        XCloseDisplay(window->display);
        XSetErrorHandler(prev_error_handler);

        free(window);

        return NULL;
    }

    window->colormap = XCreateColormap(window->display, parent,
        window->visual_info->visual, AllocNone);

    XSetWindowAttributes window_attributes = {
        0,
        BlackPixel(window->display, screen),
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
        window->colormap,
        0
    };

    window->window = XCreateWindow(window->display, parent, 0, 0, width, height,
        0, window->visual_info->depth, InputOutput, window->visual_info->visual,
        CWBackPixel | CWColormap, &window_attributes);
    if (error) {
        fprintf(stderr, "[ERROR] Failed to create window.\n");

        XFreeColormap(window->display, window->colormap);
        XFree(window->visual_info);
        XCloseDisplay(window->display);
        XSetErrorHandler(prev_error_handler);

        free(window);

        return NULL;
    }

    XStoreName(window->display, window->window, title);
    if (error) {
        fprintf(stderr, "[ERROR] Failed to set window title.\n");

        XDestroyWindow(window->display, window->window);
        XFreeColormap(window->display, window->colormap);
        XFree(window->visual_info);
        XCloseDisplay(window->display);
        XSetErrorHandler(prev_error_handler);

        free(window);

        return NULL;
    }

    window->wm_delete_window = XInternAtom(window->display, "WM_DELETE_WINDOW",
        False);
    if (error) {
        fprintf(stderr, "[ERROR] Failed to create atom.\n");

        XDestroyWindow(window->display, window->window);
        XFreeColormap(window->display, window->colormap);
        XFree(window->visual_info);
        XCloseDisplay(window->display);
        XSetErrorHandler(prev_error_handler);

        free(window);

        return NULL;
    }

    result = XSetWMProtocols(window->display, window->window,
        &window->wm_delete_window, 1);
    if (!result || error) {
        fprintf(stderr, "[ERROR] Failed to set window protocol.\n");

        XDestroyWindow(window->display, window->window);
        XFreeColormap(window->display, window->colormap);
        XFree(window->visual_info);
        XCloseDisplay(window->display);
        XSetErrorHandler(prev_error_handler);

        free(window);

        return NULL;
    }
    
    XMapWindow(window->display, window->window);
    if (error) {
        fprintf(stderr, "[ERROR] Failed to map window.\n");

        XDestroyWindow(window->display, window->window);
        XFreeColormap(window->display, window->colormap);
        XFree(window->visual_info);
        XCloseDisplay(window->display);
        XSetErrorHandler(prev_error_handler);

        free(window);

        return NULL;
    }

    PFNGLXCREATECONTEXTATTRIBSARBPROC glXCreateContextAttribsARB =
        (PFNGLXCREATECONTEXTATTRIBSARBPROC)
        glXGetProcAddress((const GLubyte*) "glXCreateContextAttribsARB");

    if (glXCreateContextAttribsARB) {   
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

        XSetErrorHandler(false_error_handler);

        for (int i = 0; i < version_count; ++i) {
            const int context_attributes[] = {
                GLX_CONTEXT_MAJOR_VERSION_ARB, versions[i].major,
                GLX_CONTEXT_MINOR_VERSION_ARB, versions[i].minor,
                GLX_CONTEXT_PROFILE_MASK_ARB, GLX_CONTEXT_CORE_PROFILE_BIT_ARB,
                None
            };

            window->context = glXCreateContextAttribsARB(window->display,
                framebuffer, NULL, True, context_attributes);
            if (window->context) {
                break;
            }
        }

        XSetErrorHandler(true_error_handler);
    } else if (((major_version == 1) && (minor_version < 3))
        || (major_version < 1)) {
        window->context = glXCreateContext(window->display, window->visual_info,
            NULL, True);
    } else {
        window->context = glXCreateNewContext(window->display, framebuffer,
            GLX_RGBA_TYPE, NULL, True);
    }

    if (!window->context || error) {
        fprintf(stderr, "[ERROR] Failed to create context.\n");

        XUnmapWindow(window->display, window->window);
        XDestroyWindow(window->display, window->window);
        XFreeColormap(window->display, window->colormap);
        XFree(window->visual_info);
        XCloseDisplay(window->display);
        XSetErrorHandler(prev_error_handler);

        free(window);

        return NULL;
    }

    result = glXMakeCurrent(window->display, window->window, window->context);
    if (!result || error) {
        fprintf(stderr, "[ERROR] Failed to set context.\n");

        glXDestroyContext(window->display, window->context);
        XUnmapWindow(window->display, window->window);
        XDestroyWindow(window->display, window->window);
        XFreeColormap(window->display, window->colormap);
        XFree(window->visual_info);
        XCloseDisplay(window->display);
        XSetErrorHandler(prev_error_handler);

        free(window);

        return NULL;
    }

    XSetErrorHandler(prev_error_handler);

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
    glXMakeCurrent(window->display, None, NULL);
    glXDestroyContext(window->display, window->context);
    XUnmapWindow(window->display, window->window);
    XDestroyWindow(window->display, window->window);
    XFreeColormap(window->display, window->colormap);
    XFree(window->visual_info);
    XCloseDisplay(window->display);

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

    XEvent event = { 0 };
    while (XCheckIfEvent(window->display, &event, predicate,
        (XPointer) &window->window)) {
        switch (event.type) {
        case ClientMessage: {
            if (event.xclient.data.l[0] == window->wm_delete_window) {
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
    glXSwapBuffers(window->display, window->window);
}

#elif defined(OPENGL_CONTEXT_WINDOWS_PLATFORM)
static int linux_window_c;
#endif

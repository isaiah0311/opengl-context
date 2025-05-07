/**
 * \file window.h
 * \author Isaiah Lateer
 * 
 * Header file for the window struct and functions.
 */

#ifndef OPENGL_CONTEXT_WINDOW_HEADER
#define OPENGL_CONTEXT_WINDOW_HEADER

#include <stdbool.h>

typedef struct window {
    void* data;
} window;

/**
 * Creates a window.
 * 
 * \return New window.
 */
window* create_window();

/**
 * Destroys a window.
 * 
 * \param[in] window Window data.
 */
void destroy_window(window* window);

/**
 * Polls events sent to the window.
 * 
 * \param[in] window Window.
 * \return Whether the application should close.
 */
bool poll_events(window* window);

#endif

/**
 * \file window.h
 * \author Isaiah Lateer
 * 
 * Header file for the window struct and functions.
 */

#ifndef OPENGL_CONTEXT_WINDOW_HEADER
#define OPENGL_CONTEXT_WINDOW_HEADER

#include <stdbool.h>

typedef struct window window;

/**
 * Creates a window.
 * 
 * \param[in] title Window title.
 * \param[in] width Window width.
 * \param[in] height Window height.
 * \return New window.
 */
window* create_window(const char* title, unsigned width, unsigned height);

/**
 * Destroys a window.
 * 
 * \param[in] window Window.
 */
void destroy_window(window* window);

/**
 * Polls events sent to the window.
 * 
 * \param[in] window Window.
 * \return Whether the application should close.
 */
bool poll_events(window* window);

/**
 * Swaps buffers.
 * 
 * \param[in] window Window.
 */
void swap_buffer(window* window);

#endif

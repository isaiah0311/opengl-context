/**
  * \file main.c
  * \author Isaiah Lateer
  * 
  * Creates an OpenGL context.
  */
 
#include <stdio.h>
#include <stdlib.h>

#include "window.h"

/**
 * Entry point for the program.
 * 
 * \return Exit code.
 */
int main() {
    window* window = create_window();
    if (!window) {
        return EXIT_FAILURE;
    }

    poll_events(window);
    destroy_window(window);

    return EXIT_SUCCESS;
}

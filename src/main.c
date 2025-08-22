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
    window* window = create_window("OpenGL Context", 400, 300);
    if (!window) {
        return EXIT_FAILURE;
    }

    bool quit = false;
    while (!quit) {
        quit = poll_events(window);
        swap_buffer(window);
    }
    
    destroy_window(window);

    return EXIT_SUCCESS;
}

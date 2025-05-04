/**
  * \file main.c
  * \author Isaiah Lateer
  * 
  * Creates an OpenGL context.
  */
 
#include <stdio.h>
#include <stdlib.h>

#include "platform.h"

/**
 * Entry point for the program.
 * 
 * \return Exit code.
 */
int main() {
    printf("OpenGL"
#ifdef OPENGL_CONTEXT_WINDOWS_PLATFORM     
    " (Windows)"
#elif defined OPENGL_CONTEXT_LINUX_PLATFORM
    " (Linux)"
#endif
    "\n");

    return EXIT_SUCCESS;
}

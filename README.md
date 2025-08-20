# Creating an OpenGL Context

Demonstrates how to create an OpenGL context using the Win32 API and X11 API.

## Structure

.vscode - Settings and configuration files used by VSCode\
bin - Executables\
include - Third party headers\
obj - Intermediate directory\
src - Source files

## Dependencies

This project requires several OpenGL API and extension headers. The basic API
headers gl.h, wgl.h, and glx.h should be supplied by the OS or graphics drivers.
Extension headers can be found in the Khronos OpenGL Registry. Extension headers
will be in an include subfolder of the project directory. Inside the include
folder should be two subfolders, GL and KHR. The headers glext.h, wglext.h, and
glxext.h should be placed in GL. The header khrplatform.h should be placed in
KHR.

These files are not included in the repository, as they are not owned or
managed by us.

Khronos OpenGL Registry: https://registry.khronos.org/OpenGL/index_gl.php

## Building

Pressing ctrl + shift + b while in VSCode will pull up the build tasks. From
here there are two options, build and clean. Build will compile the code and
place the executable in the bin folder. Clean will remove the bin and obj
directories. In order to build, you must have MSVC installed if you are on
Windows and g++ installed if you are on Linux.

## Running

If you are trying to run from VSCode using the debugger, press F5. This requires
you to have Visual Studio if you are debugging from Windows or GDB if you are
debugging from Linux.

If you are running the program outside of VSCode, or not using a debugger, you
can run it from the command line. There are no command line options, simply call
the executable.

## Authors

Isaiah Lateer

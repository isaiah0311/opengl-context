/**
 * \file platform.h
 * \author Isaiah Lateer
 * 
 * Contains definitions for determining platform.
 */

#ifndef OPENGL_CONTEXT_PLATFORM_HEADER
#define OPENGL_CONTEXT_PLATFORM_HEADER

#ifdef _WIN32
#define OPENGL_CONTEXT_WINDOWS_PLATFORM
#elif defined __linux__ && !defined __ANDROID__
#define OPENGL_CONTEXT_LINUX_PLATFORM
#else
#error "[ERROR] Platform not supported."
#endif

#endif

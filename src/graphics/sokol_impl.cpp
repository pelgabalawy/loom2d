// Single translation unit that compiles the sokol_gfx implementation.
// Every other file includes sokol_gfx.h WITHOUT SOKOL_IMPL.
//
// Backend selection: desktop uses OpenGL core (GL 4.1, works Win/Linux/Mac),
// mobile uses OpenGL ES 3. Metal/D3D11 are a future perf-only swap.

#if defined(__ANDROID__) || (defined(__APPLE__) && defined(TARGET_OS_IPHONE) && TARGET_OS_IPHONE)
    #define SOKOL_GLES3
#else
    #define SOKOL_GLCORE
#endif

#define SOKOL_IMPL
#include "sokol_gfx.h"
#include "sokol_log.h"

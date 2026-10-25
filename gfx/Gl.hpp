#pragma once
// ===========================================================================
// One place that pulls in OpenGL, the window library and the maths library,
// in the right order. Every other graphics file includes THIS, never the
// libraries directly.
//
//   epoxy  : finds the OpenGL functions in your graphics driver at runtime.
//            (OpenGL functions aren't ordinary library functions: the driver
//             hands them out when asked. A "loader" does the asking.)
//   GLFW   : opens a window, creates the OpenGL context, reads mouse/keyboard.
//   glm    : vectors and 4x4 matrices for the camera, matching GLSL's types.
// ===========================================================================
#include <epoxy/gl.h>          // must come BEFORE GLFW
#define GLFW_INCLUDE_NONE      // stop GLFW including its own OpenGL header
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <cstdio>

// ---------------------------------------------------------------------------
// THE MOST USEFUL 15 LINES IN THE GRAPHICS CODE.
// OpenGL's default reaction to a mistake is silence: nothing draws and nothing
// is printed. With a debug context, the driver calls this function with a
// description of exactly what went wrong.
// ---------------------------------------------------------------------------
inline void GLAPIENTRY glDebugOutput(GLenum, GLenum type, GLuint id, GLenum severity,
                                     GLsizei, const GLchar* msg, const void*)
{
    if (severity == GL_DEBUG_SEVERITY_NOTIFICATION) return;   // skip chatter
    std::fprintf(stderr, "[GL] id=%u type=0x%x severity=0x%x: %s\n", id, type, severity, msg);
}

inline void enableGlDebugOutput()
{
    GLint flags = 0;
    glGetIntegerv(GL_CONTEXT_FLAGS, &flags);
    if (!(flags & GL_CONTEXT_FLAG_DEBUG_BIT)) {
        std::fprintf(stderr, "note: no debug context, GL errors will be silent\n");
        return;
    }
    glEnable(GL_DEBUG_OUTPUT);
    glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);   // report on the line that caused it
    glDebugMessageCallback(glDebugOutput, nullptr);
}
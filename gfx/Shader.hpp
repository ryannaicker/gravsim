#pragma once
#include "gfx/Gl.hpp"
#include <cstdio>

// ===========================================================================
// A SHADER is a small program that runs on the GPU, written in GLSL (a C-like
// language). We use two kinds:
//   VERTEX shader   : runs once per point. Decides WHERE it lands on screen.
//   FRAGMENT shader : runs once per pixel covered. Decides its COLOUR.
// They are compiled by the graphics driver while the program runs, so a
// typo in GLSL is a RUNTIME error. Hence the error printing below: without
// it a broken shader just draws nothing.
// ===========================================================================
class Shader {
public:
    bool build(const char* vertexSrc, const char* fragmentSrc) {
        const GLuint vs = compile(GL_VERTEX_SHADER,   vertexSrc,   "vertex");
        const GLuint fs = compile(GL_FRAGMENT_SHADER, fragmentSrc, "fragment");
        if (!vs || !fs) return false;

        id_ = glCreateProgram();
        glAttachShader(id_, vs);
        glAttachShader(id_, fs);
        glLinkProgram(id_);                     // join them into one program
        glDeleteShader(vs);                     // the program keeps its own copy
        glDeleteShader(fs);

        GLint ok = 0;
        glGetProgramiv(id_, GL_LINK_STATUS, &ok);
        if (!ok) {
            char log[2048];
            glGetProgramInfoLog(id_, sizeof log, nullptr, log);
            std::fprintf(stderr, "SHADER LINK FAILED:\n%s\n", log);
            return false;
        }
        return true;
    }

    void use() const { glUseProgram(id_); }

    // A "uniform" is a value that is the same for every point in one draw:
    // e.g. the camera matrix. This looks up where the GPU keeps it.
    GLint loc(const char* name) const { return glGetUniformLocation(id_, name); }

    ~Shader() { if (id_) glDeleteProgram(id_); }

private:
    static GLuint compile(GLenum type, const char* src, const char* what) {
        const GLuint s = glCreateShader(type);
        glShaderSource(s, 1, &src, nullptr);
        glCompileShader(s);
        GLint ok = 0;
        glGetShaderiv(s, GL_COMPILE_STATUS, &ok);
        if (!ok) {
            char log[2048];
            glGetShaderInfoLog(s, sizeof log, nullptr, log);
            std::fprintf(stderr, "%s SHADER COMPILE FAILED:\n%s\n", what, log);
            glDeleteShader(s);
            return 0;
        }
        return s;
    }
    GLuint id_ = 0;
};
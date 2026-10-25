#pragma once
#include "gfx/Gl.hpp"
#include "gfx/Shader.hpp"
#include <vector>
#include <cstddef>

// ===========================================================================
// TRAIL RENDERER: draws each body's recent path as a line that fades out
// with age. All bodies share ONE buffer; we remember where each body's
// stretch starts and how long it is, then draw each stretch separately as a
// LINE_STRIP (a line through every point in order).
// ===========================================================================
struct TrailVertex {
    float x, y, z;
    float r, g, b, a;      // a = opacity: 0 for the oldest point, ~1 for the newest
};

class TrailRenderer {
public:
    bool init() {
        if (!shader_.build(VS, FS)) return false;
        glGenVertexArrays(1, &vao_);
        glGenBuffers(1, &vbo_);
        glBindVertexArray(vao_);
        glBindBuffer(GL_ARRAY_BUFFER, vbo_);
        const GLsizei stride = sizeof(TrailVertex);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, stride, (void*)offsetof(TrailVertex, x));
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, stride, (void*)offsetof(TrailVertex, r));
        glEnableVertexAttribArray(1);
        glBindVertexArray(0);
        return true;
    }

    // verts holds every body's trail back to back; ranges[i] = {first, count}.
    void draw(const std::vector<TrailVertex>& verts,
              const std::vector<std::pair<int,int>>& ranges,
              const glm::mat4& viewProj)
    {
        if (verts.empty()) return;
        glBindBuffer(GL_ARRAY_BUFFER, vbo_);
        const GLsizeiptr bytes = verts.size() * sizeof(TrailVertex);
        if (bytes > capacity_) { glBufferData(GL_ARRAY_BUFFER, bytes, verts.data(), GL_DYNAMIC_DRAW); capacity_ = bytes; }
        else                   { glBufferSubData(GL_ARRAY_BUFFER, 0, bytes, verts.data()); }

        shader_.use();
        glUniformMatrix4fv(shader_.loc("uViewProj"), 1, GL_FALSE, glm::value_ptr(viewProj));
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);   // ordinary see-through blending

        glBindVertexArray(vao_);
        for (const auto& [first, count] : ranges)
            if (count >= 2) glDrawArrays(GL_LINE_STRIP, first, count);
        glBindVertexArray(0);
    }

    ~TrailRenderer() {
        if (vbo_) glDeleteBuffers(1, &vbo_);
        if (vao_) glDeleteVertexArrays(1, &vao_);
    }

private:
    static constexpr const char* VS = R"(#version 430 core
layout(location = 0) in vec3 aPos;
layout(location = 1) in vec4 aColor;
uniform mat4 uViewProj;
out vec4 vColor;
void main() { gl_Position = uViewProj * vec4(aPos, 1.0); vColor = aColor; })";

    static constexpr const char* FS = R"(#version 430 core
in vec4 vColor;
out vec4 FragColor;
void main() { FragColor = vColor; })";

    Shader     shader_;
    GLuint     vao_ = 0, vbo_ = 0;
    GLsizeiptr capacity_ = 0;
};
#pragma once
#include "gfx/Gl.hpp"
#include "gfx/Shader.hpp"
#include <vector>
#include <cstddef>

// ===========================================================================
// POINT RENDERER: draws each body as a soft glowing dot.
//
// How data gets to the GPU (the part everyone gets confused by):
//   VBO (vertex buffer object)  : a block of GPU memory holding the DATA.
//   VAO (vertex array object)   : the DESCRIPTION of that data -- "the first
//                                 3 floats of each point are position, the
//                                 next 3 are colour, the next 1 is size".
// You need both. A VBO with no VAO is a pile of bytes the GPU can't read.
// ===========================================================================

// One point, as the GPU sees it. float, not double: GPUs are built for
// float, and your screen can't show a 1e-16 difference anyway.
// (Interleaving pos/colour/size in one struct is normal for GPU vertices,
//  unlike our physics State, because the GPU reads all of them together.)
struct PointVertex {
    float x, y, z;
    float r, g, b;
    float size;        // diameter in pixels
};

class PointRenderer {
public:
    bool init() {
        if (!shader_.build(VS, FS)) return false;
        glGenVertexArrays(1, &vao_);
        glGenBuffers(1, &vbo_);
        glBindVertexArray(vao_);
        glBindBuffer(GL_ARRAY_BUFFER, vbo_);

        const GLsizei stride = sizeof(PointVertex);   // bytes from one point to the next
        // attribute 0 = position: 3 floats, starting at the struct's x
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, stride, (void*)offsetof(PointVertex, x));
        glEnableVertexAttribArray(0);
        // attribute 1 = colour: 3 floats, starting at r
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, stride, (void*)offsetof(PointVertex, r));
        glEnableVertexAttribArray(1);
        // attribute 2 = size: 1 float
        glVertexAttribPointer(2, 1, GL_FLOAT, GL_FALSE, stride, (void*)offsetof(PointVertex, size));
        glEnableVertexAttribArray(2);

        glBindVertexArray(0);
        return true;
    }

    void draw(const std::vector<PointVertex>& pts, const glm::mat4& viewProj) {
        if (pts.empty()) return;
        glBindBuffer(GL_ARRAY_BUFFER, vbo_);
        const GLsizeiptr bytes = pts.size() * sizeof(PointVertex);
        if (bytes > capacity_) {                        // grow only when needed
            glBufferData(GL_ARRAY_BUFFER, bytes, pts.data(), GL_DYNAMIC_DRAW);
            capacity_ = bytes;
        } else {                                        // otherwise overwrite in place
            glBufferSubData(GL_ARRAY_BUFFER, 0, bytes, pts.data());
        }

        shader_.use();
        glUniformMatrix4fv(shader_.loc("uViewProj"), 1, GL_FALSE, glm::value_ptr(viewProj));

        glEnable(GL_PROGRAM_POINT_SIZE);       // let the shader set point size
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE);     // ADDITIVE: overlapping glows add up like light

        glBindVertexArray(vao_);
        glDrawArrays(GL_POINTS, 0, (GLsizei)pts.size());
        glBindVertexArray(0);
    }

    ~PointRenderer() {
        if (vbo_) glDeleteBuffers(1, &vbo_);
        if (vao_) glDeleteVertexArrays(1, &vao_);
    }

private:
    // Runs once per point: project it onto the screen, pass colour along.
    static constexpr const char* VS = R"(#version 430 core
layout(location = 0) in vec3  aPos;
layout(location = 1) in vec3  aColor;
layout(location = 2) in float aSize;
uniform mat4 uViewProj;
out vec3 vColor;
void main() {
    gl_Position  = uViewProj * vec4(aPos, 1.0);
    gl_PointSize = aSize;
    vColor       = aColor;
})";

    // Runs once per pixel of each point. gl_PointCoord goes 0..1 across the
    // square the GPU draws; we turn that square into a soft round glow.
    static constexpr const char* FS = R"(#version 430 core
in vec3 vColor;
out vec4 FragColor;
void main() {
    vec2  d  = gl_PointCoord - vec2(0.5);
    float r2 = dot(d, d) * 4.0;              // 0 at centre, 1 at the edge
    if (r2 > 1.0) discard;                   // cut the square's corners off
    float core = exp(-r2 * 6.0);             // bright centre
    float halo = exp(-r2 * 2.0) * 0.35;      // soft surround
    float a = core + halo;
    FragColor = vec4(vColor * a + vec3(core * 0.6), a);
})";

    Shader     shader_;
    GLuint     vao_ = 0, vbo_ = 0;
    GLsizeiptr capacity_ = 0;
};
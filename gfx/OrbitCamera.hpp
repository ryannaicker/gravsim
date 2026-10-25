#pragma once
#include "gfx/Gl.hpp"
#include <algorithm>
#include <cmath>

// ===========================================================================
// ORBIT CAMERA: always looks at the origin, from a point on a sphere around it.
//   distance : radius of that sphere (zoom)
//   yaw      : how far round we've walked (left/right drag)
//   pitch    : how high above the orbital plane we are (up/down drag)
//
// The origin is the "focus" (the Sun, or whatever we're following). The app
// shifts every position so the focus sits at (0,0,0) before drawing -- see
// "camera-relative rendering" in main.cpp.
// ===========================================================================
struct OrbitCamera {
    float distance = 4.0f;
    float yaw      = 0.6f;
    float pitch    = 0.7f;     // radians above the plane; ~40 degrees
    float fovY     = glm::radians(45.0f);

    // Spherical -> Cartesian. Physics uses z as "up" (orbits lie in the xy
    // plane), so the camera's up direction is +z too.
    glm::vec3 eye() const {
        return distance * glm::vec3(std::cos(pitch) * std::cos(yaw),
                                    std::cos(pitch) * std::sin(yaw),
                                    std::sin(pitch));
    }

    // VIEW matrix: moves the world so the camera sits at the origin looking down -z.
    glm::mat4 view() const {
        return glm::lookAt(eye(), glm::vec3(0.0f), glm::vec3(0.0f, 0.0f, 1.0f));
    }

    // PROJECTION matrix: perspective, far things smaller.
    // Near/far planes scale WITH the distance. A fixed near=0.1, far=1e6
    // wastes the depth buffer's precision and makes distant things flicker.
    glm::mat4 proj(float aspect) const {
        return glm::perspective(fovY, aspect, distance * 0.001f, distance * 1000.0f);
    }

    void orbit(float dx, float dy) {
        yaw  -= dx * 0.005f;
        pitch = std::clamp(pitch + dy * 0.005f, -1.55f, 1.55f);   // stop at the poles
    }

    // Zoom by a constant FACTOR per scroll notch, not a constant distance,
    // so it feels the same whether you're at 0.01 AU or 100 AU.
    void zoom(float notches) {
        distance = std::clamp(distance * std::pow(0.88f, notches), 1e-4f, 1e5f);
    }

    // Pick a distance that fits a sphere of radius r on screen.
    void frame(float r) { distance = r / std::tan(fovY * 0.5f) * 1.1f; }
};
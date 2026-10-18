#pragma once
#include <functional>
#include <vector>
#include "core/State.hpp"

// The force model is passed IN. The integrator knows nothing about gravity.
// Any function that takes a State and fills a vector of accelerations will do.
// Later the same integrator will move photons and vehicles.
using AccelFn = std::function<void(const State&, std::vector<Vec3>&)>;

// ---------------------------------------------------------------------------
// VELOCITY VERLET  (kick - drift - kick)
// Second order, symplectic, time-reversible. One force evaluation per step.
// Energy error is BOUNDED: it oscillates in a band of width O(h^2), forever.
// Requires: fixed timestep, and forces that depend on position only.
// ---------------------------------------------------------------------------
class VelocityVerlet {
public:
    // Compute the starting acceleration once, before the first step.
    void init(const State& s, const AccelFn& accel) { accel(s, acc_); }

    void step(State& s, double h, const AccelFn& accel) {
        const std::size_t n = s.size();
        if (acc_.size() != n) accel(s, acc_);   // safety net if init() was skipped

        for (std::size_t i = 0; i < n; ++i) s.vel[i] += (0.5 * h) * acc_[i];   // KICK (half)
        for (std::size_t i = 0; i < n; ++i) s.pos[i] += h * s.vel[i];          // DRIFT (full)
        accel(s, acc_);                                                        // new forces
        for (std::size_t i = 0; i < n; ++i) s.vel[i] += (0.5 * h) * acc_[i];   // KICK (half)
    }

private:
    std::vector<Vec3> acc_;   // carried between steps: step 3's result is next step's step 1
};

// ---------------------------------------------------------------------------
// CLASSICAL RK4
// Fourth order, NOT symplectic. Four force evaluations per step.
// Energy error is SECULAR: it drifts in one direction, forever.
// Exists only as the control in the energy-drift experiment.
// ---------------------------------------------------------------------------
class RK4 {
public:
    void init(const State&, const AccelFn&) {}

    void step(State& s, double h, const AccelFn& accel) {
        const std::size_t n = s.size();
        const std::vector<Vec3> p0 = s.pos, v0 = s.vel;   // remember where we started
        std::vector<Vec3> k1a, k2a, k3a, k4a;

        // Stage 1: slope at the start
        accel(s, k1a);
        const std::vector<Vec3> k1v = v0;

        // Stage 2: slope at the midpoint, reached using stage 1's slope
        for (std::size_t i = 0; i < n; ++i) {
            s.pos[i] = p0[i] + (0.5 * h) * k1v[i];
            s.vel[i] = v0[i] + (0.5 * h) * k1a[i];
        }
        accel(s, k2a);
        const std::vector<Vec3> k2v = s.vel;

        // Stage 3: slope at the midpoint again, reached using stage 2's slope
        for (std::size_t i = 0; i < n; ++i) {
            s.pos[i] = p0[i] + (0.5 * h) * k2v[i];
            s.vel[i] = v0[i] + (0.5 * h) * k2a[i];
        }
        accel(s, k3a);
        const std::vector<Vec3> k3v = s.vel;

        // Stage 4: slope at the end, reached using stage 3's slope
        for (std::size_t i = 0; i < n; ++i) {
            s.pos[i] = p0[i] + h * k3v[i];
            s.vel[i] = v0[i] + h * k3a[i];
        }
        accel(s, k4a);
        const std::vector<Vec3> k4v = s.vel;

        // Combine: weighted average of the four slopes, weights 1,2,2,1 over 6
        const double h6 = h / 6.0;
        for (std::size_t i = 0; i < n; ++i) {
            s.pos[i] = p0[i] + h6 * (k1v[i] + 2.0*k2v[i] + 2.0*k3v[i] + k4v[i]);
            s.vel[i] = v0[i] + h6 * (k1a[i] + 2.0*k2a[i] + 2.0*k3a[i] + k4a[i]);
        }
    }
};
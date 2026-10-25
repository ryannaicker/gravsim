#pragma once
#include <string>
#include <vector>
#include <cmath>
#include "core/State.hpp"
#include "core/Units.hpp"
#include "core/Integrator.hpp"
#include "core/Diagnostics.hpp"
#include "physics/Forces.hpp"
#include "physics/OrbitalElements.hpp"
#include "scenarios/Scenarios.hpp"

// ===========================================================================
// SIMULATION -- everything the window shows, with NO graphics code in it.
//
// This file knows about physics and time. It does not know what a pixel,
// a window or OpenGL is. That is deliberate:
//   * it can be tested headless (see the SIM block in test_main.cpp)
//   * the renderer can only READ from it, never change the physics
// ===========================================================================

// One entry in the scenario menu.
struct Preset {
    std::string name;
    State (*make)();        // function that builds the starting State
    double h;               // fixed physics timestep, in time units
    int    trailEvery;      // record a trail point every this many steps
    double viewRadius;      // how much space the camera should show (AU)
    double yearsPerSecond;  // default playback speed
};

// Small helper so twoBodyCircular() (which takes arguments) fits the
// "function with no arguments" shape that Preset::make expects.
inline State makeEarth() { return scenarios::twoBodyCircular(); }

// Timesteps chosen by measuring energy error (each keeps |dE/E| small):
//   Earth   2000 steps/orbit          -> 2e-11
//   Mercury 2000 steps/orbit          -> 3e-6
//   Halley  ~476,000 steps/orbit      -> 4e-5
// Halley needs so many because a FIXED step must be small enough for its
// brief, fast swing past the Sun, and then wastes that precision for the
// 75 years it spends crawling around the outer solar system.
// trailEvery x TRAIL_LEN (2000) = steps per orbit, so each trail is one lap.
inline std::vector<Preset> presets() {
    const double TM = orbitalPeriod(0.38709893, 1.0);
    return {
        { "Earth (circular)", makeEarth,          units::YEAR / 2000.0, 1,   1.6, 0.25 },
        { "Mercury",          scenarios::mercury, TM / 2000.0,          1,   0.7, 0.06 },
        { "Halley's comet",   scenarios::halley,  1.0e-3,               240, 28.0, 5.0 },
    };
}

class Simulation {
public:
    // Start a scenario from scratch.
    void load(const Preset& p) {
        preset_ = p;
        state_  = p.make();
        accel_  = [](const State& s, std::vector<Vec3>& a) { forces::newtonian(s, a); };
        integ_  = VelocityVerlet();
        integ_.init(state_, accel_);
        d0_          = diagnose(state_);
        steps_       = 0;
        accumulator_ = 0.0;
        trails_.assign(state_.size(), Trail());
        recordTrail();
    }

    // ------------------------------------------------------------------
    // FIXED TIMESTEP WITH AN ACCUMULATOR (Glenn Fiedler, "Fix Your Timestep!")
    //
    // The window asks: "this frame, please advance by `simTime`".
    // We NEVER step by simTime directly -- that would make the physics
    // depend on the frame rate. Instead we add it to a bank account and
    // spend it in exact chunks of h. Leftover time waits for next frame.
    //
    // Result: after N steps the state is IDENTICAL, bit for bit, whether
    // those steps happened at 30 fps, 300 fps, or with no window at all.
    // ------------------------------------------------------------------
    int advance(double simTime, int maxSteps) {
        accumulator_ += simTime;
        int taken = 0;
        while (accumulator_ >= preset_.h && taken < maxSteps) {
            stepOnce();
            accumulator_ -= preset_.h;
            ++taken;
        }
        // If we hit the cap the machine can't keep up: drop the debt rather
        // than letting it grow forever (the "spiral of death").
        if (taken == maxSteps) accumulator_ = 0.0;
        return taken;
    }

    // Exactly n steps, ignoring the accumulator. Used by tests.
    void stepN(long n) { for (long k = 0; k < n; ++k) stepOnce(); }

    const State&       state()    const { return state_; }
    const Preset&      preset()   const { return preset_; }
    long               steps()    const { return steps_; }
    double             time()     const { return steps_ * preset_.h; }
    const Diagnostics& baseline() const { return d0_; }

    // ------------------------------------------------------------------
    // TRAILS, stored in a RING BUFFER per body.
    //
    // A ring buffer is a fixed-size array used as a circle: when it is
    // full, the newest point overwrites the oldest. No memory is ever
    // allocated after it fills up, and adding a point is one write.
    // ------------------------------------------------------------------
    static constexpr std::size_t TRAIL_LEN = 2000;

    // Copy body i's trail out, oldest point first.
    void trail(std::size_t i, std::vector<Vec3>& out) const {
        const Trail& t = trails_[i];
        out.clear();
        if (!t.full) {
            out.assign(t.pts.begin(), t.pts.begin() + t.head);
        } else {
            out.assign(t.pts.begin() + t.head, t.pts.end());      // oldest part
            out.insert(out.end(), t.pts.begin(), t.pts.begin() + t.head);  // newest part
        }
    }

private:
    struct Trail {
        std::vector<Vec3> pts = std::vector<Vec3>(TRAIL_LEN);
        std::size_t head = 0;      // where the NEXT point will be written
        bool full = false;
    };

    void stepOnce() {
        integ_.step(state_, preset_.h, accel_);
        ++steps_;
        if (steps_ % preset_.trailEvery == 0) recordTrail();
    }

    void recordTrail() {
        for (std::size_t i = 0; i < state_.size(); ++i) {
            Trail& t = trails_[i];
            t.pts[t.head] = state_.pos[i];
            t.head = (t.head + 1) % TRAIL_LEN;     // wrap round: that's the "ring"
            if (t.head == 0) t.full = true;
        }
    }

    Preset             preset_{};
    State              state_;
    AccelFn            accel_;
    VelocityVerlet     integ_;
    Diagnostics        d0_;
    long               steps_       = 0;
    double             accumulator_ = 0.0;
    std::vector<Trail> trails_;
};
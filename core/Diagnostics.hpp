#pragma once
#include <cmath>
#include "core/State.hpp"
#include "core/Units.hpp"

// Everything we measure to check the physics is still right.
// Each of these is conserved by an exact solution (Noether's theorem):
//   energy  <- time symmetry,  momentum <- space symmetry,
//   angular momentum <- rotational symmetry.
struct Diagnostics {
    double kinetic   = 0.0;
    double potential = 0.0;
    double total     = 0.0;
    Vec3   momentum;
    Vec3   angularMomentum;
    double virial    = 0.0;   // 2T + U, averages to 0 for a bound 1/r system
};

// `softening` MUST match the value used in the force calculation, or the
// energy will look non-conserved when the integrator is actually fine.
inline Diagnostics diagnose(const State& s, double softening = 0.0)
{
    Diagnostics d;
    const std::size_t n = s.size();
    const double eps2 = softening * softening;

    // Per-body quantities: one loop over bodies.
    for (std::size_t i = 0; i < n; ++i) {
        d.kinetic         += 0.5 * s.mass[i] * norm2(s.vel[i]);          // T = sum 1/2 m v^2
        d.momentum        += s.mass[i] * s.vel[i];                       // P = sum m v
        d.angularMomentum += s.mass[i] * cross(s.pos[i], s.vel[i]);      // L = sum m (r x v)
    }

    // Pair quantity: each unordered pair exactly once (j starts at i+1),
    // which is why there is no factor of 1/2 here.
    for (std::size_t i = 0; i < n; ++i) {
        for (std::size_t j = i + 1; j < n; ++j) {
            const double r = std::sqrt(norm2(s.pos[j] - s.pos[i]) + eps2);
            d.potential -= units::G * s.mass[i] * s.mass[j] / r;          // U = -G sum m m / r
        }
    }

    d.total  = d.kinetic + d.potential;
    d.virial = 2.0 * d.kinetic + d.potential;
    return d;
}
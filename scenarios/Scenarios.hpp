#pragma once
#include <cmath>
#include "core/State.hpp"
#include "core/Units.hpp"

namespace scenarios {

// A circular two-body orbit. Primary at the origin, secondary at distance a.
//
// Circular speed: gravity supplies exactly the centripetal acceleration,
//     G M / a^2 = v^2 / a   =>   v = sqrt(G M / a)
// (You derived this in Checkpoint 1.)
//
// For TWO bodies, M is the TOTAL mass m1 + m2, because both bodies orbit
// their common centre of mass and the relative motion feels both masses.
// For Sun + Earth this barely matters (m2 is 3e-6), but for two equal stars
// using only m1 would give the wrong orbit.
inline State twoBodyCircular(double m1 = 1.0, double m2 = 3.0e-6, double a = 1.0)
{
    State s;
    const double mu = units::G * (m1 + m2);
    const double v  = std::sqrt(mu / a);

    s.add(Vec3(0, 0, 0), Vec3(0, 0, 0), m1, "primary");
    s.add(Vec3(a, 0, 0), Vec3(0, v, 0), m2, "secondary");   // velocity perpendicular to separation

    s.toBarycentricFrame();   // yesterday's work: COM at origin and at rest
    return s;
}

} // namespace scenarios
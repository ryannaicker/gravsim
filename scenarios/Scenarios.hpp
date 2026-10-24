#pragma once
#include <cmath>
#include "core/State.hpp"
#include "core/Units.hpp"
#include "physics/OrbitalElements.hpp"

namespace scenarios {

// A circular two-body orbit. Primary at the origin, secondary at distance a.
// Circular speed: G M / a^2 = v^2 / a   =>   v = sqrt(G M / a), M = m1 + m2.
inline State twoBodyCircular(double m1 = 1.0, double m2 = 3.0e-6, double a = 1.0)
{
    State s;
    const double v = std::sqrt(units::G * (m1 + m2) / a);
    s.add(Vec3(0,0,0), Vec3(0,0,0), m1, "primary");
    s.add(Vec3(a,0,0), Vec3(0,v,0), m2, "secondary");
    s.toBarycentricFrame();
    return s;
}

// Any two-body orbit, described the astronomer's way.
inline State twoBodyFromElements(double m1, double m2, const OrbitalElements& oe)
{
    State s;
    const double mu = units::G * (m1 + m2);
    Vec3 r, v;
    elementsToState(oe, mu, r, v);           // relative position and velocity
    s.add(Vec3(0,0,0), Vec3(0,0,0), m1, "primary");
    s.add(r, v, m2, "secondary");
    s.toBarycentricFrame();
    return s;
}

// Mercury. a, e from the JPL planetary fact sheet (J2000 mean elements).
// Mass ratio Mercury/Sun = 1.66012e-7.
// Inclination set to 0 on purpose: Day 5 measures ONLY the GR precession,
// and a tilted orbit would add nothing but confusion.
// Starts at perihelion (nu = 0) so the closest point lies along +x.
inline State mercury()
{
    OrbitalElements oe;
    oe.a = 0.38709893;
    oe.e = 0.20563069;
    return twoBodyFromElements(1.0, 1.66012e-7, oe);
}

// Halley's comet. a = 17.9 AU, e = 0.967 (UP1 section 10.4.2), i = 162.3 deg
// (retrograde: it orbits the "wrong way" round the Sun).
inline State halley()
{
    OrbitalElements oe;
    oe.a = 17.9;
    oe.e = 0.967;
    oe.i = 162.3 * units::PI / 180.0;
    return twoBodyFromElements(1.0, 1e-16, oe);
}

} // namespace scenarios
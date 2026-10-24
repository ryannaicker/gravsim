#pragma once
#include <cmath>
#include "core/Vec3.hpp"
#include "core/Units.hpp"

// ===========================================================================
// ORBITAL ELEMENTS  <->  STATE VECTOR
//
// An orbit can be described two ways:
//   STATE VECTOR : position r and velocity v (6 numbers). What the simulation uses.
//   ELEMENTS     : a, e, i, Omega, omega, nu (6 numbers). What astronomers use.
// Same information, different language. This file translates both ways.
//
//   a      semi-major axis           SIZE of the ellipse
//   e      eccentricity              SHAPE (0 = circle, ->1 = very stretched)
//   i      inclination               TILT of the orbital plane
//   Omega  longitude of ascending node   which way the tilt is ROTATED
//   omega  argument of periapsis     where the closest point is, WITHIN the plane
//   nu     true anomaly              where the body is RIGHT NOW on the ellipse
//
// mu = G (m1 + m2) everywhere below.
// Sources: University Physics I ch.10; Fundamentals of Astrophysics ch.10;
//          Curtis, "Orbital Mechanics for Engineering Students", ch.4.
// ===========================================================================
struct OrbitalElements {
    double a = 0, e = 0, i = 0, Omega = 0, omega = 0, nu = 0;
};

// Keep an angle in [0, 2pi).
inline double wrap2pi(double a) {
    const double TWO_PI = 2.0 * units::PI;
    a = std::fmod(a, TWO_PI);
    return (a < 0.0) ? a + TWO_PI : a;
}

// The ECCENTRICITY VECTOR (Laplace-Runge-Lenz vector), per unit mass:
//     e_vec = (v x h)/mu - r/|r|,     h = r x v
// Points at periapsis. Length = eccentricity. CONSERVED for an exact 1/r^2 force.
inline Vec3 eccentricityVector(const Vec3& r, const Vec3& v, double mu) {
    const Vec3 h = cross(r, v);
    return cross(v, h) / mu - r / norm(r);
}

// ---------------------------------------------------------------------------
// Signed angle FROM vector a TO vector b, measured anticlockwise when viewed
// from the tip of the axis `axis` looking down. Uses atan2, never acos.
//   cos(angle) ~ dot(a,b),   sin(angle) ~ dot(axis_hat, a x b)
// atan2(sin, cos) is accurate at EVERY angle and gets the quadrant right.
// ---------------------------------------------------------------------------
inline double angleBetween(const Vec3& a, const Vec3& b, const Vec3& axis) {
    const double s = dot(cross(a, b), axis) / norm(axis);
    const double c = dot(a, b);
    return wrap2pi(std::atan2(s, c));
}

// ---------------------------------------------------------------------------
// STATE -> ELEMENTS
// ---------------------------------------------------------------------------
inline OrbitalElements stateToElements(const Vec3& r, const Vec3& v, double mu)
{
    OrbitalElements oe;
    const double rn = norm(r);

    // 1. angular momentum: perpendicular to the orbital plane
    const Vec3   h  = cross(r, v);
    const double hn = norm(h);

    // 2. node vector: z_hat x h, points at the ascending node
    const Vec3   n(-h.y, h.x, 0.0);
    const double nn = norm(n);

    // 3. eccentricity vector: points at periapsis, length e
    const Vec3 ev = eccentricityVector(r, v, mu);
    oe.e = norm(ev);

    // 4. semi-major axis from the specific energy: eps = v^2/2 - mu/r = -mu/(2a)
    const double eps = 0.5 * norm2(v) - mu / rn;
    oe.a = -mu / (2.0 * eps);

    // 5. inclination: tilt of h away from the z-axis
    oe.i = std::atan2(std::sqrt(h.x*h.x + h.y*h.y), h.z);

    const double tiny = 1e-12;
    const bool circular   = oe.e < tiny;
    const bool equatorial = nn  < tiny * hn;

    // Reference direction in the plane, from which omega is measured:
    // the node line if there is one, else the x-axis.
    const Vec3 ref = equatorial ? Vec3(1, 0, 0) : n;

    // 6. longitude of ascending node: angle from x-axis to the node line
    oe.Omega = equatorial ? 0.0 : wrap2pi(std::atan2(n.y, n.x));

    // 7. argument of periapsis: from the node line to periapsis, in the orbit plane
    //    8. true anomaly: from periapsis to the body, in the orbit plane
    if (!circular) {
        oe.omega = angleBetween(ref, ev, h);
        oe.nu    = angleBetween(ev,  r,  h);
    } else {
        // circular: periapsis is undefined, so put it at the reference direction
        oe.omega = 0.0;
        oe.nu    = angleBetween(ref, r, h);
    }
    return oe;
}

// ---------------------------------------------------------------------------
// ELEMENTS -> STATE
// Build r, v in the PERIFOCAL frame (x toward periapsis, orbit in xy-plane),
// then rotate into the inertial frame:  R = Rz(Omega) Rx(i) Rz(omega).
// ---------------------------------------------------------------------------
inline void elementsToState(const OrbitalElements& oe, double mu, Vec3& r, Vec3& v)
{
    const double p  = oe.a * (1.0 - oe.e * oe.e);          // semi-latus rectum
    const double cn = std::cos(oe.nu), sn = std::sin(oe.nu);
    const double rn = p / (1.0 + oe.e * cn);                // orbit equation

    const Vec3   rp(rn * cn, rn * sn, 0.0);
    const double k = std::sqrt(mu / p);
    const Vec3   vp(-k * sn, k * (oe.e + cn), 0.0);

    const double cO = std::cos(oe.Omega), sO = std::sin(oe.Omega);
    const double ci = std::cos(oe.i),     si = std::sin(oe.i);
    const double cw = std::cos(oe.omega), sw = std::sin(oe.omega);

    // columns of R: where the perifocal x and y axes point in inertial space
    const Vec3 P( cO*cw - sO*sw*ci,  sO*cw + cO*sw*ci,  sw*si);
    const Vec3 Q(-cO*sw - sO*cw*ci, -sO*sw + cO*cw*ci,  cw*si);

    r = rp.x * P + rp.y * Q;
    v = vp.x * P + vp.y * Q;
}

// ---------------------------------------------------------------------------
// KEPLER'S EQUATION  M = E - e sin E,  solved by Newton-Raphson.
// ---------------------------------------------------------------------------
inline double solveKepler(double M, double e, double tol = 1e-14, int maxIter = 50)
{
    M = wrap2pi(M);
    double E = (e < 0.8) ? M : units::PI;     // starting guess
    for (int k = 0; k < maxIter; ++k) {
        const double f  = E - e * std::sin(E) - M;   // how wrong we are
        const double fp = 1.0 - e * std::cos(E);     // slope of f
        const double dE = f / fp;
        E -= dE;
        if (std::fabs(dE) < tol) break;
    }
    return E;
}

// eccentric anomaly -> true anomaly
inline double trueFromEccentric(double E, double e) {
    return wrap2pi(2.0 * std::atan2(std::sqrt(1.0 + e) * std::sin(0.5 * E),
                                    std::sqrt(1.0 - e) * std::cos(0.5 * E)));
}

// mean anomaly -> true anomaly (the full chain)
inline double trueFromMean(double M, double e) {
    return trueFromEccentric(solveKepler(M, e), e);
}

// Kepler's third law
inline double orbitalPeriod(double a, double mu) {
    return 2.0 * units::PI * std::sqrt(a * a * a / mu);
}
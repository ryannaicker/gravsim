#define DAY1 1
#define DAY2 1
#define DAY3 1
#define DAY4 0
#define DAY7 0

#include "tests/check.hpp"
#include "core/Vec3.hpp"
#include "core/Units.hpp"
#include "core/State.hpp"
#include "core/Integrator.hpp"
#include "core/Diagnostics.hpp"
#include "physics/Forces.hpp"
#include "physics/OrbitalElements.hpp"
//#include "physics/CR3BP.hpp"
//#include "physics/BarnesHut.hpp"
#include "scenarios/Scenarios.hpp"
#include <cstdio>
#include <initializer_list>

static AccelFn newtonAccel(double soft = 0.0){
    return [soft](const State& s, std::vector<Vec3>& a){ forces::newtonian(s, a, soft); };
}

int main()
{
    #if DAY1
    section("Vec3");
    {
        const Vec3 x(1,0,0), y(0,1,0);
        const Vec3 c = cross(x, y);
        CHECK(c.x == 0.0 && c.y == 0.0 && c.z == 1.0);
        checkClose(norm(Vec3(3,4,0)), 5.0, 1e-15, "norm 3-4-5");
        checkClose(dot(Vec3(1,2,3), Vec3(4,5,6)), 32.0, 1e-15, "dot");
        checkClose(norm(Vec3(1,2,3) + (-Vec3(1,2,3))), 0.0, 1e-300, "a + (-a) == 0");
        checkClose(dot(cross(x,y), x), 0.0, 1e-15, "cross is orthogonal");
    }

    section("Units -- your derived constants");
    {
        checkRel(units::C, 10065.3, 1e-3, "speed of light in your units");
        checkRel(units::YEAR, 2.0*units::PI, 1e-9, "one year in your time units");
        checkRel(units::RAD_TO_ARCSEC, 206264.806, 1e-6, "rad -> arcsec");
    }

    section("State -- barcycentric frame");
    {
        State s;
        s.add(Vec3(0,0,0), Vec3(1,0,0), 2.0);
        s.add(Vec3(3,0,0), Vec3(0,2,0), 1.0);
        s.toBarycentricFrame();

        checkClose(s.pos[0].x, -1.0, 1e-14, "body 0 shifted");
        checkClose(s.pos[1].x, 2.0, 1e-14, "body 1 shifted");
        checkClose(s.vel[0].x, 1.0/3.0, 1e-14, "body 0 velocity shifted");
        checkClose(s.vel[0].y, -2.0/3.0, 1e-14, "body 0 velocity shifted y");
        checkClose(norm(s.centreOfMass()), 0.0, 1e-14, "COM at origin");
        checkClose(norm(s.momentum()), 0.0, 1e-14, "total momentum zero");
    }
    #endif

    #if DAY2
    section("Newtonian forces");
    {
        State s;
        s.add(Vec3(0,0,0), Vec3(), 1.0);
        s.add(Vec3(1,0,0), Vec3(), 1.0);
        std::vector<Vec3> a;
        forces::newtonian(s, a);
        // two unit masses, 1 apart: each feels G*m/r^2 = 1 toward the other
        checkClose(a[0].x,  1.0, 1e-15, "acc on body 0");
        checkClose(a[1].x, -1.0, 1e-15, "acc on body 1");
        // third law must hold EXACTLY, not approximately
        checkClose(a[0].x + a[1].x, 0.0, 1e-300, "third law exact");
    }

    section("Verlet: circular orbit stays circular");
    {
        State s = scenarios::twoBodyCircular(1.0, 1e-6, 1.0);
        VelocityVerlet ig; auto A = newtonAccel(); ig.init(s, A);
        const double T = units::YEAR, h = T / 2000.0;
        const double r0 = norm(s.pos[1] - s.pos[0]);
        for (int k = 0; k < 2000 * 20; ++k) ig.step(s, h, A);      // 20 orbits
        checkRel(norm(s.pos[1] - s.pos[0]), r0, 1e-6, "radius after 20 orbits");
    }

    section("Conservation over 100k steps");
    {
        State s = scenarios::twoBodyCircular(1.0, 1e-3, 1.0);
        VelocityVerlet ig; auto A = newtonAccel(); ig.init(s, A);
        const Diagnostics d0 = diagnose(s);
        const double h = units::YEAR / 1000.0;
        double worst = 0.0;
        for (int k = 0; k < 100000; ++k) {
            ig.step(s, h, A);
            if (k % 500 == 0) {
                const Diagnostics d = diagnose(s);
                worst = std::fmax(worst, std::fabs((d.total - d0.total) / d0.total));
            }
        }
        std::printf("    worst |dE/E| over 100 orbits: %.3e\n", worst);
        CHECK(worst < 1e-6);
        checkClose(norm(diagnose(s).momentum), 0.0, 1e-12, "momentum conserved");
    }

    section("RK4 drifts where Verlet does not");
    {
        auto run = [](bool useRK4) {
            State s = scenarios::twoBodyCircular(1.0, 1e-3, 1.0);
            const Diagnostics d0 = diagnose(s);
            auto A = newtonAccel();
            const double h = units::YEAR / 200.0;
            VelocityVerlet vv; RK4 rk; vv.init(s, A);
            for (int k = 0; k < 200 * 2000; ++k) useRK4 ? rk.step(s, h, A) : vv.step(s, h, A);
            return (diagnose(s).total - d0.total) / std::fabs(d0.total);
        };
        const double eVV = run(false), eRK = run(true);
        std::printf("    after 2000 orbits: Verlet %+.3e   RK4 %+.3e\n", eVV, eRK);
        CHECK(std::fabs(eVV) < std::fabs(eRK));
    }

    section("Convergence order of Verlet");
    {
        // Run exactly one orbit; a perfect integrator returns to the start.
        // The leftover distance is the error. Halve h and compare.
        auto err = [](int stepsPerOrbit) {
            State s = scenarios::twoBodyCircular(1.0, 1e-6, 1.0);
            const Vec3 p0 = s.pos[1];
            VelocityVerlet ig; auto A = newtonAccel(); ig.init(s, A);
            const double T = units::YEAR;
            for (int k = 0; k < stepsPerOrbit; ++k) ig.step(s, T / stepsPerOrbit, A);
            return norm(s.pos[1] - p0);
        };
        const double order = std::log2(err(500) / err(1000));
        std::printf("    measured order = %.3f\n", order);
        checkClose(order, 2.0, 0.25, "Verlet convergence order");
    }
#endif

#if DAY3
    section("Orbital elements round-trip");
    {
        OrbitalElements oe;
        oe.a = 2.5; oe.e = 0.42; oe.i = 0.3; oe.Omega = 1.1; oe.omega = 2.2; oe.nu = 0.7;
        Vec3 r, v;
        elementsToState(oe, 1.0, r, v);
        const OrbitalElements b = stateToElements(r, v, 1.0);
        checkRel(b.a,     oe.a,     1e-12, "a");
        checkRel(b.e,     oe.e,     1e-12, "e");
        checkRel(b.i,     oe.i,     1e-12, "i");
        checkRel(b.Omega, oe.Omega, 1e-12, "Omega");
        checkRel(b.omega, oe.omega, 1e-12, "omega");
        checkRel(b.nu,    oe.nu,    1e-12, "nu");
    }

    section("Round-trip at awkward angles (where acos would fail)");
    {
        int bad = 0;
        for (double e : {0.1, 0.5, 0.9})
            for (double nu : {0.0, units::PI, 1e-9})
                for (double w : {0.0, units::PI}) {
                    OrbitalElements oe; oe.a = 1.7; oe.e = e; oe.i = 0.4;
                    oe.Omega = 1.0; oe.omega = w; oe.nu = nu;
                    Vec3 r, v, r2, v2;
                    elementsToState(oe, 1.0, r, v);
                    elementsToState(stateToElements(r, v, 1.0), 1.0, r2, v2);
                    if (norm(r2 - r) > 1e-12 * norm(r)) ++bad;
                }
        CHECK(bad == 0);
    }

    section("Kepler's equation");
    {
        for (double e : {0.0, 0.2, 0.6, 0.9})
            for (double M : {0.1, 1.0, 3.0, 5.0}) {
                const double E = solveKepler(M, e);
                checkClose(E - e * std::sin(E), M, 1e-12, "Kepler residual");
            }
    }

    section("Kepler III measured numerically");
    {
        for (double a : {0.5, 1.0, 2.0, 5.0}) {
            State s = scenarios::twoBodyCircular(1.0, 1e-9, a);
            VelocityVerlet ig; auto A = newtonAccel(); ig.init(s, A);
            const double Tpred = orbitalPeriod(a, 1.0), h = Tpred / 4000.0;
            double tprev = 0.0, yprev = (s.pos[1] - s.pos[0]).y, Tmeas = 0.0;
            for (int k = 1; k < 20000; ++k) {
                ig.step(s, h, A);
                const double y = (s.pos[1] - s.pos[0]).y;
                if (k > 10 && yprev < 0.0 && y >= 0.0) {          // crossed the x-axis upward
                    Tmeas = tprev + h * (-yprev) / (y - yprev);     // interpolate the crossing
                    break;
                }
                tprev = k * h; yprev = y;
            }
            checkRel(Tmeas, Tpred, 1e-3, "period vs Kepler III");
        }
    }

    section("Eccentricity vector conserved (Newtonian)");
    {
        State s = scenarios::mercury();
        const double mu = s.totalMass();
        const Vec3 e0 = eccentricityVector(s.pos[1] - s.pos[0], s.vel[1] - s.vel[0], mu);
        VelocityVerlet ig; auto A = newtonAccel(); ig.init(s, A);
        const double T = orbitalPeriod(0.38709893, mu);
        for (int k = 0; k < 4000 * 10; ++k) ig.step(s, T / 4000.0, A);   // 10 orbits
        const Vec3 e1 = eccentricityVector(s.pos[1] - s.pos[0], s.vel[1] - s.vel[0], mu);
        checkRel(norm(e1), norm(e0), 1e-8, "|e| conserved");
        const double drift = std::atan2(e1.y, e1.x) - std::atan2(e0.y, e0.x);
        std::printf("    e-vector direction drift over 10 orbits: %.3e rad\n", drift);
        CHECK(std::fabs(drift) < 1e-4);
    }

    section("Halley's comet");
    {
        State s = scenarios::halley();
        const OrbitalElements oe =
            stateToElements(s.pos[1] - s.pos[0], s.vel[1] - s.vel[0], s.totalMass());
        const double T = orbitalPeriod(oe.a, s.totalMass()) / units::YEAR;
        std::printf("    period %.2f yr, perihelion %.3f AU, aphelion %.2f AU\n",
                    T, oe.a * (1 - oe.e), oe.a * (1 + oe.e));
        checkRel(T, 75.7, 1e-2, "Halley period ~75.7 yr");
    }
#endif

    return report();
    
}
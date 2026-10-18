#define DAY1 1
#define DAY2 1
#define DAY3 0
#define DAY4 0
#define DAY7 0

#include "tests/check.hpp"
#include "core/Vec3.hpp"
#include "core/Units.hpp"
#include "core/State.hpp"
#include "core/Integrator.hpp"
#include "core/Diagnostics.hpp"
#include "physics/Forces.hpp"
//#include "physics/OrbitalElements.hpp"
//#include "physics/CR3BP.hpp"
//#include "physics/BarnesHut.hpp"
#include "scenarios/Scenarios.hpp"
#include <cstdio>

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

    return report();
    
}
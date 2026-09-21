#define DAY1 1
#define DAY2 0
#define DAY3 0
#define DAY4 0
#define DAY7 0

#include "tests/check.hpp"
#include "core/Vec3.hpp"
#include "core/Units.hpp"
#include "core/State.hpp"
//#include "core/Integrator.hpp"
//#include "core/Diagnostics.hpp"
//#include "physics/Forces.hpp"
//#include "physics/OrbitalElements.hpp"
//#include "physics/CR3BP.hpp"
//#include "physics/BarnesHut.hpp"
//#include "scenarios/Scenarios.hpp"
#include <cstdio>

//static AccelFn newtonAccel(double soft = 0.0){
//    return [soft](const State& s, std::vector<Vec3>& a{ forces::newtonian(s, a, soft); };
//}

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
    return report();
}
// The plot that justifies the whole numerical approach:
// Verlet vs RK4 energy error over 3000 orbits, at two timesteps.
#include <cstdio>
#include <vector>
#include <cmath>
#include "core/State.hpp"
#include "core/Units.hpp"
#include "core/Integrator.hpp"
#include "core/Diagnostics.hpp"
#include "core/Csv.hpp"
#include "physics/Forces.hpp"
#include "scenarios/Scenarios.hpp"

// Template on the integrator: the compiler makes one copy of this function
// for VelocityVerlet and another for RK4. Same code, no runtime cost.
template <class Integ>
static void run(std::vector<double>& t, std::vector<double>& e,
                int stepsPerOrbit, int orbits)
{
    State s = scenarios::twoBodyCircular(1.0, 1e-3, 1.0);
    AccelFn A = [](const State& st, std::vector<Vec3>& a){ forces::newtonian(st, a); };
    Integ ig; ig.init(s, A);

    const double E0 = diagnose(s).total;
    const double h  = units::YEAR / stepsPerOrbit;
    const long   n  = (long)stepsPerOrbit * orbits;

    for (long k = 0; k < n; ++k) {
        ig.step(s, h, A);
        if (k % stepsPerOrbit == 0) {                          // record once per orbit
            t.push_back(k * h / units::YEAR);
            e.push_back((diagnose(s).total - E0) / std::fabs(E0));   // relative error
        }
    }
}

int main()
{
    const int orbits = 3000;
    std::vector<double> t1, v1, t2, r1, t3, v2, t4, r2;
    run<VelocityVerlet>(t1, v1, 200, orbits);
    run<RK4>           (t2, r1, 200, orbits);
    run<VelocityVerlet>(t3, v2, 400, orbits);
    run<RK4>           (t4, r2, 400, orbits);

    Csv csv("energy_drift.csv");
    csv.header({"orbit", "verlet_h", "rk4_h", "verlet_h2", "rk4_h2"});
    for (std::size_t i = 0; i < t1.size(); ++i)
        csv.row({t1[i], v1[i], r1[i], v2[i], r2[i]});

    std::printf("after %d orbits:\n", orbits);
    std::printf("  Verlet 200 steps/orbit : %+.3e\n", v1.back());
    std::printf("  Verlet 400 steps/orbit : %+.3e\n", v2.back());
    std::printf("  RK4    200 steps/orbit : %+.3e\n", r1.back());
    std::printf("  RK4    400 steps/orbit : %+.3e\n", r2.back());
    std::printf("wrote energy_drift.csv\n");
    return 0;
}
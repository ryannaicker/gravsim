#pragma once
#include <cstddef>
#include <vector>
#include <string>
#include <utility>
#include "core/Vec3.hpp"

// ===========================================================================
// STRUCTURE: GIVEN (computing).   METHODS: YOUR WORK (physics).
// ===========================================================================
//
// WHY STRUCT-OF-ARRAYS (SoA) RATHER THAN ARRAY-OF-STRUCTS (AoS):
//
//   The obvious design is  struct Body { Vec3 pos, vel; double mass; };
//   then std::vector<Body>. That is AoS and it is the wrong choice here:
//
//   1. CACHE. Your force loop reads only positions and masses. A cache line
//      is 64 bytes. With AoS, every line you pull in is roughly half velocity
//      data you are not going to touch, so you halve your effective memory
//      bandwidth for no benefit.
//
//   2. GPU. A shader storage buffer wants a contiguous array of one thing.
//      SoA uploads directly; AoS needs a repack every frame.
//
//   The cost is that "body i" is spread across three arrays and you can no
//   longer pass a single Body around. Accept it. This is the standard layout
//   in every serious N-body code.
// ===========================================================================
struct State {
    std::vector<Vec3>        pos;
    std::vector<Vec3>        vel;
    std::vector<double>      mass;
    std::vector<std::string> name;

    std::size_t size() const { return pos.size(); }

    void add(const Vec3& p, const Vec3& v, double m, std::string n = "") {
        pos.push_back(p);
        vel.push_back(v);
        mass.push_back(m);
        name.push_back(std::move(n));
    }

    double totalMass() const {
        double s = 0.0;
        for (double m : mass) s += m;
        return s;
    }

    // TODO(you): centre of mass of the system.
    //   Write down the definition first. It is a weighted average -- weighted
    //   by what, exactly, and divided by what?
    Vec3 centreOfMass() const {
        Vec3 sum;
        for (std::size_t i = 0; i < size(); ++i){
        sum+= mass[i] * pos[i];
        }
        return sum / totalMass();
    }

    // TODO(you): total linear momentum.
    Vec3 momentum() const {
        Vec3 p;
        for (std::size_t i = 0; i < size(); ++i){
            p+= mass[i] * vel[i];
        }
        
        return p;
    }

    // TODO(you): shift the whole system into the frame where the centre of
    // mass is at the origin AND at rest.
    //
    // WHY THIS MATTERS (do not skip): if you don't do this, two things break.
    // One is cosmetic -- the system drifts off screen. The other is not: your
    // angular momentum diagnostic is computed about the ORIGIN, so if the
    // origin is not the centre of mass, L acquires a spurious contribution
    // from the bulk motion and looks non-conserved when it isn't.
    //
    // Question to answer for yourself before writing it: you need to subtract
    // two different things. What are they, and why is subtracting only the
    // first one insufficient?
    void toBarycentricFrame() {
        const Vec3 rcm = centreOfMass();
        const Vec3 vcm = momentum() / totalMass();
        for (std::size_t i = 0; i < size(); ++i){
            pos[i] -= rcm;
            vel[i] -= vcm;
        }
    }
};

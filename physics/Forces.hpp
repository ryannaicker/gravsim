#pragma once
#include <vector>
#include <cmath>
#include "core/State.hpp"
#include "core/Units.hpp"

// Newtonian gravity, direct O(N^2) summation.
//   a_i = G * sum_{j != i} m_j (r_j - r_i) / (|r_j - r_i|^2 + eps^2)^(3/2)
// Source: University Physics I, ch. 10, eq. 10.1, written as a vector.
namespace forces {

inline void newtonian(const State& s, std::vector<Vec3>& acc, double softening = 0.0)
{
    const std::size_t n = s.size();
    acc.assign(n, Vec3());                 // resize to n AND zero every entry
    const double eps2 = softening * softening;

    for (std::size_t i = 0; i < n; ++i) {
        for (std::size_t j = i + 1; j < n; ++j) {      // each pair ONCE
            const Vec3   d      = s.pos[j] - s.pos[i];  // arrow from i to j
            const double r2     = norm2(d) + eps2;      // softened r^2
            const double inv_r  = 1.0 / std::sqrt(r2);  // one sqrt per pair
            const double inv_r3 = inv_r * inv_r * inv_r;

            // G d / r^3 : the part of the formula both bodies share
            const Vec3 f = (units::G * inv_r3) * d;

            // Newton's third law: equal and opposite. Each body is pulled by
            // the OTHER body's mass.
            acc[i] += s.mass[j] * f;   // i pulled toward j
            acc[j] -= s.mass[i] * f;   // j pulled toward i (opposite direction)
        }
    }
}

} // namespace forces
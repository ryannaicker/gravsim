// --- Derived constants of the unit system ------------------------------------
//
// DERIVATION OF THE TIME UNIT
//   Kepler III (Newton's form), University Physics I ch. 10:
//       T^2 = 4 pi^2 a^3 / (G M)
//   In our units a = 1 (AU) and M = 1 (M_sun). Demanding G = 1:
//       T^2 = 4 pi^2   =>   T = 2 pi
//   That orbit is one year, so  1 year = 2 pi time units, i.e.
//       1 time unit = 1 year / (2 pi) = 3.15576e7 s / 6.283185 = 5.0226e6 s
//                   ~ 58.1 days
//
//   Cross-check by dimensional analysis: [G] = L^3 M^-1 T^-2, so
//       G_mine = G_SI * M T^2 / L^3 = 1   =>   T = sqrt(L^3 / (G_SI M))
//       = sqrt(3.348e33 / 1.3275e20) = 5.022e6 s.   Agrees.
//
// SPEED OF LIGHT
//   [c] = L T^-1, so c_mine = c_SI * T / L
//   Estimate first: light crosses 1 AU in ~500 s; 5.0e6 s / 500 s ~ 1e4.
//   Computed: 2.998e8 * 5.0226e6 / 1.496e11 = 10065.1 AU per time unit.
//
// EARTH IN THESE UNITS: mass 3.0e-6, orbital radius 1, speed 1, period 2 pi.
//   (Circular speed: GM/r^2 = v^2/r  =>  v = sqrt(GM/r) = 1.)
// -----------------------------------------------------------------------------
namespace units {

inline constexpr double PI = 3.14159265358979323846;

// --- SI reference values. These are just measurements; you may keep them. ---
inline constexpr double AU_M          = 1.495978707e11;   // metres
inline constexpr double SOLAR_MASS_KG = 1.98892e30;       // kg
inline constexpr double YEAR_S        = 3.15576e7;        // Julian year, seconds
inline constexpr double C_SI          = 2.99792458e8;     // m/s
inline constexpr double G = 1.0;   // by construction: the time unit is chosen to make this 1

inline constexpr double TIME_UNIT_S = YEAR_S / (2.0 * PI);

inline constexpr double C = C_SI * TIME_UNIT_S / AU_M;

inline constexpr double YEAR = 2.0 * PI;

inline constexpr double CENTURY = 100.0 * YEAR;

// 1 rad = (180/pi) deg = (180/pi)*3600 arcsec
inline constexpr double RAD_TO_ARCSEC = 180.0 * 3600.0 / PI;
}
#pragma once
#include <cmath>
#include <iostream>
#include <string>

inline int g_failures = 0;
inline int g_checks = 0;

#define CHECK(cond)                                                            \
  do {                                                                         \
    ++g_checks;                                                                \
    if (!(cond)) {                                                             \
      std::cerr << "  FAIL  " << __FILE__ << ":" << __LINE__ << "   " << #cond \
                << "\n";                                                       \
      ++g_failures;                                                            \
    }                                                                          \
  } while (0)

inline void checkClose(double a, double b, double tol,
                       const std::string &what) {
  ++g_checks;
  if (!(std::fabs(a - b) <= tol)) {
    std::cerr << "  FAIL  " << what << ": got " << a << " want " << b << "\n";
    ++g_failures;
  }
}

inline void checkRel(double a, double b, double relTol,
                     const std::string &what) {
  ++g_checks;
  const double den = std::fabs(b) > 0 ? std::fabs(b) : 1.0;
  if (!(std::fabs(a - b) / den <= relTol)) {
    std::cerr << "  FAIL  " << what << ": got " << a << " want " << b
              << "  (rel err " << std::fabs(a - b) / den << ")\n";
    ++g_failures;
  }
}

inline void section(const std::string &s) { std::cout << "[ " << s << " ]\n"; }

inline int report() {
  std::cout << "\n"
            << (g_checks - g_failures) << "/" << g_checks << " checks passed\n";
  return g_failures == 0 ? 0 : 1;
}
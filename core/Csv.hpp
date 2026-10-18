#pragma once
#include <fstream>
#include <string>
#include <vector>
#include <iomanip>

// ===========================================================================
// GIVEN IN FULL (computing).
// ===========================================================================
// Note the setprecision(17). A double has ~15.95 significant decimal digits,
// and 17 is the minimum that guarantees a round-trip (write then read gives
// back the identical bit pattern). The default for iostreams is SIX, which
// would silently destroy exactly the 1e-14 effects you are trying to measure.
class Csv {
public:
    explicit Csv(const std::string& path) : f_(path) { f_ << std::setprecision(17); }
    void header(const std::vector<std::string>& cols) {
        for (std::size_t i = 0; i < cols.size(); ++i)
            f_ << cols[i] << (i + 1 == cols.size() ? '\n' : ',');
    }
    void row(const std::vector<double>& vals) {
        for (std::size_t i = 0; i < vals.size(); ++i)
            f_ << vals[i] << (i + 1 == vals.size() ? '\n' : ',');
    }
    bool ok() const { return f_.good(); }
private:
    std::ofstream f_;
};

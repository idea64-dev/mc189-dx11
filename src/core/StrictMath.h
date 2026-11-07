#pragma once

// java.lang.StrictMath: fdlibm algorithms with results fixed by the Java spec, independent of the C runtime.
// Only what gameplay needs; java.util.Random.nextGaussian() uses StrictMath.log and StrictMath.sqrt.

#include <cmath>

namespace mc {

class StrictMath {
public:
    static double log(double x);
    // IEEE 754 square root is exactly rounded everywhere, so the CRT result is already the Java one.
    static double sqrt(double x) { return std::sqrt(x); }
};

} // namespace mc

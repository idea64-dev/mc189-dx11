#pragma once

// java.lang.StrictMath: fdlibm algorithms with results fixed by the Java spec, independent of the C runtime.
// Only what gameplay needs: Random.nextGaussian() uses log and sqrt, celestial angles cos.

#include <cmath>

namespace mc {

class StrictMath {
public:
    static double log(double x);
    // fdlibm s_sin.c / s_cos.c with the full argument reduction (e_rem_pio2.c, k_rem_pio2.c), in
    // StrictMathTrig.cpp. HotSpot's Math.sin and Math.cos agree with them on the arguments the game uses.
    static double sin(double x);
    static double cos(double x);
    // fdlibm s_atan.c / e_atan2.c, in StrictMathAtan.cpp (Java 8's Math.atan2 delegates to StrictMath).
    static double atan(double x);
    static double atan2(double y, double x);
    // IEEE 754 square root is exactly rounded everywhere, so the CRT result is already the Java one.
    static double sqrt(double x) { return std::sqrt(x); }
};

} // namespace mc

#pragma once

// java.lang.Math.sin / cos as the Java 8 HotSpot JIT and interpreter compute them on x86-64: the x87 FSIN/FCOS
// instructions for |x| <= pi/4, fdlibm (StrictMath) beyond. They differ from StrictMath in the last bit now and
// then, which can show in float results.

#include "core/StrictMath.h"

#include <cmath>

extern "C" double x87Sin(double x);
extern "C" double x87Cos(double x);

namespace mc::java {

inline double mathSin(double x)
{
    // MacroAssembler::trigfunc: anything not below or equal to pi/4 (NaN included) takes the runtime call.
    return std::fabs(x) <= 0.7853981633974483 ? x87Sin(x) : StrictMath::sin(x);
}

inline double mathCos(double x)
{
    return std::fabs(x) <= 0.7853981633974483 ? x87Cos(x) : StrictMath::cos(x);
}

} // namespace mc::java

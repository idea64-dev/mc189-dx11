#pragma once

#include <cstdint>

namespace mc {

// Replacement for org.lwjgl.Sys (timer part) and java.lang.System.nanoTime().
// Both clocks are QueryPerformanceCounter based.
class Sys {
public:
    // Current time in ticks of getTimerResolution().
    static int64_t getTime();
    // Ticks per second of getTime().
    static int64_t getTimerResolution() { return 1000; }
    // Monotonic nanoseconds, like System.nanoTime().
    static int64_t nanoTime();
};

} // namespace mc

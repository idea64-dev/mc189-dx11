#include "platform/Sys.h"

#include <Windows.h>

namespace mc {

namespace {

int64_t performanceFrequency()
{
    static const int64_t frequency = [] {
        LARGE_INTEGER value;
        QueryPerformanceFrequency(&value);
        return value.QuadPart;
    }();
    return frequency;
}

// counter * scale / frequency without overflowing 64 bits on long uptimes.
int64_t scaledCounter(int64_t scale)
{
    LARGE_INTEGER value;
    QueryPerformanceCounter(&value);
    const int64_t frequency = performanceFrequency();
    return value.QuadPart / frequency * scale + value.QuadPart % frequency * scale / frequency;
}

} // namespace

int64_t Sys::getTime()
{
    return scaledCounter(getTimerResolution());
}

int64_t Sys::nanoTime()
{
    return scaledCounter(1'000'000'000);
}

} // namespace mc

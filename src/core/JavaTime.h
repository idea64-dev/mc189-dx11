#pragma once

// System.currentTimeMillis() and System.nanoTime().

#include <chrono>
#include <cstdint>

namespace mc::java {

inline int64_t currentTimeMillis()
{
    return std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
}

inline int64_t nanoTime()
{
    return std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::steady_clock::now().time_since_epoch()).count();
}

} // namespace mc::java

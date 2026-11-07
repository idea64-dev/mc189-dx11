#pragma once

// Original: net/minecraft/util/Timer.java

#include <cstdint>

namespace mc {

class Timer {
public:
    explicit Timer(float tps);

    // Updates all fields of the Timer using the current time.
    void updateTimer();

    // How many full ticks have turned over since the last call to updateTimer(), capped at 10.
    int elapsedTicks = 0;
    // Time elapsed since the last tick, in ticks (0.0 - 1.0), for rendering. Frozen while the game is paused.
    float renderPartialTicks = 0.0f;
    // Multiplier that makes the game run faster or slower.
    float timerSpeed = 1.0f;
    // Time elapsed since the last tick, in ticks (0.0 - 1.0).
    float elapsedPartialTicks = 0.0f;

private:
    float ticksPerSecond;
    // High-resolution clock at the last updateTimer() call, in seconds.
    double lastHRTime = 0.0;
    // System clock at the last sync, in milliseconds.
    int64_t lastSyncSysClock;
    // High-resolution clock at the last sync, in milliseconds.
    int64_t lastSyncHRClock;
    // Grows by elapsed milliseconds, reset when it passes 1000.
    int64_t counter = 0;
    // Ratio syncing the high-resolution clock to the system clock, updated once per second.
    double timeSyncAdjustment = 1.0;
};

} // namespace mc

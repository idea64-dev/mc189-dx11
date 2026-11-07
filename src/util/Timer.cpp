#include "util/Timer.h"

#include "client/Minecraft.h"
#include "platform/Sys.h"
#include "util/MathHelper.h"

namespace mc {

Timer::Timer(float tps)
    : ticksPerSecond(tps)
    , lastSyncSysClock(Minecraft::getSystemTime())
    , lastSyncHRClock(Sys::nanoTime() / 1000000LL)
{
}

void Timer::updateTimer()
{
    const int64_t i = Minecraft::getSystemTime();
    const int64_t j = i - lastSyncSysClock;
    const int64_t k = Sys::nanoTime() / 1000000LL;
    const double d0 = static_cast<double>(k) / 1000.0;

    if (j <= 1000LL && j >= 0LL) {
        counter += j;

        if (counter > 1000LL) {
            const int64_t l = k - lastSyncHRClock;
            const double d1 = static_cast<double>(counter) / static_cast<double>(l);
            timeSyncAdjustment += (d1 - timeSyncAdjustment) * 0.20000000298023224;
            lastSyncHRClock = k;
            counter = 0LL;
        }

        if (counter < 0LL) {
            lastSyncHRClock = k;
        }
    } else {
        lastHRTime = d0;
    }

    lastSyncSysClock = i;
    double d2 = (d0 - lastHRTime) * timeSyncAdjustment;
    lastHRTime = d0;
    d2 = MathHelper::clamp_double(d2, 0.0, 1.0);
    elapsedPartialTicks = static_cast<float>(static_cast<double>(elapsedPartialTicks)
        + d2 * static_cast<double>(timerSpeed) * static_cast<double>(ticksPerSecond));
    elapsedTicks = static_cast<int>(elapsedPartialTicks);
    elapsedPartialTicks -= static_cast<float>(elapsedTicks);

    if (elapsedTicks > 10) {
        elapsedTicks = 10;
    }

    renderPartialTicks = elapsedPartialTicks;
}

} // namespace mc

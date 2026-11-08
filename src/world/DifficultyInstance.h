#pragma once

// Original: net/minecraft/world/DifficultyInstance.java (with World.getDifficultyForLocation as a free function)

#include "util/BlockPos.h"
#include "util/MathHelper.h"
#include "world/EnumDifficulty.h"
#include "world/World.h"
#include "world/chunk/Chunk.h"

#include <cstdint>

namespace mc {

class DifficultyInstance {
public:
    DifficultyInstance(EnumDifficulty worldDifficultyIn, int64_t worldTime, int64_t chunkInhabitedTime, float moonPhaseFactor)
        : worldDifficulty(worldDifficultyIn)
        , additionalDifficulty(calculateAdditionalDifficulty(worldDifficultyIn, worldTime, chunkInhabitedTime, moonPhaseFactor))
    {
    }

    float getAdditionalDifficulty() const { return additionalDifficulty; }
    float getClampedAdditionalDifficulty() const
    {
        return additionalDifficulty < 2.0f ? 0.0f : (additionalDifficulty > 4.0f ? 1.0f : (additionalDifficulty - 2.0f) / 2.0f);
    }

private:
    static float calculateAdditionalDifficulty(EnumDifficulty difficulty, int64_t worldTime, int64_t chunkInhabitedTime, float moonPhaseFactor)
    {
        if (difficulty == EnumDifficulty::PEACEFUL)
            return 0.0f;

        const bool flag = difficulty == EnumDifficulty::HARD;
        float f = 0.75f;
        const float f1 = MathHelper::clamp_float((static_cast<float>(worldTime) + -72000.0f) / 1440000.0f, 0.0f, 1.0f) * 0.25f;
        f = f + f1;
        float f2 = 0.0f;
        f2 = f2 + MathHelper::clamp_float(static_cast<float>(chunkInhabitedTime) / 3600000.0f, 0.0f, 1.0f) * (flag ? 1.0f : 0.75f);
        f2 = f2 + MathHelper::clamp_float(moonPhaseFactor * 0.25f, 0.0f, f1);

        if (difficulty == EnumDifficulty::EASY)
            f2 *= 0.5f;

        f = f + f2;
        return static_cast<float>(getDifficultyId(difficulty)) * f;
    }

    EnumDifficulty worldDifficulty;
    float additionalDifficulty;
};

// Java: World.getDifficultyForLocation
inline DifficultyInstance getDifficultyForLocation(World& world, const BlockPos& pos)
{
    int64_t i = 0;
    float f = 0.0f;

    if (world.isBlockLoaded(pos)) {
        f = world.getCurrentMoonPhaseFactor();
        i = world.getChunkFromBlockCoords(pos)->getInhabitedTime();
    }

    return DifficultyInstance(world.getDifficulty(), world.getWorldTime(), i, f);
}

} // namespace mc

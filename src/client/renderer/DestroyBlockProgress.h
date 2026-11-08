#pragma once

// Original: net/minecraft/client/renderer/DestroyBlockProgress.java -- a block someone is breaking (crack stage).

#include "util/BlockPos.h"

namespace mc {

class DestroyBlockProgress {
public:
    DestroyBlockProgress(int miningPlayerEntIdIn, const BlockPos& positionIn)
        : miningPlayerEntId(miningPlayerEntIdIn)
        , position(positionIn)
    {
    }

    const BlockPos& getPosition() const { return position; }
    void setPartialBlockDamage(int damage) { partialBlockProgress = damage > 10 ? 10 : damage; }
    int getPartialBlockDamage() const { return partialBlockProgress; }
    void setCloudUpdateTick(int tick) { createdAtCloudUpdateTick = tick; }
    int getCreationCloudUpdateTick() const { return createdAtCloudUpdateTick; }

private:
    int miningPlayerEntId;
    BlockPos position;
    int partialBlockProgress = 0;
    int createdAtCloudUpdateTick = 0;
};

} // namespace mc

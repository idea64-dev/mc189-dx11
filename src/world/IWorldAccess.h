#pragma once

// Original: net/minecraft/world/IWorldAccess.java -- what the renderer (RenderGlobal) and the server's world
// manager hear about world changes. All but the first three have empty defaults here, so listeners override
// only what they handle.

#include "util/BlockPos.h"

#include <string>
#include <vector>

namespace mc {

class Entity;
class EntityPlayer;

class IWorldAccess {
public:
    virtual ~IWorldAccess() = default;
    virtual void markBlockForUpdate(const BlockPos& pos) = 0;
    virtual void notifyLightSet(const BlockPos& pos) = 0;
    virtual void markBlockRangeForRenderUpdate(int x1, int y1, int z1, int x2, int y2, int z2) = 0;
    virtual void playSound(const std::string&, double, double, double, float, float) { }
    virtual void playSoundToNearExcept(EntityPlayer*, const std::string&, double, double, double, float, float) { }
    virtual void spawnParticle(int, bool, double, double, double, double, double, double, const std::vector<int>&) { }
    virtual void onEntityAdded(Entity*) { }
    virtual void onEntityRemoved(Entity*) { }
    virtual void playRecord(const std::string&, const BlockPos&) { }
    virtual void broadcastSound(int, const BlockPos&, int) { }
    virtual void playAuxSFX(EntityPlayer*, int, const BlockPos&, int) { }
    virtual void sendBlockBreakProgress(int, const BlockPos&, int) { }
};

} // namespace mc

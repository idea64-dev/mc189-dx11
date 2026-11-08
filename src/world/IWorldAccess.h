#pragma once

// Original: net/minecraft/world/IWorldAccess.java -- what the renderer (RenderGlobal) and the server's entity
// tracker hear about world changes. Sounds, particles, entities and block-break progress are added with the
// systems that send them (stages 7, 10, 13 and 15).

#include "util/BlockPos.h"

namespace mc {

class IWorldAccess {
public:
    virtual ~IWorldAccess() = default;
    virtual void markBlockForUpdate(const BlockPos& pos) = 0;
    virtual void notifyLightSet(const BlockPos& pos) = 0;
    virtual void markBlockRangeForRenderUpdate(int x1, int y1, int z1, int x2, int y2, int z2) = 0;
};

} // namespace mc

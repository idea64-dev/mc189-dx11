#pragma once

// Original: net/minecraft/client/renderer/RegionRenderCache.java and RegionRenderCacheBuilder.java
// The chunk cache a render chunk is built from: block states and combined lights are cached per position of the
// 20x20x20 region (the 16^3 section and a one-block border).

#include "block/Block.h"
#include "client/renderer/WorldRenderer.h"
#include "world/ChunkCache.h"

#include <array>
#include <memory>

namespace mc {

class RegionRenderCache : public ChunkCache {
public:
    RegionRenderCache(World* worldIn, const BlockPos& posFromIn, const BlockPos& posToIn, int subIn);

    TileEntity* getTileEntity(const BlockPos& pos) override;
    int getCombinedLight(const BlockPos& pos, int lightValue) override;
    const IBlockState* getBlockState(const BlockPos& pos) override;

private:
    const IBlockState* getBlockStateRaw(const BlockPos& pos);
    int getPositionIndex(const BlockPos& pos) const;

    BlockPos position;
    std::array<int, 8000> combinedLights;
    std::array<const IBlockState*, 8000> blockStates {};
};

class RegionRenderCacheBuilder {
public:
    RegionRenderCacheBuilder();

    WorldRenderer& getWorldRendererByLayer(EnumWorldBlockLayer layer) { return *worldRenderers[static_cast<size_t>(layer)]; }
    WorldRenderer& getWorldRendererByLayerId(int id) { return *worldRenderers[static_cast<size_t>(id)]; }

private:
    std::array<std::unique_ptr<WorldRenderer>, 4> worldRenderers;
};

} // namespace mc

#pragma once

// Original: net/minecraft/world/ChunkCache.java -- a read-only view of the chunks around a region, for rendering
// (RegionRenderCache) and path finding.
// The chunks are referenced, not copied: as in Java, their blocks are read while the world keeps changing them.

#include "world/EnumSkyBlock.h"
#include "world/IBlockAccess.h"

#include <vector>

namespace mc {

class Chunk;
class World;

class ChunkCache : public IBlockAccess {
public:
    ChunkCache(World* worldIn, const BlockPos& posFromIn, const BlockPos& posToIn, int subIn);

    bool extendedLevelsInChunkCache() override { return hasExtendedLevels; }
    TileEntity* getTileEntity(const BlockPos& pos) override;
    int getCombinedLight(const BlockPos& pos, int lightValue) override;
    const IBlockState* getBlockState(const BlockPos& pos) override;
    BiomeGenBase* getBiomeGenForCoords(const BlockPos& pos) override;
    bool isAirBlock(const BlockPos& pos) override;
    int getLightFor(EnumSkyBlock type, const BlockPos& pos);
    int getStrongPower(const BlockPos& pos, EnumFacing direction) override;
    const WorldType* getWorldType() const override;

protected:
    Chunk* chunkAt(int i, int j) const { return chunkArray[static_cast<size_t>(i) * static_cast<size_t>(sizeZ) + static_cast<size_t>(j)]; }

    int chunkX;
    int chunkZ;
    int sizeX;
    int sizeZ;
    std::vector<Chunk*> chunkArray;
    bool hasExtendedLevels;
    World* worldObj;

private:
    int getLightForExt(EnumSkyBlock type, const BlockPos& pos);
};

} // namespace mc

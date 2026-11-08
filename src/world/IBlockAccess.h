#pragma once

// Original: net/minecraft/world/IBlockAccess.java -- what blocks may ask of the world they are in: the world
// itself and the client's render-time ChunkCache.

#include "util/BlockPos.h"
#include "util/EnumFacing.h"

namespace mc {

class BiomeGenBase;
class IBlockState;
class TileEntity;
class WorldType;

class IBlockAccess {
public:
    virtual ~IBlockAccess() = default;

    virtual TileEntity* getTileEntity(const BlockPos& pos) = 0;
    virtual int getCombinedLight(const BlockPos& pos, int lightValue) = 0;
    virtual const IBlockState* getBlockState(const BlockPos& pos) = 0;
    virtual bool isAirBlock(const BlockPos& pos) = 0;
    virtual BiomeGenBase* getBiomeGenForCoords(const BlockPos& pos) = 0;
    virtual bool extendedLevelsInChunkCache() = 0;
    virtual int getStrongPower(const BlockPos& pos, EnumFacing direction) = 0;
    virtual const WorldType* getWorldType() const = 0;
};

} // namespace mc

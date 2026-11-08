#include "world/ChunkCache.h"

#include "block/Block.h"
#include "block/material/Material.h"
#include "block/state/BlockState.h"
#include "init/Blocks.h"
#include "world/World.h"
#include "world/WorldProvider.h"
#include "world/chunk/Chunk.h"

namespace mc {

ChunkCache::ChunkCache(World* worldIn, const BlockPos& posFromIn, const BlockPos& posToIn, int subIn)
    : worldObj(worldIn)
{
    chunkX = (posFromIn.getX() - subIn) >> 4;
    chunkZ = (posFromIn.getZ() - subIn) >> 4;
    const int i = (posToIn.getX() + subIn) >> 4;
    const int j = (posToIn.getZ() + subIn) >> 4;
    sizeX = i - chunkX + 1;
    sizeZ = j - chunkZ + 1;
    chunkArray.assign(static_cast<size_t>(sizeX) * static_cast<size_t>(sizeZ), nullptr);
    hasExtendedLevels = true;

    for (int k = chunkX; k <= i; ++k) {
        for (int l = chunkZ; l <= j; ++l)
            chunkArray[static_cast<size_t>(k - chunkX) * static_cast<size_t>(sizeZ) + static_cast<size_t>(l - chunkZ)] = worldIn->getChunkFromChunkCoords(k, l);
    }

    for (int i1 = posFromIn.getX() >> 4; i1 <= posToIn.getX() >> 4; ++i1) {
        for (int j1 = posFromIn.getZ() >> 4; j1 <= posToIn.getZ() >> 4; ++j1) {
            Chunk* chunk = chunkAt(i1 - chunkX, j1 - chunkZ);
            if (chunk != nullptr && !chunk->getAreLevelsEmpty(posFromIn.getY(), posToIn.getY()))
                hasExtendedLevels = false;
        }
    }
}

TileEntity* ChunkCache::getTileEntity(const BlockPos& pos)
{
    const int i = (pos.getX() >> 4) - chunkX;
    const int j = (pos.getZ() >> 4) - chunkZ;
    return chunkAt(i, j)->getTileEntity(pos, Chunk::EnumCreateEntityType::IMMEDIATE).get();
}

int ChunkCache::getCombinedLight(const BlockPos& pos, int lightValue)
{
    const int i = getLightForExt(EnumSkyBlock::SKY, pos);
    int j = getLightForExt(EnumSkyBlock::BLOCK, pos);
    if (j < lightValue)
        j = lightValue;
    return i << 20 | j << 4;
}

const IBlockState* ChunkCache::getBlockState(const BlockPos& pos)
{
    if (pos.getY() >= 0 && pos.getY() < 256) {
        const int i = (pos.getX() >> 4) - chunkX;
        const int j = (pos.getZ() >> 4) - chunkZ;
        if (i >= 0 && i < sizeX && j >= 0 && j < sizeZ) {
            if (Chunk* chunk = chunkAt(i, j))
                return chunk->getBlockState(pos);
        }
    }
    return Blocks::air->getDefaultState();
}

BiomeGenBase* ChunkCache::getBiomeGenForCoords(const BlockPos& pos)
{
    return worldObj->getBiomeGenForCoords(pos);
}

int ChunkCache::getLightForExt(EnumSkyBlock type, const BlockPos& pos)
{
    if (type == EnumSkyBlock::SKY && worldObj->provider->getHasNoSky())
        return 0;
    if (pos.getY() < 0 || pos.getY() >= 256)
        return defaultLightValue(type);

    if (getBlockState(pos)->getBlock()->getUseNeighborBrightness()) {
        int l = 0;
        for (EnumFacing enumfacing : kFacings) {
            const int k = getLightFor(type, pos.offset(enumfacing));
            if (k > l)
                l = k;
            if (l >= 15)
                return l;
        }
        return l;
    }

    const int i = (pos.getX() >> 4) - chunkX;
    const int j = (pos.getZ() >> 4) - chunkZ;
    return chunkAt(i, j)->getLightFor(type, pos);
}

bool ChunkCache::isAirBlock(const BlockPos& pos)
{
    return &getBlockState(pos)->getBlock()->getMaterial() == &Material::air;
}

int ChunkCache::getLightFor(EnumSkyBlock type, const BlockPos& pos)
{
    if (pos.getY() >= 0 && pos.getY() < 256) {
        const int i = (pos.getX() >> 4) - chunkX;
        const int j = (pos.getZ() >> 4) - chunkZ;
        return chunkAt(i, j)->getLightFor(type, pos);
    }
    return defaultLightValue(type);
}

int ChunkCache::getStrongPower(const BlockPos& pos, EnumFacing direction)
{
    const IBlockState* iblockstate = getBlockState(pos);
    return iblockstate->getBlock()->getStrongPower(this, pos, iblockstate, direction);
}

const WorldType* ChunkCache::getWorldType() const
{
    return worldObj->getWorldType();
}

} // namespace mc

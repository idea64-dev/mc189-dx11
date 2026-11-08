#pragma once

// Helpers shared by the block world callbacks: particles by type, Java's Math.random() and the biome colour
// blending of net/minecraft/world/biome/BiomeColorHelper.java.

#include "util/BlockPos.h"
#include "util/EnumParticleTypes.h"
#include "util/Random.h"
#include "world/IBlockAccess.h"
#include "world/World.h"
#include "world/biome/BiomeGenBase.h"

#include <vector>

namespace mc {

class Entity;
class EntityLivingBase;
class EntityPlayer;

// Java: worldIn.spawnParticle(EnumParticleTypes, x, y, z, xOffset, yOffset, zOffset, params...)
inline void spawnParticle(World* worldIn, EnumParticleTypes type, double x, double y, double z, double xOffset, double yOffset, double zOffset,
    const std::vector<int>& parameters = {})
{
    worldIn->spawnParticle(getParticleID(type), getShouldIgnoreRange(type), x, y, z, xOffset, yOffset, zOffset, parameters);
}

// Java: Math.random() -- a process-wide generator, only used for cosmetic effects.
inline double mathRandom()
{
    static Random random;
    return random.nextDouble();
}

// Java: the placer passed to onBlockPlaced / onBlockPlacedBy, seen as an Entity (EntityLivingBase extends Entity).
// EntityLivingBase is opaque until its header lands (stage 8); until then this returns null -- the one place
// to change is its definition in BlockRedstoneWorld.cpp.
Entity* placerEntity(EntityLivingBase* placer);
// Java: placer.getHorizontalFacing(); NORTH while the placer is opaque.
EnumFacing placerHorizontalFacing(EntityLivingBase* placer);
// The same for the player of onBlockActivated / onBlockClicked (EntityPlayer extends Entity).
Entity* playerEntity(EntityPlayer* player);

namespace BiomeColorHelper {

// The 3x3 average around pos of a biome colour.
template <class Resolver>
int getColorAtPos(IBlockAccess* blockAccess, const BlockPos& pos, Resolver resolver)
{
    int i = 0;
    int j = 0;
    int k = 0;
    for (const BlockPos& p : BlockPos::getAllInBox(pos.add(-1, 0, -1), pos.add(1, 0, 1))) {
        const int l = resolver(blockAccess->getBiomeGenForCoords(p), p);
        i += (l & 16711680) >> 16;
        j += (l & 65280) >> 8;
        k += l & 255;
    }
    return (i / 9 & 255) << 16 | (j / 9 & 255) << 8 | (k / 9 & 255);
}

inline int getGrassColorAtPos(IBlockAccess* blockAccess, const BlockPos& pos)
{
    return getColorAtPos(blockAccess, pos, [](BiomeGenBase* biome, const BlockPos& p) { return biome->getGrassColorAtPos(p); });
}

inline int getFoliageColorAtPos(IBlockAccess* blockAccess, const BlockPos& pos)
{
    return getColorAtPos(blockAccess, pos, [](BiomeGenBase* biome, const BlockPos& p) { return biome->getFoliageColorAtPos(p); });
}

inline int getWaterColorAtPos(IBlockAccess* blockAccess, const BlockPos& pos)
{
    return getColorAtPos(blockAccess, pos, [](BiomeGenBase* biome, const BlockPos&) { return biome->waterColorMultiplier; });
}

} // namespace BiomeColorHelper

} // namespace mc

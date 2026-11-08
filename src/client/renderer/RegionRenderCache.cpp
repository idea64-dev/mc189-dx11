#include "client/renderer/RegionRenderCache.h"

#include "block/state/BlockState.h"
#include "init/Blocks.h"
#include "world/chunk/Chunk.h"

namespace mc {

RegionRenderCache::RegionRenderCache(World* worldIn, const BlockPos& posFromIn, const BlockPos& posToIn, int subIn)
    : ChunkCache(worldIn, posFromIn, posToIn, subIn)
    , position(posFromIn.getX() - subIn, posFromIn.getY() - subIn, posFromIn.getZ() - subIn)
{
    combinedLights.fill(-1);
}

TileEntity* RegionRenderCache::getTileEntity(const BlockPos& pos)
{
    const int i = (pos.getX() >> 4) - chunkX;
    const int j = (pos.getZ() >> 4) - chunkZ;
    return chunkAt(i, j)->getTileEntity(pos, Chunk::EnumCreateEntityType::QUEUED).get();
}

int RegionRenderCache::getCombinedLight(const BlockPos& pos, int lightValue)
{
    const int i = getPositionIndex(pos);
    int j = combinedLights[static_cast<size_t>(i)];
    if (j == -1) {
        j = ChunkCache::getCombinedLight(pos, lightValue);
        combinedLights[static_cast<size_t>(i)] = j;
    }
    return j;
}

const IBlockState* RegionRenderCache::getBlockState(const BlockPos& pos)
{
    const int i = getPositionIndex(pos);
    const IBlockState* iblockstate = blockStates[static_cast<size_t>(i)];
    if (iblockstate == nullptr) {
        iblockstate = getBlockStateRaw(pos);
        blockStates[static_cast<size_t>(i)] = iblockstate;
    }
    return iblockstate;
}

const IBlockState* RegionRenderCache::getBlockStateRaw(const BlockPos& pos)
{
    if (pos.getY() >= 0 && pos.getY() < 256) {
        const int i = (pos.getX() >> 4) - chunkX;
        const int j = (pos.getZ() >> 4) - chunkZ;
        return chunkAt(i, j)->getBlockState(pos);
    }
    return Blocks::air->getDefaultState();
}

int RegionRenderCache::getPositionIndex(const BlockPos& pos) const
{
    const int i = pos.getX() - position.getX();
    const int j = pos.getY() - position.getY();
    const int k = pos.getZ() - position.getZ();
    return i * 400 + k * 20 + j;
}

RegionRenderCacheBuilder::RegionRenderCacheBuilder()
{
    worldRenderers[static_cast<size_t>(EnumWorldBlockLayer::SOLID)] = std::make_unique<WorldRenderer>(2097152);
    worldRenderers[static_cast<size_t>(EnumWorldBlockLayer::CUTOUT)] = std::make_unique<WorldRenderer>(131072);
    worldRenderers[static_cast<size_t>(EnumWorldBlockLayer::CUTOUT_MIPPED)] = std::make_unique<WorldRenderer>(131072);
    worldRenderers[static_cast<size_t>(EnumWorldBlockLayer::TRANSLUCENT)] = std::make_unique<WorldRenderer>(262144);
}

} // namespace mc

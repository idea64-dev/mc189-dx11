#include "world/World.h"

#include "block/Block.h"
#include "init/Blocks.h"
#include "util/MathHelper.h"
#include "world/IWorldAccess.h"
#include "world/WorldProvider.h"
#include "world/WorldSettings.h"
#include "world/border/WorldBorder.h"
#include "world/chunk/Chunk.h"
#include "world/chunk/IChunkProvider.h"
#include "world/storage/ISaveHandler.h"
#include "world/storage/WorldInfo.h"

#include <algorithm>
#include <cstdlib>

namespace mc {

World::World(std::shared_ptr<ISaveHandler> saveHandlerIn, std::shared_ptr<WorldInfo> info, std::unique_ptr<WorldProvider> providerIn, Profiler& profilerIn,
    bool client)
    : provider(std::move(providerIn))
    , theProfiler(profilerIn)
    , isRemote(client)
    , updateLCG(Random().nextInt())
    , saveHandler(std::move(saveHandlerIn))
    , worldInfo(std::move(info))
    , worldBorder(provider->getWorldBorder())
    , lightUpdateBlockList(32768)
{
    // Java: ambientTickCountdown = rand.nextInt(12000) -- the mood sounds, stage 13; the draw keeps rand in step.
    rand.nextInt(12000);
}

World::~World() = default;

void World::initialize(const WorldSettings&)
{
    worldInfo->setServerInitialized(true);
}

Block* World::getGroundAboveSeaLevel(const BlockPos& pos)
{
    BlockPos blockpos(pos.getX(), getSeaLevel(), pos.getZ());
    while (!isAirBlock(blockpos.up()))
        blockpos = blockpos.up();
    return getBlockState(blockpos)->getBlock();
}

bool World::isValid(const BlockPos& pos) const
{
    return pos.getX() >= -30000000 && pos.getZ() >= -30000000 && pos.getX() < 30000000 && pos.getZ() < 30000000 && pos.getY() >= 0 && pos.getY() < 256;
}

bool World::isAirBlock(const BlockPos& pos)
{
    return &getBlockState(pos)->getBlock()->getMaterial() == &Material::air;
}

bool World::isBlockLoaded(const BlockPos& pos, bool allowEmpty)
{
    return isValid(pos) && isChunkLoaded(pos.getX() >> 4, pos.getZ() >> 4, allowEmpty);
}

bool World::isAreaLoaded(const BlockPos& center, int radius, bool allowEmpty)
{
    return isAreaLoaded(center.getX() - radius, center.getY() - radius, center.getZ() - radius, center.getX() + radius, center.getY() + radius,
        center.getZ() + radius, allowEmpty);
}

bool World::isAreaLoaded(const BlockPos& from, const BlockPos& to, bool allowEmpty)
{
    return isAreaLoaded(from.getX(), from.getY(), from.getZ(), to.getX(), to.getY(), to.getZ(), allowEmpty);
}

bool World::isAreaLoaded(int xStart, int yStart, int zStart, int xEnd, int yEnd, int zEnd, bool allowEmpty)
{
    if (yEnd < 0 || yStart >= 256)
        return false;
    xStart = xStart >> 4;
    zStart = zStart >> 4;
    xEnd = xEnd >> 4;
    zEnd = zEnd >> 4;
    for (int i = xStart; i <= xEnd; ++i) {
        for (int j = zStart; j <= zEnd; ++j) {
            if (!isChunkLoaded(i, j, allowEmpty))
                return false;
        }
    }
    return true;
}

bool World::isChunkLoaded(int x, int z, bool allowEmpty)
{
    return chunkProvider->chunkExists(x, z) && (allowEmpty || !chunkProvider->provideChunk(x, z)->isEmpty());
}

Chunk* World::getChunkFromBlockCoords(const BlockPos& pos)
{
    return getChunkFromChunkCoords(pos.getX() >> 4, pos.getZ() >> 4);
}

Chunk* World::getChunkFromChunkCoords(int chunkX, int chunkZ)
{
    return chunkProvider->provideChunk(chunkX, chunkZ);
}

bool World::setBlockState(const BlockPos& pos, const IBlockState* newState, int flags)
{
    if (!isValid(pos))
        return false;
    if (!isRemote && worldInfo->getTerrainType() == &WorldType::DEBUG_WORLD)
        return false;
    Chunk* chunk = getChunkFromBlockCoords(pos);
    const Block* block = newState->getBlock();
    const IBlockState* iblockstate = chunk->setBlockState(pos, newState);
    if (!iblockstate)
        return false;
    const Block* block1 = iblockstate->getBlock();
    if (block->getLightOpacity() != block1->getLightOpacity() || block->getLightValue() != block1->getLightValue()) {
        theProfiler.startSection("checkLight");
        checkLight(pos);
        theProfiler.endSection();
    }
    if ((flags & 2) != 0 && (!isRemote || (flags & 4) == 0) && chunk->isPopulated())
        markBlockForUpdate(pos);
    // Java: with flag 1 on the server, notifyNeighborsRespectDebug and the comparator update -- stage 9.
    return true;
}

bool World::setBlockToAir(const BlockPos& pos)
{
    return setBlockState(pos, Blocks::air->getDefaultState(), 3);
}

void World::markBlockForUpdate(const BlockPos& pos)
{
    for (size_t i = 0; i < worldAccesses.size(); ++i)
        worldAccesses[i]->markBlockForUpdate(pos);
}

void World::markBlocksDirtyVertical(int x1, int z1, int x2, int z2)
{
    if (x2 > z2)
        std::swap(x2, z2);
    if (!provider->getHasNoSky()) {
        for (int j = x2; j <= z2; ++j)
            checkLightFor(EnumSkyBlock::SKY, BlockPos(x1, j, z1));
    }
    markBlockRangeForRenderUpdate(x1, x2, z1, x1, z2, z1);
}

void World::markBlockRangeForRenderUpdate(const BlockPos& rangeMin, const BlockPos& rangeMax)
{
    markBlockRangeForRenderUpdate(rangeMin.getX(), rangeMin.getY(), rangeMin.getZ(), rangeMax.getX(), rangeMax.getY(), rangeMax.getZ());
}

void World::markBlockRangeForRenderUpdate(int x1, int y1, int z1, int x2, int y2, int z2)
{
    for (size_t i = 0; i < worldAccesses.size(); ++i)
        worldAccesses[i]->markBlockRangeForRenderUpdate(x1, y1, z1, x2, y2, z2);
}

bool World::canSeeSky(const BlockPos& pos)
{
    return getChunkFromBlockCoords(pos)->canSeeSky(pos);
}

bool World::canBlockSeeSky(const BlockPos& pos)
{
    if (pos.getY() >= getSeaLevel())
        return canSeeSky(pos);
    BlockPos blockpos(pos.getX(), getSeaLevel(), pos.getZ());
    if (!canSeeSky(blockpos))
        return false;
    for (blockpos = blockpos.down(); blockpos.getY() > pos.getY(); blockpos = blockpos.down()) {
        const Block* block = getBlockState(blockpos)->getBlock();
        if (block->getLightOpacity() > 0 && !block->getMaterial().isLiquid())
            return false;
    }
    return true;
}

int World::getLight(const BlockPos& posIn)
{
    if (posIn.getY() < 0)
        return 0;
    BlockPos pos = posIn;
    if (pos.getY() >= 256)
        pos = BlockPos(pos.getX(), 255, pos.getZ());
    return getChunkFromBlockCoords(pos)->getLightSubtracted(pos, 0);
}

int World::getLight(const BlockPos& posIn, bool checkNeighbors)
{
    if (posIn.getX() < -30000000 || posIn.getZ() < -30000000 || posIn.getX() >= 30000000 || posIn.getZ() >= 30000000)
        return 15;
    if (checkNeighbors && getBlockState(posIn)->getBlock()->getUseNeighborBrightness()) {
        int i1 = getLight(posIn.up(), false);
        const int i = getLight(posIn.east(), false);
        const int j = getLight(posIn.west(), false);
        const int k = getLight(posIn.south(), false);
        const int l = getLight(posIn.north(), false);
        if (i > i1)
            i1 = i;
        if (j > i1)
            i1 = j;
        if (k > i1)
            i1 = k;
        if (l > i1)
            i1 = l;
        return i1;
    }
    if (posIn.getY() < 0)
        return 0;
    BlockPos pos = posIn;
    if (pos.getY() >= 256)
        pos = BlockPos(pos.getX(), 255, pos.getZ());
    return getChunkFromBlockCoords(pos)->getLightSubtracted(pos, skylightSubtracted);
}

BlockPos World::getHeight(const BlockPos& pos)
{
    int i;
    if (pos.getX() >= -30000000 && pos.getZ() >= -30000000 && pos.getX() < 30000000 && pos.getZ() < 30000000) {
        if (isChunkLoaded(pos.getX() >> 4, pos.getZ() >> 4, true))
            i = getChunkFromChunkCoords(pos.getX() >> 4, pos.getZ() >> 4)->getHeightValue(pos.getX() & 15, pos.getZ() & 15);
        else
            i = 0;
    } else {
        i = getSeaLevel() + 1;
    }
    return BlockPos(pos.getX(), i, pos.getZ());
}

int World::getChunksLowestHorizon(int x, int z)
{
    if (x < -30000000 || z < -30000000 || x >= 30000000 || z >= 30000000)
        return getSeaLevel() + 1;
    if (!isChunkLoaded(x >> 4, z >> 4, true))
        return 0;
    return getChunkFromChunkCoords(x >> 4, z >> 4)->getLowestHeight();
}

int World::getLightFromNeighborsFor(EnumSkyBlock type, const BlockPos& posIn)
{
    if (provider->getHasNoSky() && type == EnumSkyBlock::SKY)
        return 0;
    BlockPos pos = posIn;
    if (pos.getY() < 0)
        pos = BlockPos(pos.getX(), 0, pos.getZ());
    if (!isValid(pos))
        return defaultLightValue(type);
    if (!isBlockLoaded(pos))
        return defaultLightValue(type);
    if (getBlockState(pos)->getBlock()->getUseNeighborBrightness()) {
        int i1 = getLightFor(type, pos.up());
        const int i = getLightFor(type, pos.east());
        const int j = getLightFor(type, pos.west());
        const int k = getLightFor(type, pos.south());
        const int l = getLightFor(type, pos.north());
        if (i > i1)
            i1 = i;
        if (j > i1)
            i1 = j;
        if (k > i1)
            i1 = k;
        if (l > i1)
            i1 = l;
        return i1;
    }
    return getChunkFromBlockCoords(pos)->getLightFor(type, pos);
}

int World::getLightFor(EnumSkyBlock type, const BlockPos& posIn)
{
    BlockPos pos = posIn;
    if (pos.getY() < 0)
        pos = BlockPos(pos.getX(), 0, pos.getZ());
    if (!isValid(pos))
        return defaultLightValue(type);
    if (!isBlockLoaded(pos))
        return defaultLightValue(type);
    return getChunkFromBlockCoords(pos)->getLightFor(type, pos);
}

void World::setLightFor(EnumSkyBlock type, const BlockPos& pos, int lightValue)
{
    if (isValid(pos) && isBlockLoaded(pos)) {
        getChunkFromBlockCoords(pos)->setLightFor(type, pos, lightValue);
        notifyLightSet(pos);
    }
}

void World::notifyLightSet(const BlockPos& pos)
{
    for (size_t i = 0; i < worldAccesses.size(); ++i)
        worldAccesses[i]->notifyLightSet(pos);
}

int World::getCombinedLight(const BlockPos& pos, int lightValue)
{
    const int i = getLightFromNeighborsFor(EnumSkyBlock::SKY, pos);
    int j = getLightFromNeighborsFor(EnumSkyBlock::BLOCK, pos);
    if (j < lightValue)
        j = lightValue;
    return i << 20 | j << 4;
}

float World::getLightBrightness(const BlockPos& pos)
{
    return provider->getLightBrightnessTable()[static_cast<size_t>(getLightFromNeighbors(pos))];
}

const IBlockState* World::getBlockState(const BlockPos& pos)
{
    if (!isValid(pos))
        return Blocks::air->getDefaultState();
    return getChunkFromBlockCoords(pos)->getBlockState(pos);
}

void World::removeWorldAccess(IWorldAccess* worldAccess)
{
    std::erase(worldAccesses, worldAccess);
}

int World::calculateSkylightSubtracted(float partialTicks)
{
    const float f = getCelestialAngle(partialTicks);
    float f1 = 1.0f - (MathHelper::cos(f * 3.1415927f * 2.0f) * 2.0f + 0.5f);
    f1 = MathHelper::clamp_float(f1, 0.0f, 1.0f);
    f1 = 1.0f - f1;
    f1 = static_cast<float>(static_cast<double>(f1) * (1.0 - static_cast<double>(getRainStrength(partialTicks) * 5.0f) / 16.0));
    f1 = static_cast<float>(static_cast<double>(f1) * (1.0 - static_cast<double>(getThunderStrength(partialTicks) * 5.0f) / 16.0));
    f1 = 1.0f - f1;
    return static_cast<int>(f1 * 11.0f);
}

float World::getCelestialAngle(float partialTicks)
{
    return provider->calculateCelestialAngle(worldInfo->getWorldTime(), partialTicks);
}

int World::getMoonPhase()
{
    return provider->getMoonPhase(worldInfo->getWorldTime());
}

float World::getCurrentMoonPhaseFactor()
{
    return WorldProvider::moonPhaseFactors[provider->getMoonPhase(worldInfo->getWorldTime())];
}

float World::getCelestialAngleRadians(float partialTicks)
{
    const float f = getCelestialAngle(partialTicks);
    return f * 3.1415927f * 2.0f;
}

BlockPos World::getPrecipitationHeight(const BlockPos& pos)
{
    return getChunkFromBlockCoords(pos)->getPrecipitationHeight(pos);
}

BlockPos World::getTopSolidOrLiquidBlock(const BlockPos& pos)
{
    Chunk* chunk = getChunkFromBlockCoords(pos);
    BlockPos blockpos(pos.getX(), chunk->getTopFilledSegment() + 16, pos.getZ());
    for (; blockpos.getY() >= 0;) {
        const BlockPos blockpos1 = blockpos.down();
        const Material& material = chunk->getBlock(blockpos1)->getMaterial();
        if (material.blocksMovement() && &material != &Material::leaves)
            break;
        blockpos = blockpos1;
    }
    return blockpos;
}

void World::calculateInitialSkylight()
{
    const int i = calculateSkylightSubtracted(1.0f);
    if (i != skylightSubtracted)
        skylightSubtracted = i;
}

bool World::checkLight(const BlockPos& pos)
{
    bool flag = false;
    if (!provider->getHasNoSky())
        flag |= checkLightFor(EnumSkyBlock::SKY, pos);
    flag = flag | checkLightFor(EnumSkyBlock::BLOCK, pos);
    return flag;
}

int World::getRawLight(const BlockPos& pos, EnumSkyBlock lightType)
{
    if (lightType == EnumSkyBlock::SKY && canSeeSky(pos))
        return 15;
    const Block* block = getBlockState(pos)->getBlock();
    int i = lightType == EnumSkyBlock::SKY ? 0 : block->getLightValue();
    int j = block->getLightOpacity();
    if (j >= 15 && block->getLightValue() > 0)
        j = 1;
    if (j < 1)
        j = 1;
    if (j >= 15)
        return 0;
    if (i >= 14)
        return i;
    for (const EnumFacing facing : kFacings) {
        const int k = getLightFor(lightType, pos.offset(facing)) - j;
        if (k > i)
            i = k;
        if (i >= 14)
            return i;
    }
    return i;
}

bool World::checkLightFor(EnumSkyBlock lightType, const BlockPos& pos)
{
    if (!isAreaLoaded(pos, 17, false))
        return false;
    int i = 0;
    int j = 0;
    theProfiler.startSection("getBrightness");
    const int k = getLightFor(lightType, pos);
    const int l = getRawLight(pos, lightType);
    const int i1 = pos.getX();
    const int j1 = pos.getY();
    const int k1 = pos.getZ();
    std::vector<int32_t>& list = lightUpdateBlockList;
    const int length = static_cast<int>(list.size());

    if (l > k) {
        list[static_cast<size_t>(j++)] = 133152;
    } else if (l < k) {
        list[static_cast<size_t>(j++)] = 133152 | k << 18;
        while (i < j) {
            const int l1 = list[static_cast<size_t>(i++)];
            const int i2 = (l1 & 63) - 32 + i1;
            const int j2 = (l1 >> 6 & 63) - 32 + j1;
            const int k2 = (l1 >> 12 & 63) - 32 + k1;
            const int l2 = l1 >> 18 & 15;
            const BlockPos blockpos(i2, j2, k2);
            int i3 = getLightFor(lightType, blockpos);
            if (i3 == l2) {
                setLightFor(lightType, blockpos, 0);
                if (l2 > 0) {
                    const int j3 = MathHelper::abs_int(i2 - i1);
                    const int k3 = MathHelper::abs_int(j2 - j1);
                    const int l3 = MathHelper::abs_int(k2 - k1);
                    if (j3 + k3 + l3 < 17) {
                        BlockPos mutablePos;
                        for (const EnumFacing facing : kFacings) {
                            const int i4 = i2 + getFrontOffsetX(facing);
                            const int j4 = j2 + getFrontOffsetY(facing);
                            const int k4 = k2 + getFrontOffsetZ(facing);
                            mutablePos.set(i4, j4, k4);
                            const int l4 = std::max(1, getBlockState(mutablePos)->getBlock()->getLightOpacity());
                            i3 = getLightFor(lightType, mutablePos);
                            if (i3 == l2 - l4 && j < length)
                                list[static_cast<size_t>(j++)] = (i4 - i1 + 32) | ((j4 - j1 + 32) << 6) | ((k4 - k1 + 32) << 12) | ((l2 - l4) << 18);
                        }
                    }
                }
            }
        }
        i = 0;
    }

    theProfiler.endSection();
    theProfiler.startSection("checkedPosition < toCheckCount");
    while (i < j) {
        const int i5 = list[static_cast<size_t>(i++)];
        const int j5 = (i5 & 63) - 32 + i1;
        const int k5 = (i5 >> 6 & 63) - 32 + j1;
        const int l5 = (i5 >> 12 & 63) - 32 + k1;
        const BlockPos blockpos1(j5, k5, l5);
        const int i6 = getLightFor(lightType, blockpos1);
        const int j6 = getRawLight(blockpos1, lightType);
        if (j6 != i6) {
            setLightFor(lightType, blockpos1, j6);
            if (j6 > i6) {
                const int k6 = std::abs(j5 - i1);
                const int l6 = std::abs(k5 - j1);
                const int i7 = std::abs(l5 - k1);
                const bool flag = j < length - 6;
                if (k6 + l6 + i7 < 17 && flag) {
                    if (getLightFor(lightType, blockpos1.west()) < j6)
                        list[static_cast<size_t>(j++)] = j5 - 1 - i1 + 32 + ((k5 - j1 + 32) << 6) + ((l5 - k1 + 32) << 12);
                    if (getLightFor(lightType, blockpos1.east()) < j6)
                        list[static_cast<size_t>(j++)] = j5 + 1 - i1 + 32 + ((k5 - j1 + 32) << 6) + ((l5 - k1 + 32) << 12);
                    if (getLightFor(lightType, blockpos1.down()) < j6)
                        list[static_cast<size_t>(j++)] = j5 - i1 + 32 + ((k5 - 1 - j1 + 32) << 6) + ((l5 - k1 + 32) << 12);
                    if (getLightFor(lightType, blockpos1.up()) < j6)
                        list[static_cast<size_t>(j++)] = j5 - i1 + 32 + ((k5 + 1 - j1 + 32) << 6) + ((l5 - k1 + 32) << 12);
                    if (getLightFor(lightType, blockpos1.north()) < j6)
                        list[static_cast<size_t>(j++)] = j5 - i1 + 32 + ((k5 - j1 + 32) << 6) + ((l5 - 1 - k1 + 32) << 12);
                    if (getLightFor(lightType, blockpos1.south()) < j6)
                        list[static_cast<size_t>(j++)] = j5 - i1 + 32 + ((k5 - j1 + 32) << 6) + ((l5 + 1 - k1 + 32) << 12);
                }
            }
        }
    }
    theProfiler.endSection();
    return true;
}

const WorldType* World::getWorldType() const
{
    return worldInfo->getTerrainType();
}

void World::setTotalWorldTime(int64_t worldTime)
{
    worldInfo->setWorldTotalTime(worldTime);
}

int64_t World::getSeed() const
{
    return worldInfo->getSeed();
}

int64_t World::getTotalWorldTime() const
{
    return worldInfo->getWorldTotalTime();
}

int64_t World::getWorldTime() const
{
    return worldInfo->getWorldTime();
}

void World::setWorldTime(int64_t time)
{
    worldInfo->setWorldTime(time);
}

BlockPos World::getSpawnPoint()
{
    BlockPos blockpos(worldInfo->getSpawnX(), worldInfo->getSpawnY(), worldInfo->getSpawnZ());
    if (!getWorldBorder().contains(blockpos))
        blockpos = getHeight(BlockPos(getWorldBorder().getCenterX(), 0.0, getWorldBorder().getCenterZ()));
    return blockpos;
}

void World::setSpawnPoint(const BlockPos& pos)
{
    worldInfo->setSpawn(pos);
}

float World::getThunderStrength(float delta)
{
    return (prevThunderingStrength + (thunderingStrength - prevThunderingStrength) * delta) * getRainStrength(delta);
}

void World::setThunderStrength(float strength)
{
    prevThunderingStrength = strength;
    thunderingStrength = strength;
}

float World::getRainStrength(float delta)
{
    return prevRainingStrength + (rainingStrength - prevRainingStrength) * delta;
}

void World::setRainStrength(float strength)
{
    prevRainingStrength = strength;
    rainingStrength = strength;
}

bool World::isThundering()
{
    return static_cast<double>(getThunderStrength(1.0f)) > 0.9;
}

bool World::isRaining()
{
    return static_cast<double>(getRainStrength(1.0f)) > 0.2;
}

int World::getActualHeight() const
{
    return provider->getHasNoSky() ? 128 : 256;
}

Random& World::setRandomSeed(int x, int y, int z)
{
    const int64_t i = java::add(java::add(java::add(java::mul(static_cast<int64_t>(x), static_cast<int64_t>(341873128712LL)),
                                              java::mul(static_cast<int64_t>(y), static_cast<int64_t>(132897987541LL))),
                                    getWorldInfo().getSeed()),
        static_cast<int64_t>(z));
    rand.setSeed(i);
    return rand;
}

double World::getHorizon() const
{
    return worldInfo->getTerrainType() == &WorldType::FLAT ? 0.0 : 63.0;
}

void World::checkSessionLock()
{
    saveHandler->checkSessionLock();
}

bool World::isSpawnChunk(int x, int z)
{
    const BlockPos blockpos = getSpawnPoint();
    const int i = x * 16 + 8 - blockpos.getX();
    const int j = z * 16 + 8 - blockpos.getZ();
    const int k = 128;
    return i >= -k && i <= k && j >= -k && j <= k;
}

} // namespace mc

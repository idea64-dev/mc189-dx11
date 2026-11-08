#include "world/chunk/Chunk.h"

#include "block/Block.h"
#include "core/Java.h"
#include "core/Log.h"
#include "entity/Entity.h"
#include "util/MathHelper.h"
#include "init/Blocks.h"
#include "tileentity/TileEntity.h"
#include "world/biome/BiomeGenBase.h"
#include "world/biome/WorldChunkManager.h"
#include "world/World.h"
#include "world/WorldProvider.h"
#include "world/chunk/IChunkProvider.h"

#include <algorithm>
#include <climits>
#include <stdexcept>

namespace mc {

// --- ChunkPrimer ---

const IBlockState* ChunkPrimer::getBlockState(int index) const
{
    if (index < 0 || index >= static_cast<int>(data.size()))
        throw std::out_of_range("The coordinate is out of range");
    const IBlockState* iblockstate = Block::BLOCK_STATE_IDS.getByValue(data[static_cast<size_t>(index)]);
    return iblockstate ? iblockstate : Blocks::air->getDefaultState();
}

void ChunkPrimer::setBlockState(int index, const IBlockState* state)
{
    if (index < 0 || index >= static_cast<int>(data.size()))
        throw std::out_of_range("The coordinate is out of range");
    data[static_cast<size_t>(index)] = static_cast<int16_t>(Block::BLOCK_STATE_IDS.get(state));
}

// --- Chunk ---

Chunk::Chunk(World* worldIn, int x, int z)
    : xPosition(x)
    , zPosition(z)
    , worldObj(worldIn)
{
    precipitationHeightMap.fill(-999);
    blockBiomeArray.fill(255);
}

Chunk::Chunk(World* worldIn, const ChunkPrimer& primer, int x, int z)
    : Chunk(worldIn, x, z)
{
    const int i = 256;
    const bool flag = !worldIn->provider->getHasNoSky();
    for (int j = 0; j < 16; ++j) {
        for (int k = 0; k < 16; ++k) {
            for (int l = 0; l < i; ++l) {
                const int i1 = j * i * 16 | k * i | l;
                const IBlockState* iblockstate = primer.getBlockState(i1);
                if (&iblockstate->getBlock()->getMaterial() != &Material::air) {
                    const int j1 = l >> 4;
                    if (!storageArrays[static_cast<size_t>(j1)])
                        storageArrays[static_cast<size_t>(j1)] = std::make_unique<ExtendedBlockStorage>(j1 << 4, flag);
                    storageArrays[static_cast<size_t>(j1)]->set(j, l & 15, k, iblockstate);
                }
            }
        }
    }
}

Chunk::~Chunk() = default;

int Chunk::getTopFilledSegment() const
{
    for (int i = static_cast<int>(storageArrays.size()) - 1; i >= 0; --i) {
        if (storageArrays[static_cast<size_t>(i)])
            return storageArrays[static_cast<size_t>(i)]->getYLocation();
    }
    return 0;
}

void Chunk::generateHeightMap()
{
    const int i = getTopFilledSegment();
    heightMapMinimum = INT_MAX;
    for (int j = 0; j < 16; ++j) {
        for (int k = 0; k < 16; ++k) {
            precipitationHeightMap[static_cast<size_t>(j + (k << 4))] = -999;
            for (int l = i + 16; l > 0; --l) {
                const Block* block = getBlock0(j, l - 1, k);
                if (block->getLightOpacity() != 0) {
                    heightMap[static_cast<size_t>(k << 4 | j)] = l;
                    if (l < heightMapMinimum)
                        heightMapMinimum = l;
                    break;
                }
            }
        }
    }
    isModified = true;
}

void Chunk::generateSkylightMap()
{
    const int i = getTopFilledSegment();
    heightMapMinimum = INT_MAX;
    for (int j = 0; j < 16; ++j) {
        for (int k = 0; k < 16; ++k) {
            precipitationHeightMap[static_cast<size_t>(j + (k << 4))] = -999;
            for (int l = i + 16; l > 0; --l) {
                if (getBlockLightOpacity(j, l - 1, k) != 0) {
                    heightMap[static_cast<size_t>(k << 4 | j)] = l;
                    if (l < heightMapMinimum)
                        heightMapMinimum = l;
                    break;
                }
            }
            if (!worldObj->provider->getHasNoSky()) {
                int k1 = 15;
                int i1 = i + 16 - 1;
                while (true) {
                    int j1 = getBlockLightOpacity(j, i1, k);
                    if (j1 == 0 && k1 != 15)
                        j1 = 1;
                    k1 -= j1;
                    if (k1 > 0) {
                        ExtendedBlockStorage* storage = storageArrays[static_cast<size_t>(i1 >> 4)].get();
                        if (storage) {
                            storage->setExtSkylightValue(j, i1 & 15, k, k1);
                            worldObj->notifyLightSet(BlockPos((xPosition << 4) + j, i1, (zPosition << 4) + k));
                        }
                    }
                    --i1;
                    if (i1 <= 0 || k1 <= 0)
                        break;
                }
            }
        }
    }
    isModified = true;
}

void Chunk::propagateSkylightOcclusion(int x, int z)
{
    updateSkylightColumns[static_cast<size_t>(x + z * 16)] = true;
    isGapLightingUpdated = true;
}

void Chunk::recheckGaps(bool onlyOne)
{
    worldObj->theProfiler.startSection("recheckGaps");
    if (worldObj->isAreaLoaded(BlockPos(xPosition * 16 + 8, 0, zPosition * 16 + 8), 16)) {
        for (int i = 0; i < 16; ++i) {
            for (int j = 0; j < 16; ++j) {
                if (updateSkylightColumns[static_cast<size_t>(i + j * 16)]) {
                    updateSkylightColumns[static_cast<size_t>(i + j * 16)] = false;
                    const int k = getHeightValue(i, j);
                    const int l = xPosition * 16 + i;
                    const int i1 = zPosition * 16 + j;
                    int j1 = INT_MAX;
                    for (const EnumFacing facing : horizontalFacings())
                        j1 = std::min(j1, worldObj->getChunksLowestHorizon(l + getFrontOffsetX(facing), i1 + getFrontOffsetZ(facing)));
                    checkSkylightNeighborHeight(l, i1, j1);
                    for (const EnumFacing facing : horizontalFacings())
                        checkSkylightNeighborHeight(l + getFrontOffsetX(facing), i1 + getFrontOffsetZ(facing), k);
                    if (onlyOne) {
                        worldObj->theProfiler.endSection();
                        return;
                    }
                }
            }
        }
        isGapLightingUpdated = false;
    }
    worldObj->theProfiler.endSection();
}

void Chunk::checkSkylightNeighborHeight(int x, int z, int maxValue)
{
    const int i = worldObj->getHeight(BlockPos(x, 0, z)).getY();
    if (i > maxValue)
        updateSkylightNeighborHeight(x, z, maxValue, i + 1);
    else if (i < maxValue)
        updateSkylightNeighborHeight(x, z, i, maxValue + 1);
}

void Chunk::updateSkylightNeighborHeight(int x, int z, int startY, int endY)
{
    if (endY > startY && worldObj->isAreaLoaded(BlockPos(x, 0, z), 16)) {
        for (int i = startY; i < endY; ++i)
            worldObj->checkLightFor(EnumSkyBlock::SKY, BlockPos(x, i, z));
        isModified = true;
    }
}

void Chunk::relightBlock(int x, int y, int z)
{
    const int i = heightMap[static_cast<size_t>(z << 4 | x)] & 255;
    int j = i;
    if (y > i)
        j = y;
    while (j > 0 && getBlockLightOpacity(x, j - 1, z) == 0)
        --j;
    if (j == i)
        return;
    worldObj->markBlocksDirtyVertical(x + xPosition * 16, z + zPosition * 16, j, i);
    heightMap[static_cast<size_t>(z << 4 | x)] = j;
    const int k = xPosition * 16 + x;
    const int l = zPosition * 16 + z;
    if (!worldObj->provider->getHasNoSky()) {
        if (j < i) {
            for (int j1 = j; j1 < i; ++j1) {
                if (ExtendedBlockStorage* storage = storageArrays[static_cast<size_t>(j1 >> 4)].get()) {
                    storage->setExtSkylightValue(x, j1 & 15, z, 15);
                    worldObj->notifyLightSet(BlockPos((xPosition << 4) + x, j1, (zPosition << 4) + z));
                }
            }
        } else {
            for (int i1 = i; i1 < j; ++i1) {
                if (ExtendedBlockStorage* storage = storageArrays[static_cast<size_t>(i1 >> 4)].get()) {
                    storage->setExtSkylightValue(x, i1 & 15, z, 0);
                    worldObj->notifyLightSet(BlockPos((xPosition << 4) + x, i1, (zPosition << 4) + z));
                }
            }
        }
        int k1 = 15;
        while (j > 0 && k1 > 0) {
            --j;
            int i2 = getBlockLightOpacity(x, j, z);
            if (i2 == 0)
                i2 = 1;
            k1 -= i2;
            if (k1 < 0)
                k1 = 0;
            if (ExtendedBlockStorage* storage = storageArrays[static_cast<size_t>(j >> 4)].get())
                storage->setExtSkylightValue(x, j & 15, z, k1);
        }
    }
    const int l1 = heightMap[static_cast<size_t>(z << 4 | x)];
    int j2 = i;
    int k2 = l1;
    if (l1 < i) {
        j2 = l1;
        k2 = i;
    }
    if (l1 < heightMapMinimum)
        heightMapMinimum = l1;
    if (!worldObj->provider->getHasNoSky()) {
        for (const EnumFacing facing : horizontalFacings())
            updateSkylightNeighborHeight(k + getFrontOffsetX(facing), l + getFrontOffsetZ(facing), j2, k2);
        updateSkylightNeighborHeight(k, l, j2, k2);
    }
    isModified = true;
}

int Chunk::getBlockLightOpacity(const BlockPos& pos) const
{
    return getBlock(pos)->getLightOpacity();
}

int Chunk::getBlockLightOpacity(int x, int y, int z) const
{
    return getBlock0(x, y, z)->getLightOpacity();
}

Block* Chunk::getBlock0(int x, int y, int z) const
{
    if (y >= 0 && y >> 4 < static_cast<int>(storageArrays.size())) {
        if (const ExtendedBlockStorage* storage = storageArrays[static_cast<size_t>(y >> 4)].get())
            return storage->getBlockByExtId(x, y & 15, z);
    }
    return Blocks::air;
}

Block* Chunk::getBlock(const BlockPos& pos) const
{
    return getBlock0(pos.getX() & 15, pos.getY(), pos.getZ() & 15);
}

const IBlockState* Chunk::getBlockState(const BlockPos& pos) const
{
    // Java: the debug world type (all block states laid out at y 70) -- with ChunkProviderDebug, stage 6.
    if (pos.getY() >= 0 && pos.getY() >> 4 < static_cast<int>(storageArrays.size())) {
        if (const ExtendedBlockStorage* storage = storageArrays[static_cast<size_t>(pos.getY() >> 4)].get())
            return storage->get(pos.getX() & 15, pos.getY() & 15, pos.getZ() & 15);
    }
    return Blocks::air->getDefaultState();
}

int Chunk::getBlockMetadata(int x, int y, int z) const
{
    if (y >> 4 >= static_cast<int>(storageArrays.size()))
        return 0;
    // Java indexes the array directly; a negative y throws there.
    const ExtendedBlockStorage* storage = storageArrays.at(static_cast<size_t>(y >> 4)).get();
    return storage ? storage->getExtBlockMetadata(x, y & 15, z) : 0;
}

int Chunk::getBlockMetadata(const BlockPos& pos) const
{
    return getBlockMetadata(pos.getX() & 15, pos.getY(), pos.getZ() & 15);
}

const IBlockState* Chunk::setBlockState(const BlockPos& pos, const IBlockState* state)
{
    const int i = pos.getX() & 15;
    const int j = pos.getY();
    const int k = pos.getZ() & 15;
    const int l = k << 4 | i;
    if (j >= precipitationHeightMap[static_cast<size_t>(l)] - 1)
        precipitationHeightMap[static_cast<size_t>(l)] = -999;
    const int i1 = heightMap[static_cast<size_t>(l)];
    const IBlockState* iblockstate = getBlockState(pos);
    if (iblockstate == state)
        return nullptr;
    const Block* block = state->getBlock();
    const Block* block1 = iblockstate->getBlock();
    // Java indexes the array directly; y outside 0..255 throws there.
    std::unique_ptr<ExtendedBlockStorage>& slot = storageArrays.at(static_cast<size_t>(j >> 4));
    bool flag = false;
    if (!slot) {
        if (block == Blocks::air)
            return nullptr;
        slot = std::make_unique<ExtendedBlockStorage>(j >> 4 << 4, !worldObj->provider->getHasNoSky());
        flag = j >= i1;
    }
    ExtendedBlockStorage* storage = slot.get();
    storage->set(i, j & 15, k, state);
    if (block1 != block) {
        if (!worldObj->isRemote)
            const_cast<Block*>(block1)->breakBlock(worldObj, pos, iblockstate);
        else if (block1->hasTileEntity())
            worldObj->removeTileEntity(pos);
    }
    if (storage->getBlockByExtId(i, j & 15, k) != block)
        return nullptr;
    if (flag) {
        generateSkylightMap();
    } else {
        const int j1 = block->getLightOpacity();
        const int k1 = block1->getLightOpacity();
        if (j1 > 0) {
            if (j >= i1)
                relightBlock(i, j + 1, k);
        } else if (j == i1 - 1) {
            relightBlock(i, j, k);
        }
        if (j1 != k1 && (j1 < k1 || getLightFor(EnumSkyBlock::SKY, pos) > 0 || getLightFor(EnumSkyBlock::BLOCK, pos) > 0))
            propagateSkylightOcclusion(i, k);
    }
    if (block1->hasTileEntity()) {
        if (const auto tileentity = getTileEntity(pos, EnumCreateEntityType::CHECK))
            tileentity->updateContainingBlockInfo();
    }
    if (!worldObj->isRemote && block1 != block)
        const_cast<Block*>(block)->onBlockAdded(worldObj, pos, state);
    if (block->hasTileEntity()) {
        auto tileentity1 = getTileEntity(pos, EnumCreateEntityType::CHECK);
        if (!tileentity1) {
            tileentity1 = const_cast<Block*>(block)->createNewTileEntity(worldObj, block->getMetaFromState(state));
            worldObj->setTileEntity(pos, tileentity1);
        }
        if (tileentity1)
            tileentity1->updateContainingBlockInfo();
    }
    isModified = true;
    return iblockstate;
}

int Chunk::getLightFor(EnumSkyBlock type, const BlockPos& pos) const
{
    const int i = pos.getX() & 15;
    const int j = pos.getY();
    const int k = pos.getZ() & 15;
    const ExtendedBlockStorage* storage = storageArrays.at(static_cast<size_t>(j >> 4)).get();
    if (!storage)
        return canSeeSky(pos) ? defaultLightValue(type) : 0;
    if (type == EnumSkyBlock::SKY)
        return worldObj->provider->getHasNoSky() ? 0 : storage->getExtSkylightValue(i, j & 15, k);
    return storage->getExtBlocklightValue(i, j & 15, k);
}

void Chunk::setLightFor(EnumSkyBlock type, const BlockPos& pos, int value)
{
    const int i = pos.getX() & 15;
    const int j = pos.getY();
    const int k = pos.getZ() & 15;
    std::unique_ptr<ExtendedBlockStorage>& slot = storageArrays.at(static_cast<size_t>(j >> 4));
    if (!slot) {
        slot = std::make_unique<ExtendedBlockStorage>(j >> 4 << 4, !worldObj->provider->getHasNoSky());
        generateSkylightMap();
    }
    ExtendedBlockStorage* storage = slot.get();
    isModified = true;
    if (type == EnumSkyBlock::SKY) {
        if (!worldObj->provider->getHasNoSky())
            storage->setExtSkylightValue(i, j & 15, k, value);
    } else {
        storage->setExtBlocklightValue(i, j & 15, k, value);
    }
}

int Chunk::getLightSubtracted(const BlockPos& pos, int amount) const
{
    const int i = pos.getX() & 15;
    const int j = pos.getY();
    const int k = pos.getZ() & 15;
    const ExtendedBlockStorage* storage = storageArrays.at(static_cast<size_t>(j >> 4)).get();
    if (!storage)
        return !worldObj->provider->getHasNoSky() && amount < defaultLightValue(EnumSkyBlock::SKY) ? defaultLightValue(EnumSkyBlock::SKY) - amount : 0;
    int l = worldObj->provider->getHasNoSky() ? 0 : storage->getExtSkylightValue(i, j & 15, k);
    l = l - amount;
    const int i1 = storage->getExtBlocklightValue(i, j & 15, k);
    if (i1 > l)
        l = i1;
    return l;
}

bool Chunk::canSeeSky(const BlockPos& pos) const
{
    return pos.getY() >= heightMap[static_cast<size_t>((pos.getZ() & 15) << 4 | (pos.getX() & 15))];
}

void Chunk::onChunkLoad()
{
    isChunkLoaded = true;
    std::vector<std::shared_ptr<TileEntity>> tiles;
    for (const auto& entry : chunkTileEntityMap)
        tiles.push_back(entry.second);
    worldObj->addTileEntities(tiles);
    for (const auto& list : entityLists) {
        for (const auto& entity : list)
            entity->onChunkLoad();
        worldObj->loadEntities(list);
    }
}

void Chunk::onChunkUnload()
{
    isChunkLoaded = false;
    for (const auto& entry : chunkTileEntityMap)
        worldObj->markTileEntityForRemoval(entry.second);
    for (const auto& list : entityLists)
        worldObj->unloadEntities(list);
}

void Chunk::addEntity(const std::shared_ptr<Entity>& entityIn)
{
    hasEntities = true;
    const int i = MathHelper::floor_double(entityIn->posX / 16.0);
    const int j = MathHelper::floor_double(entityIn->posZ / 16.0);
    if (i != xPosition || j != zPosition) {
        Log::warn("Wrong location! ({}, {}) should be ({}, {}), {}", i, j, xPosition, zPosition, entityIn->toString());
        entityIn->setDead();
    }
    int k = MathHelper::floor_double(entityIn->posY / 16.0);
    if (k < 0)
        k = 0;
    if (k >= static_cast<int>(entityLists.size()))
        k = static_cast<int>(entityLists.size()) - 1;
    entityIn->addedToChunk = true;
    entityIn->chunkCoordX = xPosition;
    entityIn->chunkCoordY = k;
    entityIn->chunkCoordZ = zPosition;
    entityLists[static_cast<size_t>(k)].push_back(entityIn);
}

void Chunk::removeEntity(Entity* entityIn)
{
    removeEntityAtIndex(entityIn, entityIn->chunkCoordY);
}

void Chunk::removeEntityAtIndex(Entity* entityIn, int index)
{
    if (index < 0)
        index = 0;
    if (index >= static_cast<int>(entityLists.size()))
        index = static_cast<int>(entityLists.size()) - 1;
    auto& list = entityLists[static_cast<size_t>(index)];
    const auto it = std::find_if(list.begin(), list.end(), [entityIn](const auto& e) { return e.get() == entityIn; });
    if (it != list.end())
        list.erase(it);
}

void Chunk::getEntitiesWithinAABBForEntity(const Entity* entityIn, const AxisAlignedBB& aabb, std::vector<Entity*>& listToFill,
    const std::function<bool(Entity*)>& predicate)
{
    int i = MathHelper::floor_double((aabb.minY - 2.0) / 16.0);
    int j = MathHelper::floor_double((aabb.maxY + 2.0) / 16.0);
    i = MathHelper::clamp_int(i, 0, static_cast<int>(entityLists.size()) - 1);
    j = MathHelper::clamp_int(j, 0, static_cast<int>(entityLists.size()) - 1);
    for (int k = i; k <= j; ++k) {
        for (const auto& entity : entityLists[static_cast<size_t>(k)]) {
            if (entity.get() != entityIn && entity->getEntityBoundingBox().intersectsWith(aabb) && (!predicate || predicate(entity.get()))) {
                listToFill.push_back(entity.get());
                for (Entity* part : entity->getParts()) {
                    if (part != entityIn && part->getEntityBoundingBox().intersectsWith(aabb) && (!predicate || predicate(part)))
                        listToFill.push_back(part);
                }
            }
        }
    }
}

std::shared_ptr<TileEntity> Chunk::createNewTileEntity(const BlockPos& pos)
{
    Block* block = getBlock(pos);
    return !block->hasTileEntity() ? nullptr : block->createNewTileEntity(worldObj, getBlockMetadata(pos));
}

std::shared_ptr<TileEntity> Chunk::getTileEntity(const BlockPos& pos, EnumCreateEntityType type)
{
    const std::shared_ptr<TileEntity>* found = chunkTileEntityMap.find(pos);
    std::shared_ptr<TileEntity> tileentity = found ? *found : nullptr;
    if (!tileentity) {
        if (type == EnumCreateEntityType::IMMEDIATE) {
            tileentity = createNewTileEntity(pos);
            worldObj->setTileEntity(pos, tileentity);
        } else if (type == EnumCreateEntityType::QUEUED) {
            tileEntityPosQueue.push_back(pos);
        }
    } else if (tileentity->isInvalid()) {
        chunkTileEntityMap.erase(pos);
        return nullptr;
    }
    return tileentity;
}

void Chunk::addTileEntity(const std::shared_ptr<TileEntity>& tileEntityIn)
{
    addTileEntity(tileEntityIn->getPos(), tileEntityIn);
    if (isChunkLoaded)
        worldObj->addTileEntity(tileEntityIn);
}

void Chunk::addTileEntity(const BlockPos& pos, const std::shared_ptr<TileEntity>& tileEntityIn)
{
    tileEntityIn->setWorldObj(worldObj);
    tileEntityIn->setPos(pos);
    if (getBlock(pos)->hasTileEntity()) {
        if (const std::shared_ptr<TileEntity>* old = chunkTileEntityMap.find(pos))
            (*old)->invalidate();
        tileEntityIn->validate();
        chunkTileEntityMap.put(pos, tileEntityIn);
    }
}

void Chunk::removeTileEntity(const BlockPos& pos)
{
    if (isChunkLoaded) {
        const std::shared_ptr<TileEntity>* found = chunkTileEntityMap.find(pos);
        if (found) {
            const std::shared_ptr<TileEntity> tileentity = *found;
            chunkTileEntityMap.erase(pos);
            tileentity->invalidate();
        }
    }
}

BiomeGenBase* Chunk::getBiome(const BlockPos& pos, WorldChunkManager* chunkManager)
{
    const int i = pos.getX() & 15;
    const int j = pos.getZ() & 15;
    int k = blockBiomeArray[static_cast<size_t>(j << 4 | i)] & 255;
    if (k == 255) {
        BiomeGenBase* biomegenbase = chunkManager->getBiomeGenerator(pos, BiomeGenBase::plains);
        k = biomegenbase->biomeID;
        blockBiomeArray[static_cast<size_t>(j << 4 | i)] = static_cast<uint8_t>(k & 255);
    }
    BiomeGenBase* biomegenbase1 = BiomeGenBase::getBiome(k);
    return biomegenbase1 ? biomegenbase1 : BiomeGenBase::plains;
}

bool Chunk::needsSaving(bool saveAll) const
{
    if (saveAll) {
        if ((hasEntities && worldObj->getTotalWorldTime() != lastSaveTime) || isModified)
            return true;
    } else if (hasEntities && worldObj->getTotalWorldTime() >= lastSaveTime + 600LL) {
        return true;
    }
    return isModified;
}

Random Chunk::getRandomWithSeed(int64_t seed) const
{
    const int64_t a = static_cast<int64_t>(java::mul(java::mul(xPosition, xPosition), 4987142));
    const int64_t b = static_cast<int64_t>(java::mul(xPosition, 5947611));
    const int64_t c = java::mul(static_cast<int64_t>(java::mul(zPosition, zPosition)), static_cast<int64_t>(4392871LL));
    const int64_t d = static_cast<int64_t>(java::mul(zPosition, 389711));
    return Random(java::add(java::add(java::add(java::add(worldObj->getSeed(), a), b), c), d) ^ seed);
}

void Chunk::populateChunk(IChunkProvider* provider1, IChunkProvider* provider2, int x, int z)
{
    const bool flag = provider1->chunkExists(x, z - 1);
    const bool flag1 = provider1->chunkExists(x + 1, z);
    const bool flag2 = provider1->chunkExists(x, z + 1);
    const bool flag3 = provider1->chunkExists(x - 1, z);
    const bool flag4 = provider1->chunkExists(x - 1, z - 1);
    const bool flag5 = provider1->chunkExists(x + 1, z + 1);
    const bool flag6 = provider1->chunkExists(x - 1, z + 1);
    const bool flag7 = provider1->chunkExists(x + 1, z - 1);
    const auto populateAt = [&](int cx, int cz) {
        Chunk* chunk = cx == x && cz == z ? this : provider1->provideChunk(cx, cz);
        if (!chunk->terrainPopulated)
            provider1->populate(provider2, cx, cz);
        else
            provider1->populateChunk(provider2, chunk, cx, cz);
    };
    if (flag1 && flag2 && flag5)
        populateAt(x, z);
    if (flag3 && flag2 && flag6)
        populateAt(x - 1, z);
    if (flag && flag1 && flag7)
        populateAt(x, z - 1);
    if (flag4 && flag && flag3)
        populateAt(x - 1, z - 1);
}

BlockPos Chunk::getPrecipitationHeight(const BlockPos& pos)
{
    const int i = pos.getX() & 15;
    const int j = pos.getZ() & 15;
    const int k = i | j << 4;
    if (precipitationHeightMap[static_cast<size_t>(k)] == -999) {
        const int l = getTopFilledSegment() + 15;
        BlockPos blockpos(pos.getX(), l, pos.getZ());
        int i1 = -1;
        while (blockpos.getY() > 0 && i1 == -1) {
            const Material& material = getBlock(blockpos)->getMaterial();
            if (!material.blocksMovement() && !material.isLiquid())
                blockpos = blockpos.down();
            else
                i1 = blockpos.getY() + 1;
        }
        precipitationHeightMap[static_cast<size_t>(k)] = i1;
    }
    return BlockPos(pos.getX(), precipitationHeightMap[static_cast<size_t>(k)], pos.getZ());
}

void Chunk::func_150804_b(bool skipRecheckGaps)
{
    if (isGapLightingUpdated && !worldObj->provider->getHasNoSky() && !skipRecheckGaps)
        recheckGaps(worldObj->isRemote);
    field_150815_m = true;
    if (!lightPopulated && terrainPopulated)
        func_150809_p();
    // Java: a ConcurrentLinkedQueue polled until empty, so positions queued meanwhile are handled too.
    for (size_t index = 0; index < tileEntityPosQueue.size(); ++index) {
        const BlockPos blockpos = tileEntityPosQueue[index];
        if (!getTileEntity(blockpos, EnumCreateEntityType::CHECK) && getBlock(blockpos)->hasTileEntity()) {
            const auto tileentity = createNewTileEntity(blockpos);
            worldObj->setTileEntity(blockpos, tileentity);
            worldObj->markBlockRangeForRenderUpdate(blockpos, blockpos);
        }
    }
    tileEntityPosQueue.clear();
}

bool Chunk::getAreLevelsEmpty(int startY, int endY) const
{
    if (startY < 0)
        startY = 0;
    if (endY >= 256)
        endY = 255;
    for (int i = startY; i <= endY; i += 16) {
        const ExtendedBlockStorage* storage = storageArrays[static_cast<size_t>(i >> 4)].get();
        if (storage && !storage->isEmpty())
            return false;
    }
    return true;
}

void Chunk::setStorageArrays(std::array<std::unique_ptr<ExtendedBlockStorage>, 16> newStorageArrays)
{
    storageArrays = std::move(newStorageArrays);
}

void Chunk::fillChunk(std::span<const uint8_t> data, int mask, bool fullChunk)
{
    size_t i = 0;
    const bool flag = !worldObj->provider->getHasNoSky();
    for (int j = 0; j < 16; ++j) {
        std::unique_ptr<ExtendedBlockStorage>& slot = storageArrays[static_cast<size_t>(j)];
        if ((mask & 1 << j) != 0) {
            if (!slot)
                slot = std::make_unique<ExtendedBlockStorage>(j << 4, flag);
            auto& ids = slot->getData();
            for (auto& id : ids) {
                id = static_cast<uint16_t>((data[i + 1] & 255) << 8 | (data[i] & 255));
                i += 2;
            }
        } else if (fullChunk && slot) {
            slot.reset();
        }
    }
    for (int l = 0; l < 16; ++l) {
        if ((mask & 1 << l) != 0 && storageArrays[static_cast<size_t>(l)]) {
            auto& nibbles = storageArrays[static_cast<size_t>(l)]->getBlocklightArray()->getData();
            std::copy_n(data.begin() + static_cast<std::ptrdiff_t>(i), nibbles.size(), nibbles.begin());
            i += nibbles.size();
        }
    }
    if (flag) {
        for (int i1 = 0; i1 < 16; ++i1) {
            if ((mask & 1 << i1) != 0 && storageArrays[static_cast<size_t>(i1)]) {
                auto& nibbles = storageArrays[static_cast<size_t>(i1)]->getSkylightArray()->getData();
                std::copy_n(data.begin() + static_cast<std::ptrdiff_t>(i), nibbles.size(), nibbles.begin());
                i += nibbles.size();
            }
        }
    }
    if (fullChunk)
        std::copy_n(data.begin() + static_cast<std::ptrdiff_t>(i), blockBiomeArray.size(), blockBiomeArray.begin());
    for (int j1 = 0; j1 < 16; ++j1) {
        if (storageArrays[static_cast<size_t>(j1)] && (mask & 1 << j1) != 0)
            storageArrays[static_cast<size_t>(j1)]->removeInvalidBlocks();
    }
    lightPopulated = true;
    terrainPopulated = true;
    generateHeightMap();
    // Java: tileentity.updateContainingBlockInfo() for every tile entity -- stage 9.
}

void Chunk::setBiomeArray(std::span<const uint8_t> biomeArray)
{
    if (biomeArray.size() != blockBiomeArray.size()) {
        Log::warn("Could not set level chunk biomes, array length is {} instead of {}", biomeArray.size(), blockBiomeArray.size());
        return;
    }
    std::copy(biomeArray.begin(), biomeArray.end(), blockBiomeArray.begin());
}

void Chunk::enqueueRelightChecks()
{
    const BlockPos blockpos(xPosition << 4, 0, zPosition << 4);
    for (int i = 0; i < 8; ++i) {
        if (queuedLightChecks >= 4096)
            return;
        const int j = queuedLightChecks % 16;
        const int k = queuedLightChecks / 16 % 16;
        const int l = queuedLightChecks / 256;
        ++queuedLightChecks;
        for (int i1 = 0; i1 < 16; ++i1) {
            const BlockPos blockpos1 = blockpos.add(k, (j << 4) + i1, l);
            const bool flag = i1 == 0 || i1 == 15 || k == 0 || k == 15 || l == 0 || l == 15;
            const ExtendedBlockStorage* storage = storageArrays[static_cast<size_t>(j)].get();
            if ((!storage && flag) || (storage && &storage->getBlockByExtId(k, i1, l)->getMaterial() == &Material::air)) {
                for (const EnumFacing facing : kFacings) {
                    const BlockPos blockpos2 = blockpos1.offset(facing);
                    if (worldObj->getBlockState(blockpos2)->getBlock()->getLightValue() > 0)
                        worldObj->checkLight(blockpos2);
                }
                worldObj->checkLight(blockpos1);
            }
        }
    }
}

void Chunk::func_150809_p()
{
    terrainPopulated = true;
    lightPopulated = true;
    const BlockPos blockpos(xPosition << 4, 0, zPosition << 4);
    if (worldObj->provider->getHasNoSky())
        return;
    if (worldObj->isAreaLoaded(blockpos.add(-1, 0, -1), blockpos.add(16, worldObj->getSeaLevel(), 16))) {
        for (int i = 0; i < 16 && lightPopulated; ++i) {
            for (int j = 0; j < 16; ++j) {
                if (!func_150811_f(i, j)) {
                    lightPopulated = false;
                    break;
                }
            }
        }
        if (lightPopulated) {
            for (const EnumFacing facing : horizontalFacings()) {
                const int k = getAxisDirection(facing) == AxisDirection::POSITIVE ? 16 : 1;
                worldObj->getChunkFromBlockCoords(blockpos.offset(facing, k))->func_180700_a(getOpposite(facing));
            }
            func_177441_y();
        }
    } else {
        lightPopulated = false;
    }
}

void Chunk::func_177441_y()
{
    updateSkylightColumns.fill(true);
    recheckGaps(false);
}

void Chunk::func_180700_a(EnumFacing facing)
{
    if (!terrainPopulated)
        return;
    if (facing == EnumFacing::EAST) {
        for (int i = 0; i < 16; ++i)
            func_150811_f(15, i);
    } else if (facing == EnumFacing::WEST) {
        for (int j = 0; j < 16; ++j)
            func_150811_f(0, j);
    } else if (facing == EnumFacing::SOUTH) {
        for (int k = 0; k < 16; ++k)
            func_150811_f(k, 15);
    } else if (facing == EnumFacing::NORTH) {
        for (int l = 0; l < 16; ++l)
            func_150811_f(l, 0);
    }
}

bool Chunk::func_150811_f(int x, int z)
{
    const int i = getTopFilledSegment();
    bool flag = false;
    bool flag1 = false;
    BlockPos pos((xPosition << 4) + x, 0, (zPosition << 4) + z);
    for (int j = i + 16 - 1; j > worldObj->getSeaLevel() || (j > 0 && !flag1); --j) {
        pos.set(pos.getX(), j, pos.getZ());
        const int k = getBlockLightOpacity(pos);
        if (k == 255 && pos.getY() < worldObj->getSeaLevel())
            flag1 = true;
        if (!flag && k > 0)
            flag = true;
        else if (flag && k == 0 && !worldObj->checkLight(pos))
            return false;
    }
    for (int l = pos.getY(); l > 0; --l) {
        pos.set(pos.getX(), l, pos.getZ());
        if (getBlock(pos)->getLightValue() > 0)
            worldObj->checkLight(pos);
    }
    return true;
}

void Chunk::setHeightMap(std::span<const int> newHeightMap)
{
    if (newHeightMap.size() != heightMap.size()) {
        Log::warn("Could not set level chunk heightmap, array length is {} instead of {}", newHeightMap.size(), heightMap.size());
        return;
    }
    std::copy(newHeightMap.begin(), newHeightMap.end(), heightMap.begin());
}

// --- EmptyChunk ---

Block* EmptyChunk::getBlock(const BlockPos&) const
{
    return Blocks::air;
}

} // namespace mc

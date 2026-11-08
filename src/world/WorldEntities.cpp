// Original: net/minecraft/world/World.java -- the entity side: spawning and removal, the tick loop
// (updateEntities), collisions, ray tracing, material checks and the entity queries.

#include "world/World.h"

#include "block/AllBlocks.h"
#include "entity/Entity.h"
#include "entity/player/EntityPlayer.h"
#include "init/Blocks.h"
#include "tileentity/TileEntity.h"
#include "util/MathHelper.h"
#include "world/IWorldAccess.h"
#include "world/border/WorldBorder.h"
#include "world/chunk/Chunk.h"
#include "world/chunk/IChunkProvider.h"

#include <algorithm>
#include <cmath>

namespace mc {

namespace {

template <class T>
void removeFirstOf(std::vector<std::shared_ptr<T>>& list, const void* item)
{
    const auto it = std::find_if(list.begin(), list.end(), [item](const auto& e) { return e.get() == item; });
    if (it != list.end())
        list.erase(it);
}

} // namespace

bool World::notSpectating(Entity* entity)
{
    auto* player = dynamic_cast<EntityPlayer*>(entity);
    return !player || !player->isSpectator();
}

std::optional<MovingObjectPosition> World::rayTraceBlocks(Vec3 vec31, const Vec3& vec32, bool stopOnLiquid, bool ignoreBlockWithoutBoundingBox,
    bool returnLastUncollidableBlock)
{
    if (std::isnan(vec31.xCoord) || std::isnan(vec31.yCoord) || std::isnan(vec31.zCoord))
        return std::nullopt;
    if (std::isnan(vec32.xCoord) || std::isnan(vec32.yCoord) || std::isnan(vec32.zCoord))
        return std::nullopt;
    const int i = MathHelper::floor_double(vec32.xCoord);
    const int j = MathHelper::floor_double(vec32.yCoord);
    const int k = MathHelper::floor_double(vec32.zCoord);
    int l = MathHelper::floor_double(vec31.xCoord);
    int i1 = MathHelper::floor_double(vec31.yCoord);
    int j1 = MathHelper::floor_double(vec31.zCoord);
    BlockPos blockpos(l, i1, j1);
    const IBlockState* iblockstate = getBlockState(blockpos);
    Block* block = iblockstate->getBlock();
    if ((!ignoreBlockWithoutBoundingBox || block->getCollisionBoundingBox(this, blockpos, iblockstate)) && block->canCollideCheck(iblockstate, stopOnLiquid)) {
        if (auto movingobjectposition = block->collisionRayTrace(this, blockpos, vec31, vec32))
            return movingobjectposition;
    }
    std::optional<MovingObjectPosition> movingobjectposition2;
    int k1 = 200;
    while (k1-- >= 0) {
        if (std::isnan(vec31.xCoord) || std::isnan(vec31.yCoord) || std::isnan(vec31.zCoord))
            return std::nullopt;
        if (l == i && i1 == j && j1 == k)
            return returnLastUncollidableBlock ? movingobjectposition2 : std::nullopt;
        bool flag2 = true;
        bool flag = true;
        bool flag1 = true;
        double d0 = 999.0;
        double d1 = 999.0;
        double d2 = 999.0;
        if (i > l)
            d0 = static_cast<double>(l) + 1.0;
        else if (i < l)
            d0 = static_cast<double>(l) + 0.0;
        else
            flag2 = false;
        if (j > i1)
            d1 = static_cast<double>(i1) + 1.0;
        else if (j < i1)
            d1 = static_cast<double>(i1) + 0.0;
        else
            flag = false;
        if (k > j1)
            d2 = static_cast<double>(j1) + 1.0;
        else if (k < j1)
            d2 = static_cast<double>(j1) + 0.0;
        else
            flag1 = false;
        double d3 = 999.0;
        double d4 = 999.0;
        double d5 = 999.0;
        const double d6 = vec32.xCoord - vec31.xCoord;
        const double d7 = vec32.yCoord - vec31.yCoord;
        const double d8 = vec32.zCoord - vec31.zCoord;
        if (flag2)
            d3 = (d0 - vec31.xCoord) / d6;
        if (flag)
            d4 = (d1 - vec31.yCoord) / d7;
        if (flag1)
            d5 = (d2 - vec31.zCoord) / d8;
        // Java compares with -0.0D, which 0.0 equals too.
        if (d3 == -0.0)
            d3 = -1.0E-4;
        if (d4 == -0.0)
            d4 = -1.0E-4;
        if (d5 == -0.0)
            d5 = -1.0E-4;
        EnumFacing enumfacing;
        if (d3 < d4 && d3 < d5) {
            enumfacing = i > l ? EnumFacing::WEST : EnumFacing::EAST;
            vec31 = Vec3(d0, vec31.yCoord + d7 * d3, vec31.zCoord + d8 * d3);
        } else if (d4 < d5) {
            enumfacing = j > i1 ? EnumFacing::DOWN : EnumFacing::UP;
            vec31 = Vec3(vec31.xCoord + d6 * d4, d1, vec31.zCoord + d8 * d4);
        } else {
            enumfacing = k > j1 ? EnumFacing::NORTH : EnumFacing::SOUTH;
            vec31 = Vec3(vec31.xCoord + d6 * d5, vec31.yCoord + d7 * d5, d2);
        }
        l = MathHelper::floor_double(vec31.xCoord) - (enumfacing == EnumFacing::EAST ? 1 : 0);
        i1 = MathHelper::floor_double(vec31.yCoord) - (enumfacing == EnumFacing::UP ? 1 : 0);
        j1 = MathHelper::floor_double(vec31.zCoord) - (enumfacing == EnumFacing::SOUTH ? 1 : 0);
        blockpos = BlockPos(l, i1, j1);
        const IBlockState* iblockstate1 = getBlockState(blockpos);
        Block* block1 = iblockstate1->getBlock();
        if (!ignoreBlockWithoutBoundingBox || block1->getCollisionBoundingBox(this, blockpos, iblockstate1)) {
            if (block1->canCollideCheck(iblockstate1, stopOnLiquid)) {
                if (auto movingobjectposition1 = block1->collisionRayTrace(this, blockpos, vec31, vec32))
                    return movingobjectposition1;
            } else {
                movingobjectposition2 = MovingObjectPosition(MovingObjectPosition::MovingObjectType::MISS, vec31, enumfacing, blockpos);
            }
        }
    }
    return returnLastUncollidableBlock ? movingobjectposition2 : std::nullopt;
}

void World::playSoundAtEntity(Entity* entityIn, const std::string& name, float volume, float pitch)
{
    for (size_t i = 0; i < worldAccesses.size(); ++i)
        worldAccesses[i]->playSound(name, entityIn->posX, entityIn->posY, entityIn->posZ, volume, pitch);
}

void World::playSoundToNearExcept(EntityPlayer* player, const std::string& name, float volume, float pitch)
{
    for (size_t i = 0; i < worldAccesses.size(); ++i)
        worldAccesses[i]->playSoundToNearExcept(player, name, player->posX, player->posY, player->posZ, volume, pitch);
}

bool World::addWeatherEffect(const std::shared_ptr<Entity>& entityIn)
{
    weatherEffects.push_back(entityIn);
    return true;
}

bool World::spawnEntityInWorld(const std::shared_ptr<Entity>& entityIn)
{
    const int i = MathHelper::floor_double(entityIn->posX / 16.0);
    const int j = MathHelper::floor_double(entityIn->posZ / 16.0);
    bool flag = entityIn->forceSpawn;
    auto player = std::dynamic_pointer_cast<EntityPlayer>(entityIn);
    if (player)
        flag = true;
    if (!flag && !isChunkLoaded(i, j, true))
        return false;
    if (player) {
        playerEntities.push_back(player);
        updateAllPlayersSleepingFlag();
    }
    getChunkFromChunkCoords(i, j)->addEntity(entityIn);
    loadedEntityList.push_back(entityIn);
    onEntityAdded(entityIn.get());
    return true;
}

void World::onEntityAdded(Entity* entityIn)
{
    for (size_t i = 0; i < worldAccesses.size(); ++i)
        worldAccesses[i]->onEntityAdded(entityIn);
}

void World::onEntityRemoved(Entity* entityIn)
{
    for (size_t i = 0; i < worldAccesses.size(); ++i)
        worldAccesses[i]->onEntityRemoved(entityIn);
}

void World::removeEntity(Entity* entityIn)
{
    if (entityIn->riddenByEntity)
        entityIn->riddenByEntity->mountEntity(nullptr);
    if (entityIn->ridingEntity)
        entityIn->mountEntity(nullptr);
    entityIn->setDead();
    if (dynamic_cast<EntityPlayer*>(entityIn)) {
        removeFirstOf(playerEntities, entityIn);
        updateAllPlayersSleepingFlag();
        onEntityRemoved(entityIn);
    }
}

void World::removePlayerEntityDangerously(Entity* entityIn)
{
    // Keeps the entity alive until the end of this call.
    std::shared_ptr<Entity> keep;
    for (const auto& e : loadedEntityList) {
        if (e.get() == entityIn)
            keep = e;
    }
    entityIn->setDead();
    if (dynamic_cast<EntityPlayer*>(entityIn)) {
        removeFirstOf(playerEntities, entityIn);
        updateAllPlayersSleepingFlag();
    }
    const int i = entityIn->chunkCoordX;
    const int j = entityIn->chunkCoordZ;
    if (entityIn->addedToChunk && isChunkLoaded(i, j, true))
        getChunkFromChunkCoords(i, j)->removeEntity(entityIn);
    removeFirstOf(loadedEntityList, entityIn);
    onEntityRemoved(entityIn);
}

std::vector<AxisAlignedBB> World::getCollidingBoundingBoxes(Entity* entityIn, const AxisAlignedBB& bb)
{
    std::vector<AxisAlignedBB> list;
    const int i = MathHelper::floor_double(bb.minX);
    const int j = MathHelper::floor_double(bb.maxX + 1.0);
    const int k = MathHelper::floor_double(bb.minY);
    const int l = MathHelper::floor_double(bb.maxY + 1.0);
    const int i1 = MathHelper::floor_double(bb.minZ);
    const int j1 = MathHelper::floor_double(bb.maxZ + 1.0);
    const WorldBorder& worldborder = getWorldBorder();
    const bool flag = entityIn->isOutsideBorder();
    const bool flag1 = isInsideBorder(worldborder, entityIn);
    const IBlockState* iblockstate = Blocks::stone->getDefaultState();
    BlockPos mutablePos;
    for (int k1 = i; k1 < j; ++k1) {
        for (int l1 = i1; l1 < j1; ++l1) {
            if (isBlockLoaded(mutablePos.set(k1, 64, l1))) {
                for (int i2 = k - 1; i2 < l; ++i2) {
                    mutablePos.set(k1, i2, l1);
                    if (flag && flag1)
                        entityIn->setOutsideBorder(false);
                    else if (!flag && !flag1)
                        entityIn->setOutsideBorder(true);
                    const IBlockState* iblockstate1 = iblockstate;
                    if (worldborder.contains(mutablePos) || !flag1)
                        iblockstate1 = getBlockState(mutablePos);
                    iblockstate1->getBlock()->addCollisionBoxesToList(this, mutablePos, iblockstate1, bb, list, entityIn);
                }
            }
        }
    }
    const double d0 = 0.25;
    const std::vector<Entity*> list1 = getEntitiesWithinAABBExcludingEntity(entityIn, bb.expand(d0, d0, d0));
    for (Entity* other : list1) {
        // Java compares the rider and the vehicle with the list itself here, so the check always passes.
        if (auto axisalignedbb = other->getCollisionBoundingBox(); axisalignedbb && axisalignedbb->intersectsWith(bb))
            list.push_back(*axisalignedbb);
        if (auto axisalignedbb = entityIn->getCollisionBox(other); axisalignedbb && axisalignedbb->intersectsWith(bb))
            list.push_back(*axisalignedbb);
    }
    return list;
}

bool World::isInsideBorder(const WorldBorder& worldBorderIn, const Entity* entityIn) const
{
    double d0 = worldBorderIn.minX();
    double d1 = worldBorderIn.minZ();
    double d2 = worldBorderIn.maxX();
    double d3 = worldBorderIn.maxZ();
    if (entityIn->isOutsideBorder()) {
        ++d0;
        ++d1;
        --d2;
        --d3;
    } else {
        --d0;
        --d1;
        ++d2;
        ++d3;
    }
    return entityIn->posX > d0 && entityIn->posX < d2 && entityIn->posZ > d1 && entityIn->posZ < d3;
}

std::vector<AxisAlignedBB> World::getCollisionBoxes(const AxisAlignedBB& bb)
{
    std::vector<AxisAlignedBB> list;
    const int i = MathHelper::floor_double(bb.minX);
    const int j = MathHelper::floor_double(bb.maxX + 1.0);
    const int k = MathHelper::floor_double(bb.minY);
    const int l = MathHelper::floor_double(bb.maxY + 1.0);
    const int i1 = MathHelper::floor_double(bb.minZ);
    const int j1 = MathHelper::floor_double(bb.maxZ + 1.0);
    BlockPos mutablePos;
    for (int k1 = i; k1 < j; ++k1) {
        for (int l1 = i1; l1 < j1; ++l1) {
            if (isBlockLoaded(mutablePos.set(k1, 64, l1))) {
                for (int i2 = k - 1; i2 < l; ++i2) {
                    mutablePos.set(k1, i2, l1);
                    const IBlockState* iblockstate = k1 >= -30000000 && k1 < 30000000 && l1 >= -30000000 && l1 < 30000000 ? getBlockState(mutablePos)
                                                                                                                         : Blocks::bedrock->getDefaultState();
                    iblockstate->getBlock()->addCollisionBoxesToList(this, mutablePos, iblockstate, bb, list, nullptr);
                }
            }
        }
    }
    return list;
}

void World::updateEntities()
{
    theProfiler.startSection("entities");
    theProfiler.startSection("global");
    for (size_t i = 0; i < weatherEffects.size(); ++i) {
        const std::shared_ptr<Entity> entity = weatherEffects[i];
        ++entity->ticksExisted;
        entity->onUpdate();
        if (entity->isDead)
            weatherEffects.erase(weatherEffects.begin() + static_cast<std::ptrdiff_t>(i--));
    }
    theProfiler.endStartSection("remove");
    // Java: loadedEntityList.removeAll(unloadedEntityList) -- every occurrence.
    std::erase_if(loadedEntityList, [this](const std::shared_ptr<Entity>& e) {
        return std::find(unloadedEntityList.begin(), unloadedEntityList.end(), e) != unloadedEntityList.end();
    });
    for (const auto& entity1 : unloadedEntityList) {
        const int j = entity1->chunkCoordX;
        const int l1 = entity1->chunkCoordZ;
        if (entity1->addedToChunk && isChunkLoaded(j, l1, true))
            getChunkFromChunkCoords(j, l1)->removeEntity(entity1.get());
    }
    for (const auto& entity1 : unloadedEntityList)
        onEntityRemoved(entity1.get());
    unloadedEntityList.clear();
    theProfiler.endStartSection("regular");
    for (size_t i1 = 0; i1 < loadedEntityList.size(); ++i1) {
        const std::shared_ptr<Entity> entity2 = loadedEntityList[i1];
        if (entity2->ridingEntity) {
            if (!entity2->ridingEntity->isDead && entity2->ridingEntity->riddenByEntity == entity2)
                continue;
            entity2->ridingEntity->riddenByEntity = nullptr;
            entity2->ridingEntity = nullptr;
        }
        theProfiler.startSection("tick");
        if (!entity2->isDead)
            updateEntity(entity2.get());
        theProfiler.endSection();
        theProfiler.startSection("remove");
        if (entity2->isDead) {
            const int k1 = entity2->chunkCoordX;
            const int i2 = entity2->chunkCoordZ;
            if (entity2->addedToChunk && isChunkLoaded(k1, i2, true))
                getChunkFromChunkCoords(k1, i2)->removeEntity(entity2.get());
            loadedEntityList.erase(loadedEntityList.begin() + static_cast<std::ptrdiff_t>(i1--));
            onEntityRemoved(entity2.get());
        }
        theProfiler.endSection();
    }
    theProfiler.endStartSection("blockEntities");
    processingLoadedTiles = true;
    for (size_t index = 0; index < tickableTileEntities.size();) {
        const std::shared_ptr<TileEntity> tileentity = tickableTileEntities[index];
        if (!tileentity->isInvalid() && tileentity->hasWorldObj()) {
            const BlockPos blockpos = tileentity->getPos();
            if (isBlockLoaded(blockpos) && getWorldBorder().contains(blockpos))
                tileentity->update();
        }
        if (tileentity->isInvalid()) {
            tickableTileEntities.erase(tickableTileEntities.begin() + static_cast<std::ptrdiff_t>(index));
            removeFirstOf(loadedTileEntityList, tileentity.get());
            if (isBlockLoaded(tileentity->getPos()))
                getChunkFromBlockCoords(tileentity->getPos())->removeTileEntity(tileentity->getPos());
        } else {
            ++index;
        }
    }
    processingLoadedTiles = false;
    if (!tileEntitiesToBeRemoved.empty()) {
        const auto removed = [this](const std::shared_ptr<TileEntity>& e) {
            return std::find(tileEntitiesToBeRemoved.begin(), tileEntitiesToBeRemoved.end(), e) != tileEntitiesToBeRemoved.end();
        };
        std::erase_if(tickableTileEntities, removed);
        std::erase_if(loadedTileEntityList, removed);
        tileEntitiesToBeRemoved.clear();
    }
    theProfiler.endStartSection("pendingBlockEntities");
    if (!addedTileEntityList.empty()) {
        for (size_t j1 = 0; j1 < addedTileEntityList.size(); ++j1) {
            const std::shared_ptr<TileEntity> tileentity1 = addedTileEntityList[j1];
            if (!tileentity1->isInvalid()) {
                if (std::find(loadedTileEntityList.begin(), loadedTileEntityList.end(), tileentity1) == loadedTileEntityList.end())
                    addTileEntity(tileentity1);
                if (isBlockLoaded(tileentity1->getPos()))
                    getChunkFromBlockCoords(tileentity1->getPos())->addTileEntity(tileentity1->getPos(), tileentity1);
                markBlockForUpdate(tileentity1->getPos());
            }
        }
        addedTileEntityList.clear();
    }
    theProfiler.endSection();
    theProfiler.endSection();
}

void World::updateEntityWithOptionalForce(Entity* entityIn, bool forceUpdate)
{
    const int i = MathHelper::floor_double(entityIn->posX);
    const int j = MathHelper::floor_double(entityIn->posZ);
    const int k = 32;
    if (forceUpdate && !isAreaLoaded(i - k, 0, j - k, i + k, 0, j + k, true))
        return;
    entityIn->lastTickPosX = entityIn->posX;
    entityIn->lastTickPosY = entityIn->posY;
    entityIn->lastTickPosZ = entityIn->posZ;
    entityIn->prevRotationYaw = entityIn->rotationYaw;
    entityIn->prevRotationPitch = entityIn->rotationPitch;
    if (forceUpdate && entityIn->addedToChunk) {
        ++entityIn->ticksExisted;
        if (entityIn->ridingEntity)
            entityIn->updateRidden();
        else
            entityIn->onUpdate();
    }
    theProfiler.startSection("chunkCheck");
    if (!std::isfinite(entityIn->posX))
        entityIn->posX = entityIn->lastTickPosX;
    if (!std::isfinite(entityIn->posY))
        entityIn->posY = entityIn->lastTickPosY;
    if (!std::isfinite(entityIn->posZ))
        entityIn->posZ = entityIn->lastTickPosZ;
    if (!std::isfinite(entityIn->rotationPitch))
        entityIn->rotationPitch = entityIn->prevRotationPitch;
    if (!std::isfinite(entityIn->rotationYaw))
        entityIn->rotationYaw = entityIn->prevRotationYaw;
    const int l = MathHelper::floor_double(entityIn->posX / 16.0);
    const int i1 = MathHelper::floor_double(entityIn->posY / 16.0);
    const int j1 = MathHelper::floor_double(entityIn->posZ / 16.0);
    if (!entityIn->addedToChunk || entityIn->chunkCoordX != l || entityIn->chunkCoordY != i1 || entityIn->chunkCoordZ != j1) {
        const std::shared_ptr<Entity> keep = entityIn->shared_from_this();
        if (entityIn->addedToChunk && isChunkLoaded(entityIn->chunkCoordX, entityIn->chunkCoordZ, true))
            getChunkFromChunkCoords(entityIn->chunkCoordX, entityIn->chunkCoordZ)->removeEntityAtIndex(entityIn, entityIn->chunkCoordY);
        if (isChunkLoaded(l, j1, true)) {
            entityIn->addedToChunk = true;
            getChunkFromChunkCoords(l, j1)->addEntity(keep);
        } else {
            entityIn->addedToChunk = false;
        }
    }
    theProfiler.endSection();
    if (forceUpdate && entityIn->addedToChunk && entityIn->riddenByEntity) {
        if (!entityIn->riddenByEntity->isDead && entityIn->riddenByEntity->ridingEntity.get() == entityIn) {
            updateEntity(entityIn->riddenByEntity.get());
        } else {
            entityIn->riddenByEntity->ridingEntity = nullptr;
            entityIn->riddenByEntity = nullptr;
        }
    }
}

bool World::checkNoEntityCollision(const AxisAlignedBB& bb, const Entity* entityIn)
{
    for (Entity* entity : getEntitiesWithinAABBExcludingEntity(nullptr, bb)) {
        if (!entity->isDead && entity->preventEntitySpawning && entity != entityIn
            && (!entityIn || (entityIn->ridingEntity.get() != entity && entityIn->riddenByEntity.get() != entity)))
            return false;
    }
    return true;
}

bool World::checkBlockCollision(const AxisAlignedBB& bb)
{
    const int i = MathHelper::floor_double(bb.minX);
    const int j = MathHelper::floor_double(bb.maxX);
    const int k = MathHelper::floor_double(bb.minY);
    const int l = MathHelper::floor_double(bb.maxY);
    const int i1 = MathHelper::floor_double(bb.minZ);
    const int j1 = MathHelper::floor_double(bb.maxZ);
    BlockPos mutablePos;
    for (int k1 = i; k1 <= j; ++k1) {
        for (int l1 = k; l1 <= l; ++l1) {
            for (int i2 = i1; i2 <= j1; ++i2) {
                if (&getBlockState(mutablePos.set(k1, l1, i2))->getBlock()->getMaterial() != &Material::air)
                    return true;
            }
        }
    }
    return false;
}

bool World::isAnyLiquid(const AxisAlignedBB& bb)
{
    const int i = MathHelper::floor_double(bb.minX);
    const int j = MathHelper::floor_double(bb.maxX);
    const int k = MathHelper::floor_double(bb.minY);
    const int l = MathHelper::floor_double(bb.maxY);
    const int i1 = MathHelper::floor_double(bb.minZ);
    const int j1 = MathHelper::floor_double(bb.maxZ);
    BlockPos mutablePos;
    for (int k1 = i; k1 <= j; ++k1) {
        for (int l1 = k; l1 <= l; ++l1) {
            for (int i2 = i1; i2 <= j1; ++i2) {
                if (getBlockState(mutablePos.set(k1, l1, i2))->getBlock()->getMaterial().isLiquid())
                    return true;
            }
        }
    }
    return false;
}

bool World::isFlammableWithin(const AxisAlignedBB& bb)
{
    const int i = MathHelper::floor_double(bb.minX);
    const int j = MathHelper::floor_double(bb.maxX + 1.0);
    const int k = MathHelper::floor_double(bb.minY);
    const int l = MathHelper::floor_double(bb.maxY + 1.0);
    const int i1 = MathHelper::floor_double(bb.minZ);
    const int j1 = MathHelper::floor_double(bb.maxZ + 1.0);
    if (isAreaLoaded(i, k, i1, j, l, j1, true)) {
        BlockPos mutablePos;
        for (int k1 = i; k1 < j; ++k1) {
            for (int l1 = k; l1 < l; ++l1) {
                for (int i2 = i1; i2 < j1; ++i2) {
                    const Block* block = getBlockState(mutablePos.set(k1, l1, i2))->getBlock();
                    if (block == Blocks::fire || block == Blocks::flowing_lava || block == Blocks::lava)
                        return true;
                }
            }
        }
    }
    return false;
}

bool World::handleMaterialAcceleration(const AxisAlignedBB& bb, const Material& materialIn, Entity* entityIn)
{
    const int i = MathHelper::floor_double(bb.minX);
    const int j = MathHelper::floor_double(bb.maxX + 1.0);
    const int k = MathHelper::floor_double(bb.minY);
    const int l = MathHelper::floor_double(bb.maxY + 1.0);
    const int i1 = MathHelper::floor_double(bb.minZ);
    const int j1 = MathHelper::floor_double(bb.maxZ + 1.0);
    if (!isAreaLoaded(i, k, i1, j, l, j1, true))
        return false;
    bool flag = false;
    Vec3 vec3(0.0, 0.0, 0.0);
    BlockPos mutablePos;
    for (int k1 = i; k1 < j; ++k1) {
        for (int l1 = k; l1 < l; ++l1) {
            for (int i2 = i1; i2 < j1; ++i2) {
                mutablePos.set(k1, l1, i2);
                const IBlockState* iblockstate = getBlockState(mutablePos);
                Block* block = iblockstate->getBlock();
                if (&block->getMaterial() == &materialIn) {
                    const double d0 = static_cast<double>(static_cast<float>(l1 + 1) - BlockLiquid::getLiquidHeightPercent(iblockstate->getValue(BlockLiquid::LEVEL)));
                    if (static_cast<double>(l) >= d0) {
                        flag = true;
                        vec3 = block->modifyAcceleration(this, mutablePos, entityIn, vec3);
                    }
                }
            }
        }
    }
    if (vec3.lengthVector() > 0.0 && entityIn->isPushedByWater()) {
        vec3 = vec3.normalize();
        const double d1 = 0.014;
        entityIn->motionX += vec3.xCoord * d1;
        entityIn->motionY += vec3.yCoord * d1;
        entityIn->motionZ += vec3.zCoord * d1;
    }
    return flag;
}

bool World::isMaterialInBB(const AxisAlignedBB& bb, const Material& materialIn)
{
    const int i = MathHelper::floor_double(bb.minX);
    const int j = MathHelper::floor_double(bb.maxX + 1.0);
    const int k = MathHelper::floor_double(bb.minY);
    const int l = MathHelper::floor_double(bb.maxY + 1.0);
    const int i1 = MathHelper::floor_double(bb.minZ);
    const int j1 = MathHelper::floor_double(bb.maxZ + 1.0);
    BlockPos mutablePos;
    for (int k1 = i; k1 < j; ++k1) {
        for (int l1 = k; l1 < l; ++l1) {
            for (int i2 = i1; i2 < j1; ++i2) {
                if (&getBlockState(mutablePos.set(k1, l1, i2))->getBlock()->getMaterial() == &materialIn)
                    return true;
            }
        }
    }
    return false;
}

bool World::isAABBInMaterial(const AxisAlignedBB& bb, const Material& materialIn)
{
    const int i = MathHelper::floor_double(bb.minX);
    const int j = MathHelper::floor_double(bb.maxX + 1.0);
    const int k = MathHelper::floor_double(bb.minY);
    const int l = MathHelper::floor_double(bb.maxY + 1.0);
    const int i1 = MathHelper::floor_double(bb.minZ);
    const int j1 = MathHelper::floor_double(bb.maxZ + 1.0);
    BlockPos mutablePos;
    for (int k1 = i; k1 < j; ++k1) {
        for (int l1 = k; l1 < l; ++l1) {
            for (int i2 = i1; i2 < j1; ++i2) {
                const IBlockState* iblockstate = getBlockState(mutablePos.set(k1, l1, i2));
                if (&iblockstate->getBlock()->getMaterial() == &materialIn) {
                    const int j2 = iblockstate->getValue(BlockLiquid::LEVEL);
                    double d0 = static_cast<double>(l1 + 1);
                    if (j2 < 8)
                        d0 = static_cast<double>(l1 + 1) - static_cast<double>(j2) / 8.0;
                    if (d0 >= bb.minY)
                        return true;
                }
            }
        }
    }
    return false;
}

std::vector<Entity*> World::getEntitiesWithinAABBExcludingEntity(const Entity* entityIn, const AxisAlignedBB& bb)
{
    return getEntitiesInAABBexcluding(entityIn, bb, notSpectating);
}

std::vector<Entity*> World::getEntitiesInAABBexcluding(const Entity* entityIn, const AxisAlignedBB& boundingBox, const std::function<bool(Entity*)>& predicate)
{
    std::vector<Entity*> list;
    const int i = MathHelper::floor_double((boundingBox.minX - 2.0) / 16.0);
    const int j = MathHelper::floor_double((boundingBox.maxX + 2.0) / 16.0);
    const int k = MathHelper::floor_double((boundingBox.minZ - 2.0) / 16.0);
    const int l = MathHelper::floor_double((boundingBox.maxZ + 2.0) / 16.0);
    for (int i1 = i; i1 <= j; ++i1) {
        for (int j1 = k; j1 <= l; ++j1) {
            if (isChunkLoaded(i1, j1, true))
                getChunkFromChunkCoords(i1, j1)->getEntitiesWithinAABBForEntity(entityIn, boundingBox, list, predicate);
        }
    }
    return list;
}

Entity* World::getEntityByID(int id)
{
    const auto it = entitiesById.find(id);
    return it == entitiesById.end() ? nullptr : it->second;
}

void World::loadEntities(const std::vector<std::shared_ptr<Entity>>& entityCollection)
{
    loadedEntityList.insert(loadedEntityList.end(), entityCollection.begin(), entityCollection.end());
    for (const auto& entity : entityCollection)
        onEntityAdded(entity.get());
}

void World::unloadEntities(const std::vector<std::shared_ptr<Entity>>& entityCollection)
{
    unloadedEntityList.insert(unloadedEntityList.end(), entityCollection.begin(), entityCollection.end());
}

bool World::canBlockBePlaced(Block* blockIn, const BlockPos& pos, bool skipCollisionCheck, EnumFacing side, Entity* entityIn, const ItemStack* itemStackIn)
{
    const Block* block = getBlockState(pos)->getBlock();
    const std::optional<AxisAlignedBB> axisalignedbb =
        skipCollisionCheck ? std::nullopt : blockIn->getCollisionBoundingBox(this, pos, blockIn->getDefaultState());
    if (axisalignedbb && !checkNoEntityCollision(*axisalignedbb, entityIn))
        return false;
    if (&block->getMaterial() == &Material::circuits && blockIn == Blocks::anvil)
        return true;
    return block->getMaterial().isReplaceable() && blockIn->canReplace(this, pos, side, itemStackIn);
}

EntityPlayer* World::getClosestPlayerToEntity(const Entity* entityIn, double distance)
{
    return getClosestPlayer(entityIn->posX, entityIn->posY, entityIn->posZ, distance);
}

EntityPlayer* World::getClosestPlayer(double x, double y, double z, double distance)
{
    double d0 = -1.0;
    EntityPlayer* entityplayer = nullptr;
    for (const auto& entityplayer1 : playerEntities) {
        if (notSpectating(entityplayer1.get())) {
            const double d1 = entityplayer1->getDistanceSq(x, y, z);
            if ((distance < 0.0 || d1 < distance * distance) && (d0 == -1.0 || d1 < d0)) {
                d0 = d1;
                entityplayer = entityplayer1.get();
            }
        }
    }
    return entityplayer;
}

bool World::isAnyPlayerWithinRangeAt(double x, double y, double z, double range)
{
    for (const auto& entityplayer : playerEntities) {
        if (notSpectating(entityplayer.get())) {
            const double d0 = entityplayer->getDistanceSq(x, y, z);
            if (range < 0.0 || d0 < range * range)
                return true;
        }
    }
    return false;
}

EntityPlayer* World::getPlayerEntityByName(const std::string& name)
{
    for (const auto& entityplayer : playerEntities) {
        if (name == entityplayer->getName())
            return entityplayer.get();
    }
    return nullptr;
}

EntityPlayer* World::getPlayerEntityByUUID(const UUID& uuid)
{
    for (const auto& entityplayer : playerEntities) {
        if (uuid == entityplayer->getUniqueID())
            return entityplayer.get();
    }
    return nullptr;
}

void World::joinEntityInSurroundings(const std::shared_ptr<Entity>& entityIn)
{
    const int i = MathHelper::floor_double(entityIn->posX / 16.0);
    const int j = MathHelper::floor_double(entityIn->posZ / 16.0);
    const int k = 2;
    for (int l = i - k; l <= i + k; ++l) {
        for (int i1 = j - k; i1 <= j + k; ++i1)
            getChunkFromChunkCoords(l, i1);
    }
    if (std::find(loadedEntityList.begin(), loadedEntityList.end(), entityIn) == loadedEntityList.end())
        loadedEntityList.push_back(entityIn);
}

} // namespace mc

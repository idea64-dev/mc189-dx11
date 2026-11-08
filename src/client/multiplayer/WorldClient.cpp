#include "client/multiplayer/WorldClient.h"

#include "block/Block.h"
#include "client/Minecraft.h"
#include "client/audio/Sound.h"
#include "client/audio/SoundHandler.h"
#include "client/multiplayer/ChunkProviderClient.h"
#include "client/multiplayer/PlayerControllerMP.h"
#include "client/network/NetHandlerPlayClient.h"
#include "client/settings/GameSettings.h"
#include "core/Java.h"
#include "entity/player/EntityPlayerSP.h"
#include "init/Blocks.h"
#include "item/ItemStack.h"
#include "network/NetworkManager.h"
#include "util/ChatComponent.h"
#include "util/EnumParticleTypes.h"
#include "world/WorldProvider.h"
#include "world/WorldSettings.h"
#include "world/chunk/Chunk.h"
#include "world/storage/MapStorage.h"
#include "world/storage/WorldInfo.h"

#include <cmath>
#include <vector>

namespace mc {

namespace {

// A non-owning pointer for set lookups.
template <class K>
K keyOf(Entity* entity)
{
    return K { entity };
}

} // namespace

WorldClient::WorldClient(NetHandlerPlayClient* netHandler, const WorldSettings& settings, int dimension, EnumDifficulty difficulty, Profiler& profilerIn)
    // Java: SaveHandlerMP, which saves nothing -- no save handler here.
    : World(nullptr, std::make_shared<WorldInfo>(settings, "MpServer"), WorldProvider::getProviderForDimension(dimension), profilerIn, true)
    , sendQueue(netHandler)
    , mc(Minecraft::getMinecraft())
{
    getWorldInfo().setDifficulty(difficulty);
    setSpawnPoint(BlockPos(8, 64, 8));
    provider->registerWorld(this);
    chunkProvider = createChunkProvider();
    // Java: SaveDataMemoryStorage
    mapStorage = std::make_shared<MapStorage>(nullptr);
    calculateInitialSkylight();
    calculateInitialWeather();
}

WorldClient::~WorldClient() = default;

void WorldClient::tick()
{
    World::tick();
    setTotalWorldTime(getTotalWorldTime() + 1);

    if (getGameRules().getBoolean("doDaylightCycle"))
        setWorldTime(getWorldTime() + 1);

    theProfiler.startSection("reEntryProcessing");

    for (int i = 0; i < 10 && !entitySpawnQueue.empty(); ++i) {
        const std::shared_ptr<Entity> entity = entitySpawnQueue.begin()->second;
        entitySpawnQueue.erase(keyOf<EntityKey>(entity.get()));

        bool loaded = false;
        for (const auto& e : loadedEntityList) {
            if (e.get() == entity.get()) {
                loaded = true;
                break;
            }
        }
        if (!loaded)
            spawnEntityInWorld(entity);
    }

    theProfiler.endStartSection("chunkCache");
    clientChunkProvider->unloadQueuedChunks();
    theProfiler.endStartSection("blocks");
    updateBlocks();
    theProfiler.endSection();
}

std::unique_ptr<IChunkProvider> WorldClient::createChunkProvider()
{
    auto provider = std::make_unique<ChunkProviderClient>(this);
    clientChunkProvider = provider.get();
    return provider;
}

void WorldClient::updateBlocks()
{
    World::updateBlocks();

    // Java: previousActiveChunkSet.retainAll(activeChunkSet)
    std::vector<ChunkCoordIntPair> stale;
    for (const auto& entry : previousActiveChunkSet) {
        if (!activeChunkSet.contains(entry.first))
            stale.push_back(entry.first);
    }
    for (const ChunkCoordIntPair& pair : stale)
        previousActiveChunkSet.erase(pair);

    if (previousActiveChunkSet.size() == activeChunkSet.size())
        previousActiveChunkSet.clear();

    int i = 0;

    for (const auto& entry : activeChunkSet) {
        const ChunkCoordIntPair& chunkcoordintpair = entry.first;
        if (!previousActiveChunkSet.contains(chunkcoordintpair)) {
            const int j = chunkcoordintpair.chunkXPos * 16;
            const int k = chunkcoordintpair.chunkZPos * 16;
            theProfiler.startSection("getChunk");
            Chunk* chunk = getChunkFromChunkCoords(chunkcoordintpair.chunkXPos, chunkcoordintpair.chunkZPos);
            playMoodSoundAndCheckLight(j, k, chunk);
            theProfiler.endSection();
            previousActiveChunkSet.put(chunkcoordintpair, true);
            ++i;

            if (i >= 10)
                return;
        }
    }
}

void WorldClient::doPreChunk(int chuncX, int chuncZ, bool loadChunk)
{
    if (loadChunk)
        clientChunkProvider->loadChunk(chuncX, chuncZ);
    else
        clientChunkProvider->unloadChunk(chuncX, chuncZ);

    if (!loadChunk)
        markBlockRangeForRenderUpdate(chuncX * 16, 0, chuncZ * 16, chuncX * 16 + 15, 256, chuncZ * 16 + 15);
}

bool WorldClient::spawnEntityInWorld(const std::shared_ptr<Entity>& entityIn)
{
    const bool flag = World::spawnEntityInWorld(entityIn);
    entityList.put(keyOf<EntityKey>(entityIn.get()), entityIn);

    if (!flag)
        entitySpawnQueue.put(keyOf<EntityKey>(entityIn.get()), entityIn);
    // Java: else if an EntityMinecart, play MovingSoundMinecart -- with the minecarts (stage 10).

    return flag;
}

void WorldClient::removeEntity(Entity* entityIn)
{
    // Keep the entity alive while it leaves the sets.
    const std::shared_ptr<Entity> keep = entityIn->shared_from_this();
    World::removeEntity(entityIn);
    entityList.erase(keyOf<EntityKey>(entityIn));
}

void WorldClient::onEntityAdded(Entity* entityIn)
{
    World::onEntityAdded(entityIn);

    if (entitySpawnQueue.contains(keyOf<EntityKey>(entityIn)))
        entitySpawnQueue.erase(keyOf<EntityKey>(entityIn));
}

void WorldClient::onEntityRemoved(Entity* entityIn)
{
    World::onEntityRemoved(entityIn);

    if (std::shared_ptr<Entity>* listed = entityList.find(keyOf<EntityKey>(entityIn))) {
        if (entityIn->isEntityAlive()) {
            entitySpawnQueue.put(keyOf<EntityKey>(entityIn), *listed);
        } else {
            entityList.erase(keyOf<EntityKey>(entityIn));
        }
    }
}

void WorldClient::addEntityToWorld(int entityID, const std::shared_ptr<Entity>& entityToSpawn)
{
    if (Entity* entity = getEntityByID(entityID))
        removeEntity(entity);

    entityList.put(keyOf<EntityKey>(entityToSpawn.get()), entityToSpawn);
    entityToSpawn->setEntityId(entityID);

    if (!spawnEntityInWorld(entityToSpawn))
        entitySpawnQueue.put(keyOf<EntityKey>(entityToSpawn.get()), entityToSpawn);

    entitiesById[entityID] = entityToSpawn.get();
}

Entity* WorldClient::getEntityByID(int id)
{
    if (mc->thePlayer && id == mc->thePlayer->getEntityId())
        return mc->thePlayer.get();
    return World::getEntityByID(id);
}

std::shared_ptr<Entity> WorldClient::removeEntityFromWorld(int entityID)
{
    const auto it = entitiesById.find(entityID);
    if (it == entitiesById.end())
        return nullptr;
    Entity* entity = it->second;
    entitiesById.erase(it);
    if (entity == nullptr)
        return nullptr;

    std::shared_ptr<Entity> keep = entity->shared_from_this();
    entityList.erase(keyOf<EntityKey>(entity));
    removeEntity(entity);
    return keep;
}

bool WorldClient::invalidateRegionAndSetBlock(const BlockPos& pos, const IBlockState* state)
{
    const int i = pos.getX();
    const int j = pos.getY();
    const int k = pos.getZ();
    invalidateBlockReceiveRegion(i, j, k, i, j, k);
    return World::setBlockState(pos, state, 3);
}

void WorldClient::sendQuittingDisconnectingPacket()
{
    sendQueue->getNetworkManager()->closeChannel(std::make_shared<ChatComponentText>("Quitting"));
}

int WorldClient::getRenderDistanceChunks()
{
    return mc->gameSettings->renderDistanceChunks;
}

void WorldClient::doVoidFogParticles(int posX, int posY, int posZ)
{
    const int i = 16;
    Random random;
    const std::shared_ptr<ItemStack> itemstack = mc->thePlayer->getHeldItem();
    const bool flag = mc->playerController->getCurrentGameType() == GameType::CREATIVE && itemstack != nullptr
        && Block::getBlockFromItem(itemstack->getItem()) == Blocks::barrier;

    for (int j = 0; j < 1000; ++j) {
        // Java evaluates the draws left to right.
        int k = posX + rand.nextInt(i);
        k -= rand.nextInt(i);
        int l = posY + rand.nextInt(i);
        l -= rand.nextInt(i);
        int i1 = posZ + rand.nextInt(i);
        i1 -= rand.nextInt(i);
        const BlockPos blockpos(k, l, i1);
        const IBlockState* iblockstate = getBlockState(blockpos);
        iblockstate->getBlock()->randomDisplayTick(this, blockpos, iblockstate, random);

        if (flag && iblockstate->getBlock() == Blocks::barrier) {
            spawnParticle(getParticleID(EnumParticleTypes::BARRIER), false, static_cast<double>(static_cast<float>(k) + 0.5f),
                static_cast<double>(static_cast<float>(l) + 0.5f), static_cast<double>(static_cast<float>(i1) + 0.5f), 0.0, 0.0, 0.0);
        }
    }
}

void WorldClient::removeAllEntities()
{
    // Java: loadedEntityList.removeAll(unloadedEntityList)
    std::erase_if(loadedEntityList, [this](const std::shared_ptr<Entity>& e) {
        for (const auto& u : unloadedEntityList) {
            if (u.get() == e.get())
                return true;
        }
        return false;
    });

    for (size_t i = 0; i < unloadedEntityList.size(); ++i) {
        Entity* entity = unloadedEntityList[i].get();
        const int j = entity->chunkCoordX;
        const int k = entity->chunkCoordZ;

        if (entity->addedToChunk && isChunkLoaded(j, k, true))
            getChunkFromChunkCoords(j, k)->removeEntity(entity);
    }

    const std::vector<std::shared_ptr<Entity>> unloaded = std::move(unloadedEntityList);
    unloadedEntityList.clear();
    for (const auto& entity : unloaded)
        onEntityRemoved(entity.get());

    for (size_t i1 = 0; i1 < loadedEntityList.size(); ++i1) {
        const std::shared_ptr<Entity> entity1 = loadedEntityList[i1];

        if (entity1->ridingEntity) {
            if (!entity1->ridingEntity->isDead && entity1->ridingEntity->riddenByEntity.get() == entity1.get())
                continue;

            entity1->ridingEntity->riddenByEntity = nullptr;
            entity1->ridingEntity = nullptr;
        }

        if (entity1->isDead) {
            const int j1 = entity1->chunkCoordX;
            const int k1 = entity1->chunkCoordZ;

            if (entity1->addedToChunk && isChunkLoaded(j1, k1, true))
                getChunkFromChunkCoords(j1, k1)->removeEntity(entity1.get());

            loadedEntityList.erase(loadedEntityList.begin() + static_cast<std::ptrdiff_t>(i1--));
            onEntityRemoved(entity1.get());
        }
    }
}

void WorldClient::playSoundAtPos(const BlockPos& pos, const std::string& soundName, float volume, float pitch, bool distanceDelay)
{
    playSound(static_cast<double>(pos.getX()) + 0.5, static_cast<double>(pos.getY()) + 0.5, static_cast<double>(pos.getZ()) + 0.5, soundName, volume, pitch,
        distanceDelay);
}

void WorldClient::playSound(double x, double y, double z, const std::string& soundName, float volume, float pitch, bool distanceDelay)
{
    const double d0 = mc->getRenderViewEntity()->getDistanceSq(x, y, z);
    auto positionedsoundrecord = std::make_shared<PositionedSoundRecord>(
        ResourceLocation(soundName), volume, pitch, static_cast<float>(x), static_cast<float>(y), static_cast<float>(z));

    if (distanceDelay && d0 > 100.0) {
        const double d1 = std::sqrt(d0) / 40.0;
        mc->getSoundHandler().playDelayedSound(positionedsoundrecord, java::d2i(d1 * 20.0));
    } else {
        mc->getSoundHandler().playSound(positionedsoundrecord);
    }
}

void WorldClient::setWorldTime(int64_t time)
{
    if (time < 0) {
        time = -time;
        getGameRules().setOrCreateGameRule("doDaylightCycle", "false");
    } else {
        getGameRules().setOrCreateGameRule("doDaylightCycle", "true");
    }

    World::setWorldTime(time);
}

} // namespace mc

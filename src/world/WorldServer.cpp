#include "world/WorldServer.h"

#include "block/AllBlocks.h"
#include "core/Log.h"
#include "entity/EntityLivingBase.h"
#include "entity/EntityTracker.h"
#include "entity/player/EntityPlayer.h"
#include "entity/player/EntityPlayerMP.h"
#include "init/Blocks.h"
#include "init/Items.h"
#include "item/Item.h"
#include "network/NetHandlerPlayServer.h"
#include "network/play/ServerPlayPackets.h"
#include "server/MinecraftServer.h"
#include "server/management/PlayerManager.h"
#include "server/management/ServerConfigurationManager.h"
#include "tileentity/TileEntity.h"
#include "util/Vec3.h"
#include "util/IProgressUpdate.h"
#include "util/MathHelper.h"
#include "util/WeightedRandomChestContent.h"
#include "world/GameRules.h"
#include "world/WorldProvider.h"
#include "world/WorldSettings.h"
#include "world/WorldType.h"
#include "world/biome/BiomeGenBase.h"
#include "world/biome/WorldChunkManager.h"
#include "world/border/WorldBorder.h"
#include "world/chunk/Chunk.h"
#include "world/chunk/storage/ExtendedBlockStorage.h"
#include "world/gen/ChunkProviderServer.h"
#include "world/gen/feature/WorldGenerators.h"
#include "world/storage/ISaveHandler.h"
#include "world/storage/MapStorage.h"
#include "world/storage/WorldInfo.h"

#include <algorithm>

namespace mc {

namespace {

const std::vector<WeightedRandomChestContent>& bonusChestContent()
{
    static const std::vector<WeightedRandomChestContent> content {
        WeightedRandomChestContent(Items::stick, 0, 1, 3, 10),
        WeightedRandomChestContent(Item::getItemFromBlock(Blocks::planks), 0, 1, 3, 10),
        WeightedRandomChestContent(Item::getItemFromBlock(Blocks::log), 0, 1, 3, 10),
        WeightedRandomChestContent(Items::stone_axe, 0, 1, 1, 3),
        WeightedRandomChestContent(Items::wooden_axe, 0, 1, 1, 5),
        WeightedRandomChestContent(Items::stone_pickaxe, 0, 1, 1, 3),
        WeightedRandomChestContent(Items::wooden_pickaxe, 0, 1, 1, 5),
        WeightedRandomChestContent(Items::apple, 0, 2, 3, 5),
        WeightedRandomChestContent(Items::bread, 0, 2, 3, 3),
        WeightedRandomChestContent(Item::getItemFromBlock(Blocks::log2), 0, 1, 3, 10),
    };
    return content;
}

double soundRange(float volume)
{
    return volume > 1.0f ? static_cast<double>(16.0f * volume) : 16.0;
}

} // namespace

void WorldManager::onEntityAdded(Entity* entityIn)
{
    theWorldServer->getEntityTracker()->trackEntity(entityIn);
}

void WorldManager::onEntityRemoved(Entity* entityIn)
{
    theWorldServer->getEntityTracker()->untrackEntity(entityIn);
    // Java: getScoreboard().func_181140_a(entityIn) -- the scoreboard, stage 12.
}

void WorldManager::playSound(const std::string& soundName, double x, double y, double z, float volume, float pitch)
{
    mcServer->getConfigurationManager()->sendToAllNear(
        x, y, z, soundRange(volume), theWorldServer->provider->getDimensionId(), std::make_shared<S29PacketSoundEffect>(soundName, x, y, z, volume, pitch));
}

void WorldManager::playSoundToNearExcept(EntityPlayer* except, const std::string& soundName, double x, double y, double z, float volume, float pitch)
{
    mcServer->getConfigurationManager()->sendToAllNearExcept(except, x, y, z, soundRange(volume), theWorldServer->provider->getDimensionId(),
        std::make_shared<S29PacketSoundEffect>(soundName, x, y, z, volume, pitch));
}

void WorldManager::markBlockForUpdate(const BlockPos& pos)
{
    theWorldServer->getPlayerManager()->markBlockForUpdate(pos);
}

void WorldManager::playAuxSFX(EntityPlayer* player, int sfxType, const BlockPos& blockPosIn, int data)
{
    mcServer->getConfigurationManager()->sendToAllNearExcept(player, blockPosIn.getX(), blockPosIn.getY(), blockPosIn.getZ(), 64.0,
        theWorldServer->provider->getDimensionId(), std::make_shared<S28PacketEffect>(sfxType, blockPosIn, data, false));
}

void WorldManager::broadcastSound(int soundID, const BlockPos& pos, int data)
{
    mcServer->getConfigurationManager()->sendPacketToAllPlayers(std::make_shared<S28PacketEffect>(soundID, pos, data, true));
}

void WorldManager::sendBlockBreakProgress(int breakerId, const BlockPos& pos, int progress)
{
    for (const std::shared_ptr<EntityPlayerMP>& entityplayermp : mcServer->getConfigurationManager()->getPlayerList()) {
        if (entityplayermp != nullptr && entityplayermp->worldObj == theWorldServer && entityplayermp->getEntityId() != breakerId) {
            const double d0 = pos.getX() - entityplayermp->posX;
            const double d1 = pos.getY() - entityplayermp->posY;
            const double d2 = pos.getZ() - entityplayermp->posZ;
            if (d0 * d0 + d1 * d1 + d2 * d2 < 1024.0)
                entityplayermp->playerNetServerHandler->sendPacket(std::make_shared<S25PacketBlockBreakAnim>(breakerId, pos, progress));
        }
    }
}

WorldServer::WorldServer(MinecraftServer* server, std::shared_ptr<ISaveHandler> saveHandlerIn, std::shared_ptr<WorldInfo> info, int dimensionId, Profiler& profilerIn)
    : World(std::move(saveHandlerIn), std::move(info), WorldProvider::getProviderForDimension(dimensionId), profilerIn, false)
    , mcServer(server)
{
    theEntityTracker = std::make_unique<EntityTracker>(this);
    thePlayerManager = std::make_unique<PlayerManager>(this);
    provider->registerWorld(this);
    chunkProvider = createChunkProvider();
    // Java: worldTeleporter = new Teleporter(this) -- its Random(seed) draws nothing yet (stage 12).
    calculateInitialSkylight();
    calculateInitialWeather();
    getWorldBorder().setSize(server->getMaxWorldSize());
}

WorldServer::~WorldServer() = default;

void WorldServer::setWorldManager(std::unique_ptr<WorldManager> manager)
{
    worldManager = std::move(manager);
    addWorldAccess(worldManager.get());
}

World* WorldServer::init()
{
    mapStorage = std::make_shared<MapStorage>(saveHandler.get());
    // Java: the village collection and the scoreboard load from the map storage -- stages 10 and 12.
    WorldBorder& border = getWorldBorder();
    border.setCenter(worldInfo->getBorderCenterX(), worldInfo->getBorderCenterZ());
    border.setDamageAmount(worldInfo->getBorderDamagePerBlock());
    border.setDamageBuffer(worldInfo->getBorderSafeZone());
    border.setWarningDistance(worldInfo->getBorderWarningDistance());
    border.setWarningTime(worldInfo->getBorderWarningTime());

    if (worldInfo->getBorderLerpTime() > 0)
        border.setTransition(worldInfo->getBorderSize(), worldInfo->getBorderLerpTarget(), worldInfo->getBorderLerpTime());
    else
        border.setTransition(worldInfo->getBorderSize());

    return this;
}

void WorldServer::tick()
{
    World::tick();

    if (getWorldInfo().isHardcoreModeEnabled() && getDifficulty() != EnumDifficulty::HARD)
        getWorldInfo().setDifficulty(EnumDifficulty::HARD);

    provider->getWorldChunkManager()->cleanupCache();

    if (areAllPlayersAsleep()) {
        if (getGameRules().getBoolean("doDaylightCycle")) {
            const int64_t i = worldInfo->getWorldTime() + 24000;
            worldInfo->setWorldTime(i - i % 24000);
        }
        wakeAllPlayers();
    }

    theProfiler.startSection("mobSpawner");
    // Java: mobSpawner.findChunksForSpawning when doMobSpawning is on -- the mobs, stage 10.
    theProfiler.endStartSection("chunkSource");
    chunkProvider->unloadQueuedChunks();
    const int j = calculateSkylightSubtracted(1.0f);
    if (j != getSkylightSubtracted())
        setSkylightSubtracted(j);

    worldInfo->setWorldTotalTime(worldInfo->getWorldTotalTime() + 1);
    if (getGameRules().getBoolean("doDaylightCycle"))
        worldInfo->setWorldTime(worldInfo->getWorldTime() + 1);

    theProfiler.endStartSection("tickPending");
    tickUpdates(false);
    theProfiler.endStartSection("tickBlocks");
    updateBlocks();
    theProfiler.endStartSection("chunkMap");
    thePlayerManager->updatePlayerInstances();
    theProfiler.endStartSection("village");
    // Java: villageCollectionObj.tick() -- no villages exist before the villagers (stage 10).
    villageSiege.tick(*this);
    theProfiler.endStartSection("portalForcer");
    // Java: worldTeleporter.removeStalePortalLocations -- portals, stage 12.
    theProfiler.endSection();
    sendQueuedBlockEvents();
}

void WorldServer::VillageSiege::tick(World& world)
{
    if (world.isDaytime()) {
        state = 0;
    } else if (state != 2) {
        if (state == 0) {
            const float f = world.getCelestialAngle(0.0f);
            if (static_cast<double>(f) < 0.5 || static_cast<double>(f) > 0.501)
                return;
            state = world.rand.nextInt(10) == 0 ? 1 : 2;
            started = false;
            if (state == 2)
                return;
        }

        if (state != -1 && !started) {
            // Java: func_75529_b looks for a village of 10+ doors and 20+ villagers near a player; there are none.
            return;
        }
    }
}

void WorldServer::updateAllPlayersSleepingFlag()
{
    allPlayersSleeping = false;

    if (!playerEntities.empty()) {
        int i = 0;
        int j = 0;
        for (const std::shared_ptr<EntityPlayer>& entityplayer : playerEntities) {
            if (entityplayer->isSpectator())
                ++i;
            else if (entityplayer->isPlayerSleeping())
                ++j;
        }
        allPlayersSleeping = j > 0 && j >= static_cast<int>(playerEntities.size()) - i;
    }
}

void WorldServer::wakeAllPlayers()
{
    allPlayersSleeping = false;
    for (const std::shared_ptr<EntityPlayer>& entityplayer : playerEntities) {
        if (entityplayer->isPlayerSleeping())
            entityplayer->wakeUpPlayer(false, false, true);
    }
    resetRainAndThunder();
}

void WorldServer::resetRainAndThunder()
{
    worldInfo->setRainTime(0);
    worldInfo->setRaining(false);
    worldInfo->setThunderTime(0);
    worldInfo->setThundering(false);
}

bool WorldServer::areAllPlayersAsleep()
{
    if (allPlayersSleeping && !isRemote) {
        for (const std::shared_ptr<EntityPlayer>& entityplayer : playerEntities) {
            if (entityplayer->isSpectator() || !entityplayer->isPlayerFullyAsleep())
                return false;
        }
        return true;
    }
    return false;
}

void WorldServer::setInitialSpawnLocation()
{
    if (worldInfo->getSpawnY() <= 0)
        worldInfo->setSpawnY(getSeaLevel() + 1);

    int i = worldInfo->getSpawnX();
    int j = worldInfo->getSpawnZ();
    int k = 0;

    while (&getGroundAboveSeaLevel(BlockPos(i, 0, j))->getMaterial() == &Material::air) {
        const int di = rand.nextInt(8);
        i += di - rand.nextInt(8);
        const int dj = rand.nextInt(8);
        j += dj - rand.nextInt(8);
        ++k;
        if (k == 10000)
            break;
    }

    worldInfo->setSpawnX(i);
    worldInfo->setSpawnZ(j);
}

void WorldServer::updateBlocks()
{
    World::updateBlocks();

    if (worldInfo->getTerrainType() == &WorldType::DEBUG_WORLD) {
        for (const auto& entry : activeChunkSet)
            getChunkFromChunkCoords(entry.first.chunkXPos, entry.first.chunkZPos)->func_150804_b(false);
        return;
    }

    for (const auto& entry : activeChunkSet) {
        const ChunkCoordIntPair& chunkcoordintpair = entry.first;
        const int k = chunkcoordintpair.chunkXPos * 16;
        const int l = chunkcoordintpair.chunkZPos * 16;
        theProfiler.startSection("getChunk");
        Chunk* chunk = getChunkFromChunkCoords(chunkcoordintpair.chunkXPos, chunkcoordintpair.chunkZPos);
        playMoodSoundAndCheckLight(k, l, chunk);
        theProfiler.endStartSection("tickChunk");
        chunk->func_150804_b(false);
        theProfiler.endStartSection("thunder");

        if (rand.nextInt(100000) == 0 && isRaining() && isThundering()) {
            updateLCG = java::add(java::mul(updateLCG, 3), 1013904223);
            const int i1 = updateLCG >> 2;
            const BlockPos blockpos = adjustPosToNearbyEntity(BlockPos(k + (i1 & 15), 0, l + (i1 >> 8 & 15)));
            if (isRainingAt(blockpos)) {
                // Java: addWeatherEffect(new EntityLightningBolt(...)) -- the lightning bolt, stage 10.
            }
        }

        theProfiler.endStartSection("iceandsnow");

        if (rand.nextInt(16) == 0) {
            updateLCG = java::add(java::mul(updateLCG, 3), 1013904223);
            const int k2 = updateLCG >> 2;
            const BlockPos blockpos2 = getPrecipitationHeight(BlockPos(k + (k2 & 15), 0, l + (k2 >> 8 & 15)));
            const BlockPos blockpos1 = blockpos2.down();

            if (canBlockFreezeNoWater(blockpos1))
                setBlockState(blockpos1, Blocks::ice->getDefaultState());

            if (isRaining() && canSnowAt(blockpos2, true))
                setBlockState(blockpos2, Blocks::snow_layer->getDefaultState());

            if (isRaining() && getBiomeGenForCoords(blockpos1)->canRain())
                getBlockState(blockpos1)->getBlock()->fillWithRain(this, blockpos1);
        }

        theProfiler.endStartSection("tickBlocks");
        const int l2 = getGameRules().getInt("randomTickSpeed");

        if (l2 > 0) {
            for (const std::unique_ptr<ExtendedBlockStorage>& extendedblockstorage : chunk->getBlockStorageArray()) {
                if (extendedblockstorage && extendedblockstorage->getNeedsRandomTick()) {
                    for (int j1 = 0; j1 < l2; ++j1) {
                        updateLCG = java::add(java::mul(updateLCG, 3), 1013904223);
                        const int k1 = updateLCG >> 2;
                        const int l1 = k1 & 15;
                        const int i2 = k1 >> 8 & 15;
                        const int j2 = k1 >> 16 & 15;
                        const IBlockState* iblockstate = extendedblockstorage->get(l1, j2, i2);
                        Block* block = iblockstate->getBlock();
                        if (block->getTickRandomly())
                            block->randomTick(this, BlockPos(l1 + k, j2 + extendedblockstorage->getYLocation(), i2 + l), iblockstate, rand);
                    }
                }
            }
        }

        theProfiler.endSection();
    }
}

BlockPos WorldServer::adjustPosToNearbyEntity(const BlockPos& pos)
{
    const BlockPos blockpos = getPrecipitationHeight(pos);
    const AxisAlignedBB axisalignedbb = AxisAlignedBB(blockpos, BlockPos(blockpos.getX(), getHeight(), blockpos.getZ())).expand(3.0, 3.0, 3.0);
    const std::vector<EntityLivingBase*> list = getEntitiesWithinAABB<EntityLivingBase>(axisalignedbb,
        [this](EntityLivingBase* entity) { return entity != nullptr && entity->isEntityAlive() && canSeeSky(entity->getPosition()); });
    return !list.empty() ? list[static_cast<size_t>(rand.nextInt(static_cast<int>(list.size())))]->getPosition() : blockpos;
}

bool WorldServer::isBlockTickPending(const BlockPos& pos, Block* blockType)
{
    const NextTickListEntry nextticklistentry(pos, blockType);
    return std::any_of(pendingTickListEntriesThisTick.begin(), pendingTickListEntriesThisTick.end(),
        [&](const NextTickListEntry& entry) { return entry.equals(nextticklistentry); });
}

void WorldServer::updateBlockTick(const BlockPos& pos, Block* blockIn, int delay, int priority)
{
    NextTickListEntry nextticklistentry(pos, blockIn);
    int i = 0;

    if (scheduledUpdatesAreImmediate && &blockIn->getMaterial() != &Material::air) {
        if (blockIn->requiresUpdates()) {
            i = 8;
            if (isAreaLoaded(nextticklistentry.position.add(-i, -i, -i), nextticklistentry.position.add(i, i, i))) {
                const IBlockState* iblockstate = getBlockState(nextticklistentry.position);
                if (&iblockstate->getBlock()->getMaterial() != &Material::air && iblockstate->getBlock() == nextticklistentry.getBlock())
                    iblockstate->getBlock()->updateTick(this, nextticklistentry.position, iblockstate, rand);
            }
            return;
        }
        delay = 1;
    }

    if (isAreaLoaded(pos.add(-i, -i, -i), pos.add(i, i, i))) {
        if (&blockIn->getMaterial() != &Material::air) {
            nextticklistentry.setScheduledTime(static_cast<int64_t>(delay) + worldInfo->getWorldTotalTime());
            nextticklistentry.setPriority(priority);
        }

        if (!pendingTickListEntriesHashSet.contains(nextticklistentry)) {
            pendingTickListEntriesHashSet.insert(nextticklistentry);
            pendingTickListEntriesTreeSet.insert(nextticklistentry);
        }
    }
}

void WorldServer::scheduleBlockUpdate(const BlockPos& pos, Block* blockIn, int delay, int priority)
{
    NextTickListEntry nextticklistentry(pos, blockIn);
    nextticklistentry.setPriority(priority);

    if (&blockIn->getMaterial() != &Material::air)
        nextticklistentry.setScheduledTime(static_cast<int64_t>(delay) + worldInfo->getWorldTotalTime());

    if (!pendingTickListEntriesHashSet.contains(nextticklistentry)) {
        pendingTickListEntriesHashSet.insert(nextticklistentry);
        pendingTickListEntriesTreeSet.insert(nextticklistentry);
    }
}

void WorldServer::updateEntities()
{
    if (playerEntities.empty()) {
        if (updateEntityTick++ >= 1200)
            return;
    } else {
        resetUpdateEntityTick();
    }

    World::updateEntities();
}

bool WorldServer::tickUpdates(bool runAllPending)
{
    if (worldInfo->getTerrainType() == &WorldType::DEBUG_WORLD)
        return false;

    int i = static_cast<int>(pendingTickListEntriesTreeSet.size());
    if (i != static_cast<int>(pendingTickListEntriesHashSet.size()))
        throw std::logic_error("TickNextTick list out of synch");

    if (i > 1000)
        i = 1000;

    theProfiler.startSection("cleaning");

    for (int j = 0; j < i; ++j) {
        const NextTickListEntry nextticklistentry = *pendingTickListEntriesTreeSet.begin();
        if (!runAllPending && nextticklistentry.scheduledTime > worldInfo->getWorldTotalTime())
            break;
        pendingTickListEntriesTreeSet.erase(pendingTickListEntriesTreeSet.begin());
        pendingTickListEntriesHashSet.erase(nextticklistentry);
        pendingTickListEntriesThisTick.push_back(nextticklistentry);
    }

    theProfiler.endSection();
    theProfiler.startSection("ticking");

    // Java removes each entry through its iterator before ticking it; ticks scheduled meanwhile go to the sets.
    while (!pendingTickListEntriesThisTick.empty()) {
        const NextTickListEntry nextticklistentry1 = pendingTickListEntriesThisTick.front();
        pendingTickListEntriesThisTick.erase(pendingTickListEntriesThisTick.begin());
        const int k = 0;

        if (isAreaLoaded(nextticklistentry1.position.add(-k, -k, -k), nextticklistentry1.position.add(k, k, k))) {
            const IBlockState* iblockstate = getBlockState(nextticklistentry1.position);
            if (&iblockstate->getBlock()->getMaterial() != &Material::air && Block::isEqualTo(iblockstate->getBlock(), nextticklistentry1.getBlock()))
                iblockstate->getBlock()->updateTick(this, nextticklistentry1.position, iblockstate, rand);
        } else {
            scheduleUpdate(nextticklistentry1.position, nextticklistentry1.getBlock(), 0);
        }
    }

    theProfiler.endSection();
    pendingTickListEntriesThisTick.clear();
    return !pendingTickListEntriesTreeSet.empty();
}

std::vector<NextTickListEntry> WorldServer::getPendingBlockUpdates(Chunk* chunkIn, bool remove)
{
    const ChunkCoordIntPair chunkcoordintpair = chunkIn->getChunkCoordIntPair();
    const int i = (chunkcoordintpair.chunkXPos << 4) - 2;
    const int j = i + 16 + 2;
    const int k = (chunkcoordintpair.chunkZPos << 4) - 2;
    const int l = k + 16 + 2;
    return func_175712_a(i, k, j, l, remove);
}

std::vector<NextTickListEntry> WorldServer::func_175712_a(int minX, int minZ, int maxX, int maxZ, bool remove)
{
    std::vector<NextTickListEntry> list;
    const auto inside = [&](const NextTickListEntry& entry) {
        const BlockPos& blockpos = entry.position;
        return blockpos.getX() >= minX && blockpos.getX() < maxX && blockpos.getZ() >= minZ && blockpos.getZ() < maxZ;
    };

    for (auto it = pendingTickListEntriesTreeSet.begin(); it != pendingTickListEntriesTreeSet.end();) {
        if (inside(*it)) {
            list.push_back(*it);
            if (remove) {
                pendingTickListEntriesHashSet.erase(*it);
                it = pendingTickListEntriesTreeSet.erase(it);
                continue;
            }
        }
        ++it;
    }

    for (auto it = pendingTickListEntriesThisTick.begin(); it != pendingTickListEntriesThisTick.end();) {
        if (inside(*it)) {
            list.push_back(*it);
            if (remove) {
                pendingTickListEntriesHashSet.erase(*it);
                it = pendingTickListEntriesThisTick.erase(it);
                continue;
            }
        }
        ++it;
    }

    return list;
}

void WorldServer::updateEntityWithOptionalForce(Entity* entityIn, bool forceUpdate)
{
    // Java: animals, water mobs and NPCs die when the server forbids them -- with those classes (stage 10).
    World::updateEntityWithOptionalForce(entityIn, forceUpdate);
}

bool WorldServer::canSpawnNPCs() const
{
    return mcServer->getCanSpawnNPCs();
}

bool WorldServer::canSpawnAnimals() const
{
    return mcServer->getCanSpawnAnimals();
}

std::unique_ptr<IChunkProvider> WorldServer::createChunkProvider()
{
    std::unique_ptr<IChunkLoader> ichunkloader = saveHandler->getChunkLoader(*provider);
    auto provider = std::make_unique<ChunkProviderServer>(this, std::move(ichunkloader), this->provider->createChunkGenerator());
    theChunkProviderServer = provider.get();
    return provider;
}

std::vector<TileEntity*> WorldServer::getTileEntitiesIn(int minX, int minY, int minZ, int maxX, int maxY, int maxZ)
{
    std::vector<TileEntity*> list;
    for (const std::shared_ptr<TileEntity>& tileentity : loadedTileEntityList) {
        const BlockPos& blockpos = tileentity->getPos();
        if (blockpos.getX() >= minX && blockpos.getY() >= minY && blockpos.getZ() >= minZ && blockpos.getX() < maxX && blockpos.getY() < maxY
            && blockpos.getZ() < maxZ)
            list.push_back(tileentity.get());
    }
    return list;
}

bool WorldServer::isBlockModifiable(EntityPlayer* player, const BlockPos& pos)
{
    return !mcServer->isBlockProtected(this, pos, player) && getWorldBorder().contains(pos);
}

void WorldServer::initialize(const WorldSettings& settings)
{
    if (!worldInfo->isInitialized()) {
        createSpawnPosition(settings);
        if (worldInfo->getTerrainType() == &WorldType::DEBUG_WORLD)
            setDebugWorldSettings();
        World::initialize(settings);
        worldInfo->setServerInitialized(true);
    }
}

void WorldServer::setDebugWorldSettings()
{
    worldInfo->setMapFeaturesEnabled(false);
    worldInfo->setAllowCommands(true);
    worldInfo->setRaining(false);
    worldInfo->setThundering(false);
    worldInfo->setCleanWeatherTime(1000000000);
    worldInfo->setWorldTime(6000);
    worldInfo->setGameType(GameType::SPECTATOR);
    worldInfo->setHardcore(false);
    worldInfo->setDifficulty(EnumDifficulty::PEACEFUL);
    worldInfo->setDifficultyLocked(true);
    getGameRules().setOrCreateGameRule("doDaylightCycle", "false");
}

void WorldServer::createSpawnPosition(const WorldSettings& settings)
{
    if (!provider->canRespawnHere()) {
        worldInfo->setSpawn(BlockPos::ORIGIN.up(provider->getAverageGroundLevel()));
    } else if (worldInfo->getTerrainType() == &WorldType::DEBUG_WORLD) {
        worldInfo->setSpawn(BlockPos::ORIGIN.up());
    } else {
        findingSpawnPoint = true;
        WorldChunkManager* worldchunkmanager = provider->getWorldChunkManager();
        const std::vector<BiomeGenBase*>& list = worldchunkmanager->getBiomesToSpawnIn();
        Random random(getSeed());
        const std::optional<BlockPos> blockpos = worldchunkmanager->findBiomePosition(0, 0, 256, list, random);
        int i = 0;
        const int j = provider->getAverageGroundLevel();
        int k = 0;

        if (blockpos) {
            i = blockpos->getX();
            k = blockpos->getZ();
        } else {
            Log::warn("Unable to find spawn biome");
        }

        int l = 0;
        while (!provider->canCoordinateBeSpawn(i, k)) {
            const int di = random.nextInt(64);
            i += di - random.nextInt(64);
            const int dk = random.nextInt(64);
            k += dk - random.nextInt(64);
            ++l;
            if (l == 1000)
                break;
        }

        worldInfo->setSpawn(BlockPos(i, j, k));
        findingSpawnPoint = false;

        if (settings.isBonusChestEnabled())
            createBonusChest();
    }
}

void WorldServer::createBonusChest()
{
    WorldGeneratorBonusChest worldgeneratorbonuschest(bonusChestContent(), 10);

    for (int i = 0; i < 10; ++i) {
        const int dj = rand.nextInt(6);
        const int j = worldInfo->getSpawnX() + dj - rand.nextInt(6);
        const int dk = rand.nextInt(6);
        const int k = worldInfo->getSpawnZ() + dk - rand.nextInt(6);
        const BlockPos blockpos = getTopSolidOrLiquidBlock(BlockPos(j, 0, k)).up();
        if (worldgeneratorbonuschest.generate(this, rand, blockpos))
            break;
    }
}

std::optional<BlockPos> WorldServer::getSpawnCoordinate() const
{
    return provider->getSpawnCoordinate();
}

void WorldServer::saveAllChunks(bool saveAll, IProgressUpdate* progressCallback)
{
    if (!chunkProvider->canSave())
        return;

    if (progressCallback)
        progressCallback->displaySavingString("Saving level");
    saveLevel();
    if (progressCallback)
        progressCallback->displayLoadingString("Saving chunks");
    chunkProvider->saveChunks(saveAll, progressCallback);

    for (Chunk* chunk : std::vector<Chunk*>(theChunkProviderServer->func_152380_a())) {
        if (chunk != nullptr && !thePlayerManager->hasPlayerInstance(chunk->xPosition, chunk->zPosition))
            theChunkProviderServer->dropChunk(chunk->xPosition, chunk->zPosition);
    }
}

void WorldServer::saveChunkData()
{
    if (chunkProvider->canSave())
        chunkProvider->saveExtraData();
}

void WorldServer::saveLevel()
{
    checkSessionLock();
    WorldBorder& border = getWorldBorder();
    worldInfo->setBorderSize(border.getDiameter());
    worldInfo->setBorderCenterX(border.getCenterX());
    worldInfo->setBorderCenterZ(border.getCenterZ());
    worldInfo->setBorderSafeZone(border.getDamageBuffer());
    worldInfo->setBorderDamagePerBlock(border.getDamageAmount());
    worldInfo->setBorderWarningDistance(border.getWarningDistance());
    worldInfo->setBorderWarningTime(border.getWarningTime());
    worldInfo->setBorderLerpTarget(border.getTargetSize());
    worldInfo->setBorderLerpTime(border.getTimeUntilTarget());
    saveHandler->saveWorldInfoWithPlayer(*worldInfo, mcServer->getConfigurationManager()->getHostPlayerData());
    mapStorage->saveAllData();
}

void WorldServer::onEntityAdded(Entity* entityIn)
{
    World::onEntityAdded(entityIn);
    entitiesById[entityIn->getEntityId()] = entityIn;
    entitiesByUuid[entityIn->getUniqueID()] = entityIn;
    // Java: the parts of multipart entities (the dragon) -- stage 10.
}

void WorldServer::onEntityRemoved(Entity* entityIn)
{
    World::onEntityRemoved(entityIn);
    entitiesById.erase(entityIn->getEntityId());
    entitiesByUuid.erase(entityIn->getUniqueID());
}

bool WorldServer::addWeatherEffect(const std::shared_ptr<Entity>& entityIn)
{
    if (World::addWeatherEffect(entityIn)) {
        mcServer->getConfigurationManager()->sendToAllNear(entityIn->posX, entityIn->posY, entityIn->posZ, 512.0, provider->getDimensionId(),
            std::make_shared<S2CPacketSpawnGlobalEntity>(entityIn.get(), 1));
        return true;
    }
    return false;
}

void WorldServer::setEntityState(Entity* entityIn, int8_t state)
{
    getEntityTracker()->func_151248_b(entityIn, std::make_shared<S19PacketEntityStatus>(entityIn, state));
}

void WorldServer::addBlockEvent(const BlockPos& pos, Block* blockIn, int eventID, int eventParam)
{
    const BlockEventData blockeventdata(pos, blockIn, eventID, eventParam);
    std::vector<BlockEventData>& queue = blockEventQueue[static_cast<size_t>(blockEventCacheIndex)];
    if (std::find(queue.begin(), queue.end(), blockeventdata) != queue.end())
        return;
    queue.push_back(blockeventdata);
}

void WorldServer::sendQueuedBlockEvents()
{
    while (!blockEventQueue[static_cast<size_t>(blockEventCacheIndex)].empty()) {
        const int i = blockEventCacheIndex;
        blockEventCacheIndex ^= 1;

        for (const BlockEventData& blockeventdata : blockEventQueue[static_cast<size_t>(i)]) {
            if (fireBlockEvent(blockeventdata)) {
                const BlockPos& p = blockeventdata.getPosition();
                mcServer->getConfigurationManager()->sendToAllNear(p.getX(), p.getY(), p.getZ(), 64.0, provider->getDimensionId(),
                    std::make_shared<S24PacketBlockAction>(p, blockeventdata.getBlock(), blockeventdata.getEventID(), blockeventdata.getEventParameter()));
            }
        }

        blockEventQueue[static_cast<size_t>(i)].clear();
    }
}

bool WorldServer::fireBlockEvent(const BlockEventData& event)
{
    const IBlockState* iblockstate = getBlockState(event.getPosition());
    return iblockstate->getBlock() == event.getBlock()
        ? iblockstate->getBlock()->onBlockEventReceived(this, event.getPosition(), iblockstate, event.getEventID(), event.getEventParameter())
        : false;
}

void WorldServer::flush()
{
    saveHandler->flush();
}

void WorldServer::updateWeather()
{
    const bool flag = isRaining();
    World::updateWeather();
    ServerConfigurationManager* manager = mcServer->getConfigurationManager();

    if (prevRainingStrength != rainingStrength)
        manager->sendPacketToAllPlayersInDimension(std::make_shared<S2BPacketChangeGameState>(7, rainingStrength), provider->getDimensionId());

    if (prevThunderingStrength != thunderingStrength)
        manager->sendPacketToAllPlayersInDimension(std::make_shared<S2BPacketChangeGameState>(8, thunderingStrength), provider->getDimensionId());

    if (flag != isRaining()) {
        if (flag)
            manager->sendPacketToAllPlayers(std::make_shared<S2BPacketChangeGameState>(2, 0.0f));
        else
            manager->sendPacketToAllPlayers(std::make_shared<S2BPacketChangeGameState>(1, 0.0f));
        manager->sendPacketToAllPlayers(std::make_shared<S2BPacketChangeGameState>(7, rainingStrength));
        manager->sendPacketToAllPlayers(std::make_shared<S2BPacketChangeGameState>(8, thunderingStrength));
    }
}

int WorldServer::getRenderDistanceChunks()
{
    return mcServer->getConfigurationManager()->getViewDistance();
}

void WorldServer::spawnParticle(EnumParticleTypes particleType, bool longDistance, double xCoord, double yCoord, double zCoord, int numberOfParticles,
    double xOffset, double yOffset, double zOffset, double particleSpeed, const std::vector<int>& particleArguments)
{
    const auto packet = std::make_shared<S2APacketParticles>(particleType, longDistance, static_cast<float>(xCoord), static_cast<float>(yCoord),
        static_cast<float>(zCoord), static_cast<float>(xOffset), static_cast<float>(yOffset), static_cast<float>(zOffset), static_cast<float>(particleSpeed),
        numberOfParticles, particleArguments);

    for (const std::shared_ptr<EntityPlayer>& player : playerEntities) {
        auto* entityplayermp = static_cast<EntityPlayerMP*>(player.get());
        const BlockPos blockpos = entityplayermp->getPosition();
        const double d0 = blockpos.distanceSq(xCoord, yCoord, zCoord);
        if (d0 <= 256.0 || (longDistance && d0 <= 65536.0))
            entityplayermp->playerNetServerHandler->sendPacket(packet);
    }
}

Entity* WorldServer::getEntityFromUuid(const UUID& uuid)
{
    const auto it = entitiesByUuid.find(uuid);
    return it == entitiesByUuid.end() ? nullptr : it->second;
}

void WorldServer::addScheduledTask(std::function<void()> task)
{
    mcServer->addScheduledTask(std::move(task));
}

bool WorldServer::isCallingFromMinecraftThread() const
{
    return mcServer->isCallingFromMinecraftThread();
}

} // namespace mc

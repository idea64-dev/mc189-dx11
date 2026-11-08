#pragma once

// Original: net/minecraft/world/WorldServer.java and WorldManager.java (the IWorldAccess that sends the world's
// sounds, effects and block changes to the players).
// Not ported yet: mob spawning (SpawnerAnimals, stage 10), villages (the siege below runs its night check, which
// draws from rand, with no villages to find), portals (Teleporter, stage 12), explosions, the scoreboard and
// the Nether / End worlds (WorldServerMulti).

#include "world/IWorldAccess.h"
#include "world/NextTickListEntry.h"
#include "world/World.h"

#include <array>
#include <functional>
#include <set>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace mc {

class EntityPlayerMP;
class EntityTracker;
class MinecraftServer;
class PlayerManager;
class ChunkProviderServer;
class WorldServer;
class IProgressUpdate;
enum class EnumParticleTypes;

// Original: net/minecraft/world/WorldManager.java
class WorldManager : public IWorldAccess {
public:
    WorldManager(MinecraftServer* server, WorldServer* world)
        : mcServer(server)
        , theWorldServer(world)
    {
    }

    void spawnParticle(int, bool, double, double, double, double, double, double, const std::vector<int>&) override { }
    void onEntityAdded(Entity* entityIn) override;
    void onEntityRemoved(Entity* entityIn) override;
    void playSound(const std::string& soundName, double x, double y, double z, float volume, float pitch) override;
    void playSoundToNearExcept(EntityPlayer* except, const std::string& soundName, double x, double y, double z, float volume, float pitch) override;
    void markBlockRangeForRenderUpdate(int, int, int, int, int, int) override { }
    void markBlockForUpdate(const BlockPos& pos) override;
    void notifyLightSet(const BlockPos&) override { }
    void playRecord(const std::string&, const BlockPos&) override { }
    void playAuxSFX(EntityPlayer* player, int sfxType, const BlockPos& blockPosIn, int data) override;
    void broadcastSound(int soundID, const BlockPos& pos, int data) override;
    void sendBlockBreakProgress(int breakerId, const BlockPos& pos, int progress) override;

private:
    MinecraftServer* mcServer;
    WorldServer* theWorldServer;
};

class WorldServer : public World {
public:
    WorldServer(MinecraftServer* server, std::shared_ptr<ISaveHandler> saveHandlerIn, std::shared_ptr<WorldInfo> info, int dimensionId, Profiler& profilerIn);
    ~WorldServer() override;

    World* init() override;
    void tick() override;
    void updateAllPlayersSleepingFlag() override;
    bool areAllPlayersAsleep();
    void setInitialSpawnLocation() override;
    bool isBlockTickPending(const BlockPos& pos, Block* blockType) override;
    void scheduleUpdate(const BlockPos& pos, Block* blockIn, int delay) override { updateBlockTick(pos, blockIn, delay, 0); }
    void updateBlockTick(const BlockPos& pos, Block* blockIn, int delay, int priority) override;
    void scheduleBlockUpdate(const BlockPos& pos, Block* blockIn, int delay, int priority) override;
    void updateEntities() override;
    void resetUpdateEntityTick() { updateEntityTick = 0; }
    bool tickUpdates(bool runAllPending) override;
    bool hasScheduledTicks() const override { return true; }
    std::vector<NextTickListEntry> getPendingBlockUpdates(Chunk* chunkIn, bool remove) override;
    // Java: func_175712_a -- the scheduled ticks inside the box (x and z only).
    std::vector<NextTickListEntry> func_175712_a(int minX, int minZ, int maxX, int maxZ, bool remove);
    void updateEntityWithOptionalForce(Entity* entityIn, bool forceUpdate) override;
    std::vector<TileEntity*> getTileEntitiesIn(int minX, int minY, int minZ, int maxX, int maxY, int maxZ);
    bool isBlockModifiable(EntityPlayer* player, const BlockPos& pos) override;
    void initialize(const WorldSettings& settings) override;
    std::optional<BlockPos> getSpawnCoordinate() const;
    // Throws MinecraftException when another instance took over the save.
    void saveAllChunks(bool saveAll, IProgressUpdate* progressCallback);
    void saveChunkData();
    bool addWeatherEffect(const std::shared_ptr<Entity>& entityIn) override;
    void setEntityState(Entity* entityIn, int8_t state) override;
    void addBlockEvent(const BlockPos& pos, Block* blockIn, int eventID, int eventParam) override;
    void flush();
    MinecraftServer* getMinecraftServer() const { return mcServer; }
    EntityTracker* getEntityTracker() const { return theEntityTracker.get(); }
    PlayerManager* getPlayerManager() const { return thePlayerManager.get(); }
    void spawnParticle(EnumParticleTypes particleType, double xCoord, double yCoord, double zCoord, int numberOfParticles, double xOffset, double yOffset,
        double zOffset, double particleSpeed, const std::vector<int>& particleArguments = {})
    {
        spawnParticle(particleType, false, xCoord, yCoord, zCoord, numberOfParticles, xOffset, yOffset, zOffset, particleSpeed, particleArguments);
    }
    void spawnParticle(EnumParticleTypes particleType, bool longDistance, double xCoord, double yCoord, double zCoord, int numberOfParticles, double xOffset,
        double yOffset, double zOffset, double particleSpeed, const std::vector<int>& particleArguments);
    Entity* getEntityFromUuid(const UUID& uuid);
    // Java: addWorldAccess(new WorldManager(server, this)) -- the world keeps it.
    void setWorldManager(std::unique_ptr<WorldManager> manager);
    void addScheduledTask(std::function<void()> task);
    bool isCallingFromMinecraftThread() const;

    ChunkProviderServer* theChunkProviderServer = nullptr;
    bool disableLevelSaving = false;

protected:
    std::unique_ptr<IChunkProvider> createChunkProvider() override;
    int getRenderDistanceChunks() override;
    void updateBlocks() override;
    void updateWeather() override;
    void onEntityAdded(Entity* entityIn) override;
    void onEntityRemoved(Entity* entityIn) override;
    void wakeAllPlayers();
    void createBonusChest();
    void saveLevel();
    BlockPos adjustPosToNearbyEntity(const BlockPos& pos);

private:
    // Java: VillageSiege -- only its nightly roll so far; with no villages it never starts.
    struct VillageSiege {
        int state = -1;
        bool started = false;
        void tick(World& world);
    };

    struct TickEntryHash {
        size_t operator()(const NextTickListEntry& entry) const { return std::hash<int64_t>()(entry.position.toLong()); }
    };
    struct TickEntryEqual {
        bool operator()(const NextTickListEntry& a, const NextTickListEntry& b) const { return a.equals(b); }
    };

    void resetRainAndThunder();
    void setDebugWorldSettings();
    void createSpawnPosition(const WorldSettings& settings);
    void sendQueuedBlockEvents();
    bool fireBlockEvent(const BlockEventData& event);
    bool canSpawnNPCs() const;
    bool canSpawnAnimals() const;

    MinecraftServer* const mcServer;
    std::unique_ptr<EntityTracker> theEntityTracker;
    std::unique_ptr<PlayerManager> thePlayerManager;
    std::unique_ptr<WorldManager> worldManager;
    std::unordered_set<NextTickListEntry, TickEntryHash, TickEntryEqual> pendingTickListEntriesHashSet;
    std::set<NextTickListEntry> pendingTickListEntriesTreeSet;
    // The tree set's entries by chunk (getPendingBlockUpdates asks for a chunk at a time).
    std::unordered_map<int64_t, std::set<NextTickListEntry>> pendingTicksByChunk;
    void indexPendingTick(const NextTickListEntry& entry, bool add);
    struct UuidHash {
        size_t operator()(const UUID& id) const { return static_cast<size_t>(id.hashCode()); }
    };
    std::unordered_map<UUID, Entity*, UuidHash> entitiesByUuid;
    bool allPlayersSleeping = false;
    int updateEntityTick = 0;
    VillageSiege villageSiege;
    std::array<std::vector<BlockEventData>, 2> blockEventQueue;
    int blockEventCacheIndex = 0;
    std::vector<NextTickListEntry> pendingTickListEntriesThisTick;
};

} // namespace mc

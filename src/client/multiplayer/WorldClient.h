#pragma once

// Original: net/minecraft/client/multiplayer/WorldClient.java
// The client's copy of the world, filled by NetHandlerPlayClient. Not ported yet: the scoreboard
// (setWorldScoreboard), fireworks (makeFireworks, with the particles of stage 15), the minecart sound and the crash
// report section.

#include "core/JavaHashMap.h"
#include "entity/Entity.h"
#include "world/World.h"

#include <memory>
#include <string>

namespace mc {

class ChunkProviderClient;
class Minecraft;
class NBTTagCompound;
class NetHandlerPlayClient;
class WorldSettings;
enum class EnumDifficulty;

class WorldClient : public World {
public:
    WorldClient(NetHandlerPlayClient* netHandler, const WorldSettings& settings, int dimension, EnumDifficulty difficulty, Profiler& profilerIn);
    ~WorldClient() override;

    void tick() override;
    // Java: a no-op in 1.8.9 (the region the server overwrites is not tracked any more).
    void invalidateBlockReceiveRegion(int, int, int, int, int, int) { }
    void doPreChunk(int chuncX, int chuncZ, bool loadChunk);
    bool spawnEntityInWorld(const std::shared_ptr<Entity>& entityIn) override;
    void removeEntity(Entity* entityIn) override;
    void addEntityToWorld(int entityID, const std::shared_ptr<Entity>& entityToSpawn);
    Entity* getEntityByID(int id) override;
    // The removed entity (kept alive for the caller), null when there was none.
    std::shared_ptr<Entity> removeEntityFromWorld(int entityID);
    bool invalidateRegionAndSetBlock(const BlockPos& pos, const IBlockState* state);
    void sendQuittingDisconnectingPacket();
    void doVoidFogParticles(int posX, int posY, int posZ);
    void removeAllEntities();
    void playSoundAtPos(const BlockPos& pos, const std::string& soundName, float volume, float pitch, bool distanceDelay);
    void playSound(double x, double y, double z, const std::string& soundName, float volume, float pitch, bool distanceDelay) override;
    void makeFireworks(double, double, double, double, double, double, const NBTTagCompound*) { }
    // Java: a negative time means doDaylightCycle is off.
    void setWorldTime(int64_t time);
    ChunkProviderClient& getClientChunkProvider() { return *clientChunkProvider; }

protected:
    std::unique_ptr<IChunkProvider> createChunkProvider() override;
    void updateBlocks() override;
    void updateWeather() override { }
    int getRenderDistanceChunks() override;
    void onEntityAdded(Entity* entityIn) override;
    void onEntityRemoved(Entity* entityIn) override;

private:
    // Java: HashSet<Entity>. Entity.hashCode/equals go by the entity id, read at each call, so an entity whose id
    // changes after insertion is no longer found -- as in Java. The value holds the reference.
    struct EntityKey {
        Entity* entity;
        bool operator==(const EntityKey& other) const { return entity->getEntityId() == other.entity->getEntityId(); }
    };
    struct EntityKeyHash {
        int32_t operator()(const EntityKey& key) const { return key.entity->hashCode(); }
    };
    using EntitySet = JavaHashMap<EntityKey, std::shared_ptr<Entity>, EntityKeyHash>;

    NetHandlerPlayClient* sendQueue;
    ChunkProviderClient* clientChunkProvider = nullptr;
    EntitySet entityList;
    EntitySet entitySpawnQueue;
    Minecraft* mc;
    JavaHashMap<ChunkCoordIntPair, bool> previousActiveChunkSet;
};

} // namespace mc

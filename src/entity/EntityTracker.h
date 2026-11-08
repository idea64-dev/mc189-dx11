#pragma once

// Original: net/minecraft/entity/EntityTracker.java and EntityTrackerEntry.java -- which players see which entity,
// and the spawn / movement / metadata packets that keep their copies in step.
// Only the players are tracked so far; the other entity kinds join with their classes (stage 10).

#include "core/JavaHashMap.h"

#include <memory>
#include <unordered_map>
#include <vector>

namespace mc {

class Chunk;
class Entity;
class EntityPlayer;
class EntityPlayerMP;
class Packet;
class WorldServer;

class EntityTrackerEntry {
public:
    EntityTrackerEntry(Entity* trackedEntityIn, int trackingDistanceThresholdIn, int updateFrequencyIn, bool sendVelocityUpdatesIn);

    void updatePlayerList(const std::vector<std::shared_ptr<EntityPlayer>>& players);
    void sendPacketToTrackedPlayers(const std::shared_ptr<Packet>& packetIn);
    // Java: func_151261_b -- to the trackers and the tracked player itself.
    void func_151261_b(const std::shared_ptr<Packet>& packetIn);
    void sendDestroyEntityPacketToTrackedPlayers();
    void removeFromTrackedPlayers(EntityPlayerMP* playerMP);
    void updatePlayerEntity(EntityPlayerMP* playerMP);
    bool func_180233_c(EntityPlayerMP* playerMP);
    void updatePlayerEntities(const std::vector<std::shared_ptr<EntityPlayer>>& players);
    void removeTrackedPlayerSymmetric(EntityPlayerMP* playerMP);

    Entity* trackedEntity;
    int trackingDistanceThreshold;
    int updateFrequency;
    int encodedPosX;
    int encodedPosY;
    int encodedPosZ;
    int encodedRotationYaw;
    int encodedRotationPitch;
    int lastHeadMotion;
    double lastTrackedEntityMotionX = 0.0;
    double lastTrackedEntityMotionY = 0.0;
    double motionZ = 0.0;
    int updateCounter = 0;
    bool playerEntitiesUpdated = false;
    // Java: a HashSet of players, whose order follows identity hashes; here the order they started tracking.
    std::vector<EntityPlayerMP*> trackingPlayers;

private:
    void sendMetadataToAllAssociatedPlayers();
    bool isPlayerWatchingThisChunk(EntityPlayerMP* playerMP);
    bool isTracking(EntityPlayerMP* playerMP) const;
    void stopTracking(EntityPlayerMP* playerMP);
    std::shared_ptr<Packet> createSpawnPacket();

    double lastTrackedEntityPosX = 0.0;
    double lastTrackedEntityPosY = 0.0;
    double lastTrackedEntityPosZ = 0.0;
    bool firstUpdateDone = false;
    bool sendVelocityUpdates;
    int ticksSinceLastForcedTeleport = 0;
    Entity* field_85178_v = nullptr;
    bool ridingEntity = false;
    bool onGround;
};

class EntityTracker {
public:
    explicit EntityTracker(WorldServer* theWorldIn);
    ~EntityTracker();

    void trackEntity(Entity* entityIn);
    void trackEntity(Entity* entityIn, int trackingRange, int updateFrequency) { addEntityToTracker(entityIn, trackingRange, updateFrequency, false); }
    void addEntityToTracker(Entity* entityIn, int trackingRange, int updateFrequency, bool sendVelocityUpdates);
    void untrackEntity(Entity* entityIn);
    void updateTrackedEntities();
    void func_180245_a(EntityPlayerMP* player);
    void sendToAllTrackingEntity(Entity* entityIn, const std::shared_ptr<Packet>& packet);
    void func_151248_b(Entity* entityIn, const std::shared_ptr<Packet>& packet);
    void removePlayerFromTrackers(EntityPlayerMP* player);
    void func_85172_a(EntityPlayerMP* player, Chunk* chunk);

private:
    WorldServer* theWorld;
    // Java: a HashSet whose hash is the entity id -- iterated in that order.
    JavaHashMap<int32_t, EntityTrackerEntry*> trackedEntities;
    // Java: trackedEntityHashTable (an IntHashMap); owns the entries.
    std::unordered_map<int, std::unique_ptr<EntityTrackerEntry>> trackedEntityHashTable;
    int maxTrackingDistanceThreshold;
};

} // namespace mc

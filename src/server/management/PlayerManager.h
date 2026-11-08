#pragma once

// Original: net/minecraft/server/management/PlayerManager.java -- which chunks each player watches; sends their
// block changes and keeps the watched chunks loaded.

#include "util/BlockPos.h"
#include "world/ChunkCoordIntPair.h"

#include <array>
#include <cstdint>
#include <memory>
#include <unordered_map>
#include <vector>

namespace mc {

class Chunk;
class EntityPlayerMP;
class Packet;
class TileEntity;
class WorldServer;

class PlayerManager {
public:
    explicit PlayerManager(WorldServer* serverWorld);
    ~PlayerManager();

    WorldServer* getWorldServer() const { return theWorldServer; }
    void updatePlayerInstances();
    bool hasPlayerInstance(int chunkX, int chunkZ) const;
    void markBlockForUpdate(const BlockPos& pos);
    void addPlayer(EntityPlayerMP* player);
    // Reorders the player's chunk queue to spiral out from the player's chunk.
    void filterChunkLoadQueue(EntityPlayerMP* player);
    void removePlayer(EntityPlayerMP* player);
    void updateMountedMovingPlayer(EntityPlayerMP* player);
    bool isPlayerWatchingChunk(EntityPlayerMP* player, int chunkX, int chunkZ);
    void setPlayerViewRadius(int radius);
    static int getFurthestViewableBlock(int distance) { return distance * 16 - 16; }

private:
    class PlayerInstance {
    public:
        PlayerInstance(PlayerManager& manager, int chunkX, int chunkZ);

        void addPlayer(EntityPlayerMP* player);
        void removePlayer(EntityPlayerMP* player);
        void processChunk();
        void flagChunkForUpdate(int x, int y, int z);
        void sendToAllPlayersWatchingChunk(const std::shared_ptr<Packet>& thePacket);
        void onUpdate();
        bool isWatchedBy(EntityPlayerMP* player) const;

        const ChunkCoordIntPair chunkCoords;

    private:
        void increaseInhabitedTime(Chunk* theChunk);
        void sendTileToAllPlayersWatchingChunk(TileEntity* theTileEntity);

        PlayerManager& owner;
        std::vector<EntityPlayerMP*> playersWatchingChunk;
        std::array<int16_t, 64> locationOfBlockChange {};
        int numBlocksToUpdate = 0;
        int flagsYAreasToUpdate = 0;
        int64_t previousWorldTime = 0;
    };

    static int64_t instanceKey(int chunkX, int chunkZ);
    PlayerInstance* getPlayerInstance(int chunkX, int chunkZ, bool createIfAbsent);
    static bool overlaps(int x1, int z1, int x2, int z2, int radius);

    WorldServer* theWorldServer;
    std::vector<EntityPlayerMP*> players;
    // Java: a LongHashMap, never iterated; it owns the instances.
    std::unordered_map<int64_t, std::unique_ptr<PlayerInstance>> playerInstances;
    std::vector<PlayerInstance*> playerInstancesToUpdate;
    std::vector<PlayerInstance*> playerInstanceList;
    int playerViewRadius = 0;
    int64_t previousTotalWorldTime = 0;
    static constexpr int xzDirectionsConst[4][2] { { 1, 0 }, { 0, 1 }, { -1, 0 }, { 0, -1 } };
};

} // namespace mc

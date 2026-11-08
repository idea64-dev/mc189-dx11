#include "server/management/PlayerManager.h"

#include "block/Block.h"
#include "core/Log.h"
#include "entity/player/EntityPlayerMP.h"
#include "network/NetHandlerPlayServer.h"
#include "network/play/ServerPlayPackets.h"
#include "server/MinecraftServer.h"
#include "server/management/ServerConfigurationManager.h"
#include "tileentity/TileEntity.h"
#include "util/MathHelper.h"
#include "world/WorldProvider.h"
#include "world/WorldServer.h"
#include "world/chunk/Chunk.h"
#include "world/gen/ChunkProviderServer.h"

#include <algorithm>

namespace mc {

namespace {

template <class List>
bool listContains(const List& list, const ChunkCoordIntPair& pos)
{
    return std::find(list.begin(), list.end(), pos) != list.end();
}

// Java: List.remove(Object) -- the first occurrence.
template <class List, class T>
void listRemove(List& list, const T& value)
{
    if (const auto it = std::find(list.begin(), list.end(), value); it != list.end())
        list.erase(it);
}

} // namespace

PlayerManager::PlayerManager(WorldServer* serverWorld)
    : theWorldServer(serverWorld)
{
    setPlayerViewRadius(serverWorld->getMinecraftServer()->getConfigurationManager()->getViewDistance());
}

PlayerManager::~PlayerManager() = default;

int64_t PlayerManager::instanceKey(int chunkX, int chunkZ)
{
    // Java: (long)chunkX + 2147483647L | (long)chunkZ + 2147483647L << 32
    return (static_cast<int64_t>(chunkX) + 2147483647LL) | static_cast<int64_t>(static_cast<uint64_t>(static_cast<int64_t>(chunkZ) + 2147483647LL) << 32);
}

void PlayerManager::updatePlayerInstances()
{
    const int64_t i = theWorldServer->getTotalWorldTime();

    if (i - previousTotalWorldTime > 8000) {
        previousTotalWorldTime = i;
        for (size_t j = 0; j < playerInstanceList.size(); ++j) {
            PlayerInstance* instance = playerInstanceList[j];
            instance->onUpdate();
            instance->processChunk();
        }
    } else {
        for (size_t k = 0; k < playerInstancesToUpdate.size(); ++k)
            playerInstancesToUpdate[k]->onUpdate();
    }

    playerInstancesToUpdate.clear();

    if (players.empty()) {
        if (!theWorldServer->provider->canRespawnHere())
            theWorldServer->theChunkProviderServer->unloadAllChunks();
    }
}

bool PlayerManager::hasPlayerInstance(int chunkX, int chunkZ) const
{
    return playerInstances.contains(instanceKey(chunkX, chunkZ));
}

PlayerManager::PlayerInstance* PlayerManager::getPlayerInstance(int chunkX, int chunkZ, bool createIfAbsent)
{
    const int64_t i = instanceKey(chunkX, chunkZ);
    if (const auto it = playerInstances.find(i); it != playerInstances.end())
        return it->second.get();
    if (!createIfAbsent)
        return nullptr;
    auto created = std::make_unique<PlayerInstance>(*this, chunkX, chunkZ);
    PlayerInstance* instance = created.get();
    playerInstances.emplace(i, std::move(created));
    playerInstanceList.push_back(instance);
    return instance;
}

void PlayerManager::markBlockForUpdate(const BlockPos& pos)
{
    const int i = pos.getX() >> 4;
    const int j = pos.getZ() >> 4;
    if (PlayerInstance* instance = getPlayerInstance(i, j, false))
        instance->flagChunkForUpdate(pos.getX() & 15, pos.getY(), pos.getZ() & 15);
}

void PlayerManager::addPlayer(EntityPlayerMP* player)
{
    const int i = java::d2i(player->posX) >> 4;
    const int j = java::d2i(player->posZ) >> 4;
    player->managedPosX = player->posX;
    player->managedPosZ = player->posZ;

    for (int k = i - playerViewRadius; k <= i + playerViewRadius; ++k) {
        for (int l = j - playerViewRadius; l <= j + playerViewRadius; ++l)
            getPlayerInstance(k, l, true)->addPlayer(player);
    }

    players.push_back(player);
    filterChunkLoadQueue(player);
}

void PlayerManager::filterChunkLoadQueue(EntityPlayerMP* player)
{
    const std::vector<ChunkCoordIntPair> list(player->loadedChunks.begin(), player->loadedChunks.end());
    int i = 0;
    const int j = playerViewRadius;
    const int k = java::d2i(player->posX) >> 4;
    const int l = java::d2i(player->posZ) >> 4;
    int i1 = 0;
    int j1 = 0;
    ChunkCoordIntPair chunkcoordintpair = getPlayerInstance(k, l, true)->chunkCoords;
    player->loadedChunks.clear();

    if (listContains(list, chunkcoordintpair))
        player->loadedChunks.push_back(chunkcoordintpair);

    for (int k1 = 1; k1 <= j * 2; ++k1) {
        for (int l1 = 0; l1 < 2; ++l1) {
            const int* aint = xzDirectionsConst[i++ % 4];
            for (int i2 = 0; i2 < k1; ++i2) {
                i1 += aint[0];
                j1 += aint[1];
                chunkcoordintpair = getPlayerInstance(k + i1, l + j1, true)->chunkCoords;
                if (listContains(list, chunkcoordintpair))
                    player->loadedChunks.push_back(chunkcoordintpair);
            }
        }
    }

    i = i % 4;

    for (int j2 = 0; j2 < j * 2; ++j2) {
        i1 += xzDirectionsConst[i][0];
        j1 += xzDirectionsConst[i][1];
        chunkcoordintpair = getPlayerInstance(k + i1, l + j1, true)->chunkCoords;
        if (listContains(list, chunkcoordintpair))
            player->loadedChunks.push_back(chunkcoordintpair);
    }
}

void PlayerManager::removePlayer(EntityPlayerMP* player)
{
    const int i = java::d2i(player->managedPosX) >> 4;
    const int j = java::d2i(player->managedPosZ) >> 4;

    for (int k = i - playerViewRadius; k <= i + playerViewRadius; ++k) {
        for (int l = j - playerViewRadius; l <= j + playerViewRadius; ++l) {
            if (PlayerInstance* instance = getPlayerInstance(k, l, false))
                instance->removePlayer(player);
        }
    }

    listRemove(players, player);
}

bool PlayerManager::overlaps(int x1, int z1, int x2, int z2, int radius)
{
    const int i = x1 - x2;
    const int j = z1 - z2;
    return i >= -radius && i <= radius ? j >= -radius && j <= radius : false;
}

void PlayerManager::updateMountedMovingPlayer(EntityPlayerMP* player)
{
    const int i = java::d2i(player->posX) >> 4;
    const int j = java::d2i(player->posZ) >> 4;
    const double d0 = player->managedPosX - player->posX;
    const double d1 = player->managedPosZ - player->posZ;
    const double d2 = d0 * d0 + d1 * d1;

    if (d2 >= 64.0) {
        const int k = java::d2i(player->managedPosX) >> 4;
        const int l = java::d2i(player->managedPosZ) >> 4;
        const int i1 = playerViewRadius;
        const int j1 = i - k;
        const int k1 = j - l;

        if (j1 != 0 || k1 != 0) {
            for (int l1 = i - i1; l1 <= i + i1; ++l1) {
                for (int i2 = j - i1; i2 <= j + i1; ++i2) {
                    if (!overlaps(l1, i2, k, l, i1))
                        getPlayerInstance(l1, i2, true)->addPlayer(player);

                    if (!overlaps(l1 - j1, i2 - k1, i, j, i1)) {
                        if (PlayerInstance* instance = getPlayerInstance(l1 - j1, i2 - k1, false))
                            instance->removePlayer(player);
                    }
                }
            }

            filterChunkLoadQueue(player);
            player->managedPosX = player->posX;
            player->managedPosZ = player->posZ;
        }
    }
}

bool PlayerManager::isPlayerWatchingChunk(EntityPlayerMP* player, int chunkX, int chunkZ)
{
    PlayerInstance* instance = getPlayerInstance(chunkX, chunkZ, false);
    return instance != nullptr && instance->isWatchedBy(player) && !listContains(player->loadedChunks, instance->chunkCoords);
}

void PlayerManager::setPlayerViewRadius(int radius)
{
    radius = MathHelper::clamp_int(radius, 3, 32);

    if (radius != playerViewRadius) {
        const int i = radius - playerViewRadius;

        for (EntityPlayerMP* entityplayermp : std::vector<EntityPlayerMP*>(players)) {
            const int j = java::d2i(entityplayermp->posX) >> 4;
            const int k = java::d2i(entityplayermp->posZ) >> 4;

            if (i > 0) {
                for (int j1 = j - radius; j1 <= j + radius; ++j1) {
                    for (int k1 = k - radius; k1 <= k + radius; ++k1) {
                        PlayerInstance* instance = getPlayerInstance(j1, k1, true);
                        if (!instance->isWatchedBy(entityplayermp))
                            instance->addPlayer(entityplayermp);
                    }
                }
            } else {
                for (int l = j - playerViewRadius; l <= j + playerViewRadius; ++l) {
                    for (int i1 = k - playerViewRadius; i1 <= k + playerViewRadius; ++i1) {
                        if (!overlaps(l, i1, j, k, radius))
                            getPlayerInstance(l, i1, true)->removePlayer(entityplayermp);
                    }
                }
            }
        }

        playerViewRadius = radius;
    }
}

PlayerManager::PlayerInstance::PlayerInstance(PlayerManager& manager, int chunkX, int chunkZ)
    : chunkCoords(chunkX, chunkZ)
    , owner(manager)
{
    owner.getWorldServer()->theChunkProviderServer->loadChunk(chunkX, chunkZ);
}

bool PlayerManager::PlayerInstance::isWatchedBy(EntityPlayerMP* player) const
{
    return std::find(playersWatchingChunk.begin(), playersWatchingChunk.end(), player) != playersWatchingChunk.end();
}

void PlayerManager::PlayerInstance::addPlayer(EntityPlayerMP* player)
{
    if (isWatchedBy(player)) {
        Log::debug("Failed to add player. {} already is in chunk {}, {}", player->getName(), chunkCoords.chunkXPos, chunkCoords.chunkZPos);
        return;
    }
    if (playersWatchingChunk.empty())
        previousWorldTime = owner.theWorldServer->getTotalWorldTime();
    playersWatchingChunk.push_back(player);
    player->loadedChunks.push_back(chunkCoords);
}

void PlayerManager::PlayerInstance::removePlayer(EntityPlayerMP* player)
{
    if (!isWatchedBy(player))
        return;

    Chunk* chunk = owner.theWorldServer->getChunkFromChunkCoords(chunkCoords.chunkXPos, chunkCoords.chunkZPos);
    if (chunk->isPopulated())
        player->playerNetServerHandler->sendPacket(std::make_shared<S21PacketChunkData>(chunk, true, 0));

    listRemove(playersWatchingChunk, player);
    listRemove(player->loadedChunks, chunkCoords);

    if (playersWatchingChunk.empty()) {
        increaseInhabitedTime(chunk);
        PlayerManager& manager = owner;
        const ChunkCoordIntPair coords = chunkCoords;
        listRemove(manager.playerInstanceList, this);
        if (numBlocksToUpdate > 0)
            listRemove(manager.playerInstancesToUpdate, this);
        // Destroys this instance.
        manager.playerInstances.erase(instanceKey(coords.chunkXPos, coords.chunkZPos));
        manager.getWorldServer()->theChunkProviderServer->dropChunk(coords.chunkXPos, coords.chunkZPos);
    }
}

void PlayerManager::PlayerInstance::processChunk()
{
    increaseInhabitedTime(owner.theWorldServer->getChunkFromChunkCoords(chunkCoords.chunkXPos, chunkCoords.chunkZPos));
}

void PlayerManager::PlayerInstance::increaseInhabitedTime(Chunk* theChunk)
{
    theChunk->setInhabitedTime(theChunk->getInhabitedTime() + owner.theWorldServer->getTotalWorldTime() - previousWorldTime);
    previousWorldTime = owner.theWorldServer->getTotalWorldTime();
}

void PlayerManager::PlayerInstance::flagChunkForUpdate(int x, int y, int z)
{
    if (numBlocksToUpdate == 0)
        owner.playerInstancesToUpdate.push_back(this);

    flagsYAreasToUpdate |= 1 << (y >> 4);

    if (numBlocksToUpdate < 64) {
        const auto short1 = static_cast<int16_t>(x << 12 | z << 8 | y);
        for (int i = 0; i < numBlocksToUpdate; ++i) {
            if (locationOfBlockChange[static_cast<size_t>(i)] == short1)
                return;
        }
        locationOfBlockChange[static_cast<size_t>(numBlocksToUpdate++)] = short1;
    }
}

void PlayerManager::PlayerInstance::sendToAllPlayersWatchingChunk(const std::shared_ptr<Packet>& thePacket)
{
    for (size_t i = 0; i < playersWatchingChunk.size(); ++i) {
        EntityPlayerMP* entityplayermp = playersWatchingChunk[i];
        if (!listContains(entityplayermp->loadedChunks, chunkCoords))
            entityplayermp->playerNetServerHandler->sendPacket(thePacket);
    }
}

void PlayerManager::PlayerInstance::onUpdate()
{
    if (numBlocksToUpdate == 0)
        return;

    WorldServer* world = owner.theWorldServer;

    if (numBlocksToUpdate == 1) {
        const int i = (locationOfBlockChange[0] >> 12 & 15) + chunkCoords.chunkXPos * 16;
        const int j = locationOfBlockChange[0] & 255;
        const int k = (locationOfBlockChange[0] >> 8 & 15) + chunkCoords.chunkZPos * 16;
        const BlockPos blockpos(i, j, k);
        sendToAllPlayersWatchingChunk(std::make_shared<S23PacketBlockChange>(world, blockpos));
        if (world->getBlockState(blockpos)->getBlock()->hasTileEntity())
            sendTileToAllPlayersWatchingChunk(world->getTileEntity(blockpos));
    } else if (numBlocksToUpdate == 64) {
        const int i1 = chunkCoords.chunkXPos * 16;
        const int k1 = chunkCoords.chunkZPos * 16;
        sendToAllPlayersWatchingChunk(
            std::make_shared<S21PacketChunkData>(world->getChunkFromChunkCoords(chunkCoords.chunkXPos, chunkCoords.chunkZPos), false, flagsYAreasToUpdate));

        for (int i2 = 0; i2 < 16; ++i2) {
            if ((flagsYAreasToUpdate & 1 << i2) != 0) {
                const int k2 = i2 << 4;
                for (TileEntity* tileentity : world->getTileEntitiesIn(i1, k2, k1, i1 + 16, k2 + 16, k1 + 16))
                    sendTileToAllPlayersWatchingChunk(tileentity);
            }
        }
    } else {
        sendToAllPlayersWatchingChunk(std::make_shared<S22PacketMultiBlockChange>(
            numBlocksToUpdate, locationOfBlockChange.data(), world->getChunkFromChunkCoords(chunkCoords.chunkXPos, chunkCoords.chunkZPos)));

        for (int j1 = 0; j1 < numBlocksToUpdate; ++j1) {
            const int16_t location = locationOfBlockChange[static_cast<size_t>(j1)];
            const int l1 = (location >> 12 & 15) + chunkCoords.chunkXPos * 16;
            const int j2 = location & 255;
            const int l2 = (location >> 8 & 15) + chunkCoords.chunkZPos * 16;
            const BlockPos blockpos1(l1, j2, l2);
            if (world->getBlockState(blockpos1)->getBlock()->hasTileEntity())
                sendTileToAllPlayersWatchingChunk(world->getTileEntity(blockpos1));
        }
    }

    numBlocksToUpdate = 0;
    flagsYAreasToUpdate = 0;
}

void PlayerManager::PlayerInstance::sendTileToAllPlayersWatchingChunk(TileEntity* theTileEntity)
{
    if (theTileEntity != nullptr) {
        if (std::shared_ptr<Packet> packet = theTileEntity->getDescriptionPacket())
            sendToAllPlayersWatchingChunk(packet);
    }
}

} // namespace mc

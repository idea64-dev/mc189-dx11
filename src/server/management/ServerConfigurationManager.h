#pragma once

// Original: net/minecraft/server/management/ServerConfigurationManager.java and
// server/integrated/IntegratedPlayerList.java -- the players of a server: logging in and out, respawning,
// sending packets to all or to the ones near a point.
// The ban, op and white lists, the profile cache (usercache.json) and the statistics files come with
// multiplayer and stage 12; an integrated server has none of them in effect.

#include "auth/GameProfile.h"
#include "world/WorldSettings.h"

#include <memory>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

namespace mc {

class Entity;
class EntityPlayer;
class EntityPlayerMP;
class IChatComponent;
class IPlayerFileData;
class MinecraftServer;
class NBTTagCompound;
class NetworkManager;
class Packet;
class WorldServer;

class ServerConfigurationManager {
public:
    explicit ServerConfigurationManager(MinecraftServer* server);
    virtual ~ServerConfigurationManager();

    void initializeConnectionToPlayer(const std::shared_ptr<NetworkManager>& netManager, const std::shared_ptr<EntityPlayerMP>& playerIn);
    // Java: setPlayerManager(WorldServer[]) -- the player data storage and the border listener of the overworld.
    void setPlayerManager(const std::vector<WorldServer*>& worldServers);
    void preparePlayer(EntityPlayerMP* playerIn, WorldServer* worldIn);
    int getEntityViewDistance() const;
    std::shared_ptr<NBTTagCompound> readPlayerDataFromFile(EntityPlayerMP* playerIn);
    void playerLoggedIn(const std::shared_ptr<EntityPlayerMP>& playerIn);
    void serverUpdateMountedMovingPlayer(EntityPlayerMP* playerIn);
    void playerLoggedOut(EntityPlayerMP* playerIn);
    // Null when the profile may join, else the reason it may not.
    virtual std::optional<std::string> allowUserToConnect(const GameProfile& profile);
    std::shared_ptr<EntityPlayerMP> createPlayerForUser(const GameProfile& profile);
    std::shared_ptr<EntityPlayerMP> recreatePlayerEntity(EntityPlayerMP* playerIn, int dimension, bool conqueredEnd);
    // Java: moves the player through a portal -- the Nether and the End worlds come with stage 12.
    void transferPlayerToDimension(EntityPlayerMP* playerIn, int dimension);
    void onTick();
    void sendPacketToAllPlayers(const std::shared_ptr<Packet>& packetIn);
    void sendPacketToAllPlayersInDimension(const std::shared_ptr<Packet>& packetIn, int dimension);
    std::vector<std::string> getAllUsernames() const;
    std::vector<GameProfile> getAllProfiles() const;
    bool canJoin(const GameProfile&) const { return true; }
    bool canSendCommands(const GameProfile& profile) const;
    EntityPlayerMP* getPlayerByUsername(const std::string& username) const;
    void sendToAllNear(double x, double y, double z, double radius, int dimension, const std::shared_ptr<Packet>& packetIn);
    void sendToAllNearExcept(EntityPlayer* except, double x, double y, double z, double radius, int dimension, const std::shared_ptr<Packet>& packetIn);
    void saveAllPlayerData();
    void updateTimeAndWeatherForPlayer(EntityPlayerMP* playerIn, WorldServer* worldIn);
    void syncPlayerInventory(EntityPlayerMP* playerIn);
    int getCurrentPlayerCount() const { return static_cast<int>(playerEntityList.size()); }
    int getMaxPlayers() const { return maxPlayers; }
    std::vector<std::string> getAvailablePlayerDat() const;
    int getViewDistance() const { return viewDistance; }
    MinecraftServer* getServerInstance() const { return mcServer; }
    virtual std::shared_ptr<NBTTagCompound> getHostPlayerData() { return nullptr; }
    void setGameType(GameType gameTypeIn) { gameType = gameTypeIn; }
    void setCommandsAllowedForAll(bool allowed) { commandsAllowedForAll = allowed; }
    void removeAllPlayers();
    void sendChatMsgImpl(const std::shared_ptr<IChatComponent>& component, bool isChat);
    void sendChatMsg(const std::shared_ptr<IChatComponent>& component) { sendChatMsgImpl(component, true); }
    void setViewDistance(int distance);
    const std::vector<std::shared_ptr<EntityPlayerMP>>& getPlayerList() const { return playerEntityList; }
    EntityPlayerMP* getPlayerByUUID(const UUID& playerUUID) const;
    bool bypassesPlayerLimit(const GameProfile&) const { return false; }

protected:
    virtual void writePlayerData(EntityPlayerMP* playerIn);

    int maxPlayers = 8;

private:
    class BorderListener;
    struct UuidHash {
        size_t operator()(const UUID& id) const { return static_cast<size_t>(id.hashCode()); }
    };

    void setPlayerGameTypeBasedOnOther(EntityPlayerMP* target, EntityPlayerMP* source, WorldServer* worldIn);
    std::vector<EntityPlayerMP*> playerPointers() const;
    void removeFromList(EntityPlayerMP* playerIn);

    MinecraftServer* const mcServer;
    std::vector<std::shared_ptr<EntityPlayerMP>> playerEntityList;
    std::unordered_map<UUID, EntityPlayerMP*, UuidHash> uuidToPlayerMap;
    IPlayerFileData* playerNBTManagerObj = nullptr;
    std::unique_ptr<BorderListener> borderListener;
    int viewDistance = 0;
    std::optional<GameType> gameType;
    bool commandsAllowedForAll = false;
    int playerPingIndex = 0;
};

// Original: net/minecraft/server/integrated/IntegratedPlayerList.java
class IntegratedPlayerList : public ServerConfigurationManager {
public:
    explicit IntegratedPlayerList(MinecraftServer* server);

    std::optional<std::string> allowUserToConnect(const GameProfile& profile) override;
    std::shared_ptr<NBTTagCompound> getHostPlayerData() override { return hostPlayerData; }

protected:
    void writePlayerData(EntityPlayerMP* playerIn) override;

private:
    std::shared_ptr<NBTTagCompound> hostPlayerData;
};

} // namespace mc

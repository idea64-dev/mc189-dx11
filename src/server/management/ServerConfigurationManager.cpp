#include "server/management/ServerConfigurationManager.h"

#include "core/JavaString.h"
#include "core/Log.h"
#include "entity/EntityTracker.h"
#include "entity/player/EntityPlayerMP.h"
#include "entity/player/InventoryPlayer.h"
#include "nbt/NBTTagCompound.h"
#include "network/NetHandlerPlayServer.h"
#include "network/NetworkManager.h"
#include "network/play/ServerPlayPackets.h"
#include "potion/Potion.h"
#include "server/MinecraftServer.h"
#include "server/management/ItemInWorldManager.h"
#include "server/management/PlayerManager.h"
#include "util/ChatComponent.h"
#include "util/MathHelper.h"
#include "world/GameRules.h"
#include "world/WorldProvider.h"
#include "world/WorldServer.h"
#include "world/border/WorldBorder.h"
#include "world/gen/ChunkProviderServer.h"
#include "world/storage/ISaveHandler.h"
#include "world/storage/WorldInfo.h"

#include <algorithm>

namespace mc {

class ServerConfigurationManager::BorderListener : public IBorderListener {
public:
    explicit BorderListener(ServerConfigurationManager& managerIn)
        : manager(managerIn)
    {
    }

    void onSizeChanged(WorldBorder& border, double) override { send(border, S44PacketWorldBorder::Action::SET_SIZE); }
    void onTransitionStarted(WorldBorder& border, double, double, int64_t) override { send(border, S44PacketWorldBorder::Action::LERP_SIZE); }
    void onCenterChanged(WorldBorder& border, double, double) override { send(border, S44PacketWorldBorder::Action::SET_CENTER); }
    void onWarningTimeChanged(WorldBorder& border, int) override { send(border, S44PacketWorldBorder::Action::SET_WARNING_TIME); }
    void onWarningDistanceChanged(WorldBorder& border, int) override { send(border, S44PacketWorldBorder::Action::SET_WARNING_BLOCKS); }
    void onDamageAmountChanged(WorldBorder&, double) override { }
    void onDamageBufferChanged(WorldBorder&, double) override { }

private:
    void send(WorldBorder& border, S44PacketWorldBorder::Action action) { manager.sendPacketToAllPlayers(std::make_shared<S44PacketWorldBorder>(border, action)); }

    ServerConfigurationManager& manager;
};

ServerConfigurationManager::ServerConfigurationManager(MinecraftServer* server)
    : mcServer(server)
{
}

ServerConfigurationManager::~ServerConfigurationManager() = default;

std::vector<EntityPlayerMP*> ServerConfigurationManager::playerPointers() const
{
    std::vector<EntityPlayerMP*> list;
    list.reserve(playerEntityList.size());
    for (const auto& player : playerEntityList)
        list.push_back(player.get());
    return list;
}

void ServerConfigurationManager::removeFromList(EntityPlayerMP* playerIn)
{
    const auto it = std::find_if(playerEntityList.begin(), playerEntityList.end(), [playerIn](const auto& player) { return player.get() == playerIn; });
    if (it != playerEntityList.end())
        playerEntityList.erase(it);
}

void ServerConfigurationManager::initializeConnectionToPlayer(const std::shared_ptr<NetworkManager>& netManager, const std::shared_ptr<EntityPlayerMP>& playerIn)
{
    const GameProfile& gameprofile = playerIn->getGameProfile();
    // Java: the name last seen for this id in the profile cache (usercache.json) -- the cache comes with multiplayer.
    const std::string s = gameprofile.getName();
    const std::shared_ptr<NBTTagCompound> nbttagcompound = readPlayerDataFromFile(playerIn.get());
    playerIn->setWorld(mcServer->worldServerForDimension(playerIn->dimension));
    playerIn->theItemInWorldManager->setWorld(static_cast<WorldServer*>(playerIn->worldObj));
    const std::string s1 = netManager->getRemoteAddress();
    Log::info("{}[{}] logged in with entity id {} at ({}, {}, {})", playerIn->getName(), s1, playerIn->getEntityId(), java::toString(playerIn->posX),
        java::toString(playerIn->posY), java::toString(playerIn->posZ));
    WorldServer* worldserver = mcServer->worldServerForDimension(playerIn->dimension);
    WorldInfo& worldinfo = worldserver->getWorldInfo();
    const BlockPos blockpos = worldserver->getSpawnPoint();
    setPlayerGameTypeBasedOnOther(playerIn.get(), nullptr, worldserver);
    const std::shared_ptr<NetHandlerPlayServer> nethandlerplayserver = NetHandlerPlayServer::create(mcServer, netManager, playerIn);
    nethandlerplayserver->sendPacket(std::make_shared<S01PacketJoinGame>(playerIn->getEntityId(), playerIn->theItemInWorldManager->getGameType(),
        worldinfo.isHardcoreModeEnabled(), worldserver->provider->getDimensionId(), worldserver->getDifficulty(), getMaxPlayers(), worldinfo.getTerrainType(),
        worldserver->getGameRules().getBoolean("reducedDebugInfo")));
    // Java: writeString -- a VarInt length and the UTF-8 bytes.
    {
        const std::string brand = mcServer->getServerModName();
        std::vector<uint8_t> data;
        uint32_t length = static_cast<uint32_t>(brand.size());
        while ((length & ~0x7Fu) != 0) {
            data.push_back(static_cast<uint8_t>((length & 0x7F) | 0x80));
            length >>= 7;
        }
        data.push_back(static_cast<uint8_t>(length));
        data.insert(data.end(), brand.begin(), brand.end());
        nethandlerplayserver->sendPacket(std::make_shared<S3FPacketCustomPayload>("MC|Brand", std::move(data)));
    }
    nethandlerplayserver->sendPacket(std::make_shared<S41PacketServerDifficulty>(worldserver->getDifficulty(), worldinfo.isDifficultyLocked()));
    nethandlerplayserver->sendPacket(std::make_shared<S05PacketSpawnPosition>(blockpos));
    nethandlerplayserver->sendPacket(std::make_shared<S39PacketPlayerAbilities>(playerIn->capabilities));
    nethandlerplayserver->sendPacket(std::make_shared<S09PacketHeldItemChange>(playerIn->inventory->currentItem));
    // Java: the statistics file sends the achievements, and the scoreboard is sent -- stage 12.

    std::shared_ptr<ChatComponentTranslation> chatcomponenttranslation;
    if (java::toLowerCase(playerIn->getName()) != java::toLowerCase(s))
        chatcomponenttranslation = std::make_shared<ChatComponentTranslation>("multiplayer.player.joined.renamed",
            std::vector<ChatComponentTranslation::Argument> { playerIn->getDisplayName(), s });
    else
        chatcomponenttranslation = std::make_shared<ChatComponentTranslation>("multiplayer.player.joined",
            std::vector<ChatComponentTranslation::Argument> { playerIn->getDisplayName() });
    chatcomponenttranslation->getChatStyle()->setColor('e');
    sendChatMsg(chatcomponenttranslation);
    playerLoggedIn(playerIn);
    nethandlerplayserver->setPlayerLocation(playerIn->posX, playerIn->posY, playerIn->posZ, playerIn->rotationYaw, playerIn->rotationPitch);
    updateTimeAndWeatherForPlayer(playerIn.get(), worldserver);

    if (!mcServer->getResourcePackUrl().empty())
        playerIn->loadResourcePack(mcServer->getResourcePackUrl(), mcServer->getResourcePackHash());

    for (const PotionEffect& potioneffect : playerIn->getActivePotionEffects())
        nethandlerplayserver->sendPacket(std::make_shared<S1DPacketEntityEffect>(playerIn->getEntityId(), potioneffect));

    playerIn->addSelfToInternalCraftingInventory();
    // Java: the mount saved under "Riding" respawns with the player -- the rideable entities, stage 10.
    (void)nbttagcompound;
}

void ServerConfigurationManager::setPlayerManager(const std::vector<WorldServer*>& worldServers)
{
    playerNBTManagerObj = worldServers[0]->getSaveHandler()->getPlayerNBTManager();
    borderListener = std::make_unique<BorderListener>(*this);
    worldServers[0]->getWorldBorder().addListener(borderListener.get());
}

void ServerConfigurationManager::preparePlayer(EntityPlayerMP* playerIn, WorldServer* worldIn)
{
    WorldServer* worldserver = playerIn->getServerForPlayer();
    if (worldIn != nullptr)
        worldIn->getPlayerManager()->removePlayer(playerIn);
    worldserver->getPlayerManager()->addPlayer(playerIn);
    worldserver->theChunkProviderServer->loadChunk(java::d2i(playerIn->posX) >> 4, java::d2i(playerIn->posZ) >> 4);
}

int ServerConfigurationManager::getEntityViewDistance() const
{
    return PlayerManager::getFurthestViewableBlock(getViewDistance());
}

std::shared_ptr<NBTTagCompound> ServerConfigurationManager::readPlayerDataFromFile(EntityPlayerMP* playerIn)
{
    const std::shared_ptr<NBTTagCompound> nbttagcompound = mcServer->worldServers[0]->getWorldInfo().getPlayerNBTTagCompound();
    if (playerIn->getName() == mcServer->getServerOwner() && nbttagcompound) {
        playerIn->readFromNBT(*nbttagcompound);
        Log::debug("loading single player");
        return nbttagcompound;
    }
    return playerNBTManagerObj->readPlayerData(playerIn);
}

void ServerConfigurationManager::writePlayerData(EntityPlayerMP* playerIn)
{
    playerNBTManagerObj->writePlayerData(playerIn);
    // Java: the player's statistics file is saved too -- stage 12.
}

void ServerConfigurationManager::playerLoggedIn(const std::shared_ptr<EntityPlayerMP>& playerIn)
{
    playerEntityList.push_back(playerIn);
    uuidToPlayerMap[playerIn->getUniqueID()] = playerIn.get();
    sendPacketToAllPlayers(std::make_shared<S38PacketPlayerListItem>(S38PacketPlayerListItem::Action::ADD_PLAYER, std::vector<EntityPlayerMP*> { playerIn.get() }));
    WorldServer* worldserver = mcServer->worldServerForDimension(playerIn->dimension);
    worldserver->spawnEntityInWorld(playerIn);
    preparePlayer(playerIn.get(), nullptr);

    for (size_t i = 0; i < playerEntityList.size(); ++i) {
        EntityPlayerMP* entityplayermp = playerEntityList[i].get();
        playerIn->playerNetServerHandler->sendPacket(
            std::make_shared<S38PacketPlayerListItem>(S38PacketPlayerListItem::Action::ADD_PLAYER, std::vector<EntityPlayerMP*> { entityplayermp }));
    }
}

void ServerConfigurationManager::serverUpdateMountedMovingPlayer(EntityPlayerMP* playerIn)
{
    playerIn->getServerForPlayer()->getPlayerManager()->updateMountedMovingPlayer(playerIn);
}

void ServerConfigurationManager::playerLoggedOut(EntityPlayerMP* playerIn)
{
    // Java: triggerAchievement(StatList.leaveGameStat) -- stage 12.
    writePlayerData(playerIn);
    WorldServer* worldserver = playerIn->getServerForPlayer();

    if (playerIn->ridingEntity != nullptr) {
        worldserver->removePlayerEntityDangerously(playerIn->ridingEntity.get());
        Log::debug("removing player mount");
    }

    // Keep the player alive until it has left every list.
    const std::shared_ptr<EntityPlayerMP> keepAlive = std::static_pointer_cast<EntityPlayerMP>(playerIn->shared_from_this());
    worldserver->removeEntity(playerIn);
    worldserver->getPlayerManager()->removePlayer(playerIn);
    removeFromList(playerIn);
    const UUID uuid = playerIn->getUniqueID();
    if (const auto it = uuidToPlayerMap.find(uuid); it != uuidToPlayerMap.end() && it->second == playerIn)
        uuidToPlayerMap.erase(it);
    sendPacketToAllPlayers(std::make_shared<S38PacketPlayerListItem>(S38PacketPlayerListItem::Action::REMOVE_PLAYER, std::vector<EntityPlayerMP*> { playerIn }));
}

std::optional<std::string> ServerConfigurationManager::allowUserToConnect(const GameProfile& profile)
{
    // Java: the ban lists and the white list come first -- multiplayer.
    if (static_cast<int>(playerEntityList.size()) >= maxPlayers && !bypassesPlayerLimit(profile))
        return std::string("The server is full!");
    return std::nullopt;
}

std::shared_ptr<EntityPlayerMP> ServerConfigurationManager::createPlayerForUser(const GameProfile& profile)
{
    // Java: EntityPlayer.getUUID(profile)
    const UUID uuid = profile.getId() ? *profile.getId() : UUID::nameUUIDFromBytes("OfflinePlayer:" + profile.getName());
    std::vector<EntityPlayerMP*> list;

    for (const auto& entityplayermp : playerEntityList) {
        if (entityplayermp->getUniqueID() == uuid)
            list.push_back(entityplayermp.get());
    }

    if (profile.getId()) {
        if (EntityPlayerMP* entityplayermp2 = getPlayerByUUID(*profile.getId()); entityplayermp2 && std::find(list.begin(), list.end(), entityplayermp2) == list.end())
            list.push_back(entityplayermp2);
    }

    for (EntityPlayerMP* entityplayermp1 : list)
        entityplayermp1->playerNetServerHandler->kickPlayerFromServer("You logged in from another location");

    // Java: a DemoWorldManager for demo servers -- the demo is out of the project.
    WorldServer* world = mcServer->worldServerForDimension(0);
    return Entity::create<EntityPlayerMP>(mcServer, world, profile, std::make_unique<ItemInWorldManager>(world));
}

std::shared_ptr<EntityPlayerMP> ServerConfigurationManager::recreatePlayerEntity(EntityPlayerMP* playerIn, int dimension, bool conqueredEnd)
{
    const std::shared_ptr<EntityPlayerMP> oldPlayer = std::static_pointer_cast<EntityPlayerMP>(playerIn->shared_from_this());
    playerIn->getServerForPlayer()->getEntityTracker()->removePlayerFromTrackers(playerIn);
    playerIn->getServerForPlayer()->getEntityTracker()->untrackEntity(playerIn);
    playerIn->getServerForPlayer()->getPlayerManager()->removePlayer(playerIn);
    removeFromList(playerIn);
    mcServer->worldServerForDimension(playerIn->dimension)->removePlayerEntityDangerously(playerIn);
    const std::optional<BlockPos> blockpos = playerIn->getBedLocation();
    const bool flag = playerIn->isSpawnForced();
    playerIn->dimension = dimension;
    WorldServer* world = mcServer->worldServerForDimension(playerIn->dimension);
    const std::shared_ptr<EntityPlayerMP> entityplayermp
        = Entity::create<EntityPlayerMP>(mcServer, world, playerIn->getGameProfile(), std::make_unique<ItemInWorldManager>(world));
    entityplayermp->playerNetServerHandler = playerIn->playerNetServerHandler;
    entityplayermp->clonePlayer(playerIn, conqueredEnd);
    entityplayermp->setEntityId(playerIn->getEntityId());
    // Java: setCommandStats(playerIn) -- the command stats, stage 12.
    WorldServer* worldserver = mcServer->worldServerForDimension(playerIn->dimension);
    setPlayerGameTypeBasedOnOther(entityplayermp.get(), playerIn, worldserver);

    if (blockpos) {
        const std::optional<BlockPos> blockpos1 = EntityPlayer::getBedSpawnLocation(mcServer->worldServerForDimension(playerIn->dimension), *blockpos, flag);
        if (blockpos1) {
            entityplayermp->setLocationAndAngles(static_cast<double>(static_cast<float>(blockpos1->getX()) + 0.5f),
                static_cast<double>(static_cast<float>(blockpos1->getY()) + 0.1f), static_cast<double>(static_cast<float>(blockpos1->getZ()) + 0.5f), 0.0f, 0.0f);
            entityplayermp->setSpawnPoint(*blockpos, flag);
        } else {
            entityplayermp->playerNetServerHandler->sendPacket(std::make_shared<S2BPacketChangeGameState>(0, 0.0f));
        }
    }

    worldserver->theChunkProviderServer->loadChunk(java::d2i(entityplayermp->posX) >> 4, java::d2i(entityplayermp->posZ) >> 4);

    while (!worldserver->getCollidingBoundingBoxes(entityplayermp.get(), entityplayermp->getEntityBoundingBox()).empty() && entityplayermp->posY < 256.0)
        entityplayermp->setPosition(entityplayermp->posX, entityplayermp->posY + 1.0, entityplayermp->posZ);

    entityplayermp->playerNetServerHandler->sendPacket(std::make_shared<S07PacketRespawn>(entityplayermp->dimension, entityplayermp->worldObj->getDifficulty(),
        entityplayermp->worldObj->getWorldInfo().getTerrainType(), entityplayermp->theItemInWorldManager->getGameType()));
    const BlockPos blockpos2 = worldserver->getSpawnPoint();
    entityplayermp->playerNetServerHandler->setPlayerLocation(
        entityplayermp->posX, entityplayermp->posY, entityplayermp->posZ, entityplayermp->rotationYaw, entityplayermp->rotationPitch);
    entityplayermp->playerNetServerHandler->sendPacket(std::make_shared<S05PacketSpawnPosition>(blockpos2));
    entityplayermp->playerNetServerHandler->sendPacket(
        std::make_shared<S1FPacketSetExperience>(entityplayermp->experience, entityplayermp->experienceTotal, entityplayermp->experienceLevel));
    updateTimeAndWeatherForPlayer(entityplayermp.get(), worldserver);
    worldserver->getPlayerManager()->addPlayer(entityplayermp.get());
    worldserver->spawnEntityInWorld(entityplayermp);
    playerEntityList.push_back(entityplayermp);
    uuidToPlayerMap[entityplayermp->getUniqueID()] = entityplayermp.get();
    entityplayermp->addSelfToInternalCraftingInventory();
    entityplayermp->setHealth(entityplayermp->getHealth());
    return entityplayermp;
}

void ServerConfigurationManager::transferPlayerToDimension(EntityPlayerMP* playerIn, int dimension)
{
    Log::warn("{} can't travel to dimension {}: only the overworld exists so far", playerIn->getName(), dimension);
}

void ServerConfigurationManager::onTick()
{
    if (++playerPingIndex > 600) {
        sendPacketToAllPlayers(std::make_shared<S38PacketPlayerListItem>(S38PacketPlayerListItem::Action::UPDATE_LATENCY, playerPointers()));
        playerPingIndex = 0;
    }
}

void ServerConfigurationManager::sendPacketToAllPlayers(const std::shared_ptr<Packet>& packetIn)
{
    for (size_t i = 0; i < playerEntityList.size(); ++i)
        playerEntityList[i]->playerNetServerHandler->sendPacket(packetIn);
}

void ServerConfigurationManager::sendPacketToAllPlayersInDimension(const std::shared_ptr<Packet>& packetIn, int dimension)
{
    for (size_t i = 0; i < playerEntityList.size(); ++i) {
        EntityPlayerMP* entityplayermp = playerEntityList[i].get();
        if (entityplayermp->dimension == dimension)
            entityplayermp->playerNetServerHandler->sendPacket(packetIn);
    }
}

std::vector<std::string> ServerConfigurationManager::getAllUsernames() const
{
    std::vector<std::string> names;
    for (const auto& player : playerEntityList)
        names.push_back(player->getName());
    return names;
}

std::vector<GameProfile> ServerConfigurationManager::getAllProfiles() const
{
    std::vector<GameProfile> profiles;
    for (const auto& player : playerEntityList)
        profiles.push_back(player->getGameProfile());
    return profiles;
}

bool ServerConfigurationManager::canSendCommands(const GameProfile& profile) const
{
    // Java: ops.hasEntry(profile) first -- the op list comes with multiplayer.
    return (mcServer->isSinglePlayer() && mcServer->worldServers[0]->getWorldInfo().areCommandsAllowed()
               && java::toLowerCase(mcServer->getServerOwner()) == java::toLowerCase(profile.getName()))
        || commandsAllowedForAll;
}

EntityPlayerMP* ServerConfigurationManager::getPlayerByUsername(const std::string& username) const
{
    const std::string lower = java::toLowerCase(username);
    for (const auto& entityplayermp : playerEntityList) {
        if (java::toLowerCase(entityplayermp->getName()) == lower)
            return entityplayermp.get();
    }
    return nullptr;
}

void ServerConfigurationManager::sendToAllNear(double x, double y, double z, double radius, int dimension, const std::shared_ptr<Packet>& packetIn)
{
    sendToAllNearExcept(nullptr, x, y, z, radius, dimension, packetIn);
}

void ServerConfigurationManager::sendToAllNearExcept(
    EntityPlayer* except, double x, double y, double z, double radius, int dimension, const std::shared_ptr<Packet>& packetIn)
{
    for (size_t i = 0; i < playerEntityList.size(); ++i) {
        EntityPlayerMP* entityplayermp = playerEntityList[i].get();
        if (entityplayermp != except && entityplayermp->dimension == dimension) {
            const double d0 = x - entityplayermp->posX;
            const double d1 = y - entityplayermp->posY;
            const double d2 = z - entityplayermp->posZ;
            if (d0 * d0 + d1 * d1 + d2 * d2 < radius * radius)
                entityplayermp->playerNetServerHandler->sendPacket(packetIn);
        }
    }
}

void ServerConfigurationManager::saveAllPlayerData()
{
    for (size_t i = 0; i < playerEntityList.size(); ++i)
        writePlayerData(playerEntityList[i].get());
}

void ServerConfigurationManager::updateTimeAndWeatherForPlayer(EntityPlayerMP* playerIn, WorldServer* worldIn)
{
    WorldBorder& worldborder = mcServer->worldServers[0]->getWorldBorder();
    playerIn->playerNetServerHandler->sendPacket(std::make_shared<S44PacketWorldBorder>(worldborder, S44PacketWorldBorder::Action::INITIALIZE));
    playerIn->playerNetServerHandler->sendPacket(
        std::make_shared<S03PacketTimeUpdate>(worldIn->getTotalWorldTime(), worldIn->getWorldTime(), worldIn->getGameRules().getBoolean("doDaylightCycle")));

    if (worldIn->isRaining()) {
        playerIn->playerNetServerHandler->sendPacket(std::make_shared<S2BPacketChangeGameState>(1, 0.0f));
        playerIn->playerNetServerHandler->sendPacket(std::make_shared<S2BPacketChangeGameState>(7, worldIn->getRainStrength(1.0f)));
        playerIn->playerNetServerHandler->sendPacket(std::make_shared<S2BPacketChangeGameState>(8, worldIn->getThunderStrength(1.0f)));
    }
}

void ServerConfigurationManager::syncPlayerInventory(EntityPlayerMP* playerIn)
{
    playerIn->sendContainerToPlayer(playerIn->inventoryContainer.get());
    playerIn->setPlayerHealthUpdated();
    playerIn->playerNetServerHandler->sendPacket(std::make_shared<S09PacketHeldItemChange>(playerIn->inventory->currentItem));
}

std::vector<std::string> ServerConfigurationManager::getAvailablePlayerDat() const
{
    return mcServer->worldServers[0]->getSaveHandler()->getPlayerNBTManager()->getAvailablePlayerDat();
}

void ServerConfigurationManager::setPlayerGameTypeBasedOnOther(EntityPlayerMP* target, EntityPlayerMP* source, WorldServer* worldIn)
{
    if (source != nullptr)
        target->theItemInWorldManager->setGameType(source->theItemInWorldManager->getGameType());
    else if (gameType)
        target->theItemInWorldManager->setGameType(*gameType);

    target->theItemInWorldManager->initializeGameType(worldIn->getWorldInfo().getGameType());
}

void ServerConfigurationManager::removeAllPlayers()
{
    for (size_t i = 0; i < playerEntityList.size(); ++i)
        playerEntityList[i]->playerNetServerHandler->kickPlayerFromServer("Server closed");
}

void ServerConfigurationManager::sendChatMsgImpl(const std::shared_ptr<IChatComponent>& component, bool isChat)
{
    mcServer->addChatMessage(component);
    sendPacketToAllPlayers(std::make_shared<S02PacketChat>(component, static_cast<int8_t>(isChat ? 1 : 0)));
}

void ServerConfigurationManager::setViewDistance(int distance)
{
    viewDistance = distance;
    for (const auto& worldserver : mcServer->worldServers) {
        if (worldserver)
            worldserver->getPlayerManager()->setPlayerViewRadius(distance);
    }
}

EntityPlayerMP* ServerConfigurationManager::getPlayerByUUID(const UUID& playerUUID) const
{
    const auto it = uuidToPlayerMap.find(playerUUID);
    return it == uuidToPlayerMap.end() ? nullptr : it->second;
}

IntegratedPlayerList::IntegratedPlayerList(MinecraftServer* server)
    : ServerConfigurationManager(server)
{
    setViewDistance(10);
}

std::optional<std::string> IntegratedPlayerList::allowUserToConnect(const GameProfile& profile)
{
    if (java::toLowerCase(profile.getName()) == java::toLowerCase(getServerInstance()->getServerOwner()) && getPlayerByUsername(profile.getName()) != nullptr)
        return std::string("That name is already taken.");
    return ServerConfigurationManager::allowUserToConnect(profile);
}

void IntegratedPlayerList::writePlayerData(EntityPlayerMP* playerIn)
{
    if (playerIn->getName() == getServerInstance()->getServerOwner()) {
        hostPlayerData = std::make_shared<NBTTagCompound>();
        playerIn->writeToNBT(*hostPlayerData);
    }
    ServerConfigurationManager::writePlayerData(playerIn);
}

} // namespace mc

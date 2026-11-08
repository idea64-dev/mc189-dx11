#pragma once

// Original: net/minecraft/client/network/NetHandlerPlayClient.java
// Threading: NetworkManager::processInboundPackets() runs on the client thread where Java drains the scheduled
// tasks, so the handlers run their bodies directly (Java's PacketThreadUtil.checkThreadAndEnqueue re-schedules
// them onto that thread).
// Spawning entities the server sends needs their classes: until the objects and mobs exist (stage 10) their spawn
// packets are logged and dropped. Not ported yet: maps (S34), statistics (S37), the scoreboard (S3B-S3E), trading
// (MC|TrList), books (MC|BOpen), server resource packs (S48 -- declined), signs (TileEntitySign), container windows
// other than the player's (S2D) and the Twitch metadata of S42.

#include "auth/GameProfile.h"
#include "core/JavaHashMap.h"
#include "entity/ai/attributes/Attributes.h" // JavaHash<UUID>
#include "network/play/INetHandlerPlayClient.h"
#include "util/Random.h"
#include "util/UUID.h"

#include <memory>
#include <string>
#include <vector>

namespace mc {

class GuiScreen;
class Minecraft;
class NetworkManager;
class NetworkPlayerInfo;
class Packet;
class WorldClient;

class NetHandlerPlayClient : public INetHandlerPlayClient {
public:
    // The network manager owns its handler; the handler only points back at it.
    NetHandlerPlayClient(Minecraft* mcIn, std::shared_ptr<GuiScreen> guiScreen, NetworkManager* networkManager, GameProfile profileIn);
    ~NetHandlerPlayClient() override;

    void cleanup() { clientWorldController = nullptr; }

    void handleJoinGame(S01PacketJoinGame& packetIn) override;
    void handleSpawnObject(S0EPacketSpawnObject& packetIn) override;
    void handleSpawnExperienceOrb(S11PacketSpawnExperienceOrb& packetIn) override;
    void handleSpawnGlobalEntity(S2CPacketSpawnGlobalEntity& packetIn) override;
    void handleSpawnPainting(S10PacketSpawnPainting& packetIn) override;
    void handleEntityVelocity(S12PacketEntityVelocity& packetIn) override;
    void handleEntityMetadata(S1CPacketEntityMetadata& packetIn) override;
    void handleSpawnPlayer(S0CPacketSpawnPlayer& packetIn) override;
    void handleEntityTeleport(S18PacketEntityTeleport& packetIn) override;
    void handleHeldItemChange(S09PacketHeldItemChange& packetIn) override;
    void handleEntityMovement(S14PacketEntity& packetIn) override;
    void handleEntityHeadLook(S19PacketEntityHeadLook& packetIn) override;
    void handleDestroyEntities(S13PacketDestroyEntities& packetIn) override;
    void handlePlayerPosLook(S08PacketPlayerPosLook& packetIn) override;
    void handleMultiBlockChange(S22PacketMultiBlockChange& packetIn) override;
    void handleChunkData(S21PacketChunkData& packetIn) override;
    void handleBlockChange(S23PacketBlockChange& packetIn) override;
    void handleDisconnect(S40PacketDisconnect& packetIn) override;
    void onDisconnect(const std::shared_ptr<IChatComponent>& reason) override;
    void addToSendQueue(std::shared_ptr<Packet> packet);
    void handleCollectItem(S0DPacketCollectItem& packetIn) override;
    void handleChat(S02PacketChat& packetIn) override;
    void handleAnimation(S0BPacketAnimation& packetIn) override;
    void handleUseBed(S0APacketUseBed& packetIn) override;
    void handleSpawnMob(S0FPacketSpawnMob& packetIn) override;
    void handleTimeUpdate(S03PacketTimeUpdate& packetIn) override;
    void handleSpawnPosition(S05PacketSpawnPosition& packetIn) override;
    void handleEntityAttach(S1BPacketEntityAttach& packetIn) override;
    void handleEntityStatus(S19PacketEntityStatus& packetIn) override;
    void handleUpdateHealth(S06PacketUpdateHealth& packetIn) override;
    void handleSetExperience(S1FPacketSetExperience& packetIn) override;
    void handleRespawn(S07PacketRespawn& packetIn) override;
    void handleExplosion(S27PacketExplosion& packetIn) override;
    void handleOpenWindow(S2DPacketOpenWindow& packetIn) override;
    void handleSetSlot(S2FPacketSetSlot& packetIn) override;
    void handleConfirmTransaction(S32PacketConfirmTransaction& packetIn) override;
    void handleWindowItems(S30PacketWindowItems& packetIn) override;
    void handleSignEditorOpen(S36PacketSignEditorOpen& packetIn) override;
    void handleUpdateSign(S33PacketUpdateSign& packetIn) override;
    void handleUpdateTileEntity(S35PacketUpdateTileEntity& packetIn) override;
    void handleWindowProperty(S31PacketWindowProperty& packetIn) override;
    void handleEntityEquipment(S04PacketEntityEquipment& packetIn) override;
    void handleCloseWindow(S2EPacketCloseWindow& packetIn) override;
    void handleBlockAction(S24PacketBlockAction& packetIn) override;
    void handleBlockBreakAnim(S25PacketBlockBreakAnim& packetIn) override;
    void handleMapChunkBulk(S26PacketMapChunkBulk& packetIn) override;
    void handleChangeGameState(S2BPacketChangeGameState& packetIn) override;
    void handleEffect(S28PacketEffect& packetIn) override;
    void handleEntityEffect(S1DPacketEntityEffect& packetIn) override;
    void handleCombatEvent(S42PacketCombatEvent& packetIn) override;
    void handleServerDifficulty(S41PacketServerDifficulty& packetIn) override;
    void handleCamera(S43PacketCamera& packetIn) override;
    void handleWorldBorder(S44PacketWorldBorder& packetIn) override;
    void handleTitle(S45PacketTitle& packetIn) override;
    void handleSetCompressionLevel(S46PacketSetCompressionLevel& packetIn) override;
    void handlePlayerListHeaderFooter(S47PacketPlayerListHeaderFooter& packetIn) override;
    void handleRemoveEntityEffect(S1EPacketRemoveEntityEffect& packetIn) override;
    void handlePlayerListItem(S38PacketPlayerListItem& packetIn) override;
    void handleKeepAlive(S00PacketKeepAlive& packetIn) override;
    void handlePlayerAbilities(S39PacketPlayerAbilities& packetIn) override;
    void handleTabComplete(S3APacketTabComplete& packetIn) override;
    void handleSoundEffect(S29PacketSoundEffect& packetIn) override;
    void handleResourcePack(S48PacketResourcePackSend& packetIn) override;
    void handleEntityNBT(S49PacketUpdateEntityNBT& packetIn) override;
    void handleCustomPayload(S3FPacketCustomPayload& packetIn) override;
    void handleParticles(S2APacketParticles& packetIn) override;
    void handleEntityProperties(S20PacketEntityProperties& packetIn) override;

    NetworkManager* getNetworkManager() const { return netManager; }
    // Java: playerInfoMap.values(), in the map's order.
    std::vector<NetworkPlayerInfo*> getPlayerInfoMap() const;
    // Null when the server has not listed the player.
    NetworkPlayerInfo* getPlayerInfo(const UUID& id) const;
    NetworkPlayerInfo* getPlayerInfo(const std::string& name) const;
    const GameProfile& getGameProfile() const { return profile; }

    int currentServerMaxPlayers = 20;

private:
    NetworkManager* netManager;
    GameProfile profile;
    std::shared_ptr<GuiScreen> guiScreenServer;
    Minecraft* gameController;
    WorldClient* clientWorldController = nullptr;
    bool doneLoadingTerrain = false;
    JavaHashMap<UUID, std::unique_ptr<NetworkPlayerInfo>> playerInfoMap;
    bool field_147308_k = false;
    Random avRandomizer;
};

} // namespace mc

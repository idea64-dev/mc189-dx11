#pragma once

// Original: net/minecraft/network/play/INetHandlerPlayClient.java
// Not ported yet: the scoreboard packets (S3B..S3E), maps (S34) and statistics (S37) -- stage 12.

#include "network/Packet.h"

namespace mc {

class S00PacketKeepAlive;
class S01PacketJoinGame;
class S02PacketChat;
class S03PacketTimeUpdate;
class S04PacketEntityEquipment;
class S05PacketSpawnPosition;
class S06PacketUpdateHealth;
class S07PacketRespawn;
class S08PacketPlayerPosLook;
class S09PacketHeldItemChange;
class S0APacketUseBed;
class S0BPacketAnimation;
class S0CPacketSpawnPlayer;
class S0DPacketCollectItem;
class S0EPacketSpawnObject;
class S0FPacketSpawnMob;
class S10PacketSpawnPainting;
class S11PacketSpawnExperienceOrb;
class S12PacketEntityVelocity;
class S13PacketDestroyEntities;
class S14PacketEntity;
class S18PacketEntityTeleport;
class S19PacketEntityHeadLook;
class S19PacketEntityStatus;
class S1BPacketEntityAttach;
class S1CPacketEntityMetadata;
class S1DPacketEntityEffect;
class S1EPacketRemoveEntityEffect;
class S1FPacketSetExperience;
class S20PacketEntityProperties;
class S21PacketChunkData;
class S22PacketMultiBlockChange;
class S23PacketBlockChange;
class S24PacketBlockAction;
class S25PacketBlockBreakAnim;
class S26PacketMapChunkBulk;
class S27PacketExplosion;
class S28PacketEffect;
class S29PacketSoundEffect;
class S2APacketParticles;
class S2BPacketChangeGameState;
class S2CPacketSpawnGlobalEntity;
class S2DPacketOpenWindow;
class S2EPacketCloseWindow;
class S2FPacketSetSlot;
class S30PacketWindowItems;
class S31PacketWindowProperty;
class S32PacketConfirmTransaction;
class S33PacketUpdateSign;
class S35PacketUpdateTileEntity;
class S36PacketSignEditorOpen;
class S38PacketPlayerListItem;
class S39PacketPlayerAbilities;
class S3APacketTabComplete;
class S3FPacketCustomPayload;
class S40PacketDisconnect;
class S41PacketServerDifficulty;
class S42PacketCombatEvent;
class S43PacketCamera;
class S44PacketWorldBorder;
class S45PacketTitle;
class S46PacketSetCompressionLevel;
class S47PacketPlayerListHeaderFooter;
class S48PacketResourcePackSend;
class S49PacketUpdateEntityNBT;

class INetHandlerPlayClient : public INetHandler {
public:
    virtual void handleSpawnObject(S0EPacketSpawnObject& packetIn) = 0;
    virtual void handleSpawnExperienceOrb(S11PacketSpawnExperienceOrb& packetIn) = 0;
    virtual void handleSpawnGlobalEntity(S2CPacketSpawnGlobalEntity& packetIn) = 0;
    virtual void handleSpawnMob(S0FPacketSpawnMob& packetIn) = 0;
    virtual void handleSpawnPainting(S10PacketSpawnPainting& packetIn) = 0;
    virtual void handleSpawnPlayer(S0CPacketSpawnPlayer& packetIn) = 0;
    virtual void handleAnimation(S0BPacketAnimation& packetIn) = 0;
    virtual void handleBlockBreakAnim(S25PacketBlockBreakAnim& packetIn) = 0;
    virtual void handleUpdateTileEntity(S35PacketUpdateTileEntity& packetIn) = 0;
    virtual void handleBlockAction(S24PacketBlockAction& packetIn) = 0;
    virtual void handleBlockChange(S23PacketBlockChange& packetIn) = 0;
    virtual void handleChat(S02PacketChat& packetIn) = 0;
    virtual void handleTabComplete(S3APacketTabComplete& packetIn) = 0;
    virtual void handleMultiBlockChange(S22PacketMultiBlockChange& packetIn) = 0;
    virtual void handleConfirmTransaction(S32PacketConfirmTransaction& packetIn) = 0;
    virtual void handleCloseWindow(S2EPacketCloseWindow& packetIn) = 0;
    virtual void handleWindowItems(S30PacketWindowItems& packetIn) = 0;
    virtual void handleOpenWindow(S2DPacketOpenWindow& packetIn) = 0;
    virtual void handleWindowProperty(S31PacketWindowProperty& packetIn) = 0;
    virtual void handleSetSlot(S2FPacketSetSlot& packetIn) = 0;
    virtual void handleCustomPayload(S3FPacketCustomPayload& packetIn) = 0;
    virtual void handleDisconnect(S40PacketDisconnect& packetIn) = 0;
    virtual void handleUseBed(S0APacketUseBed& packetIn) = 0;
    virtual void handleEntityStatus(S19PacketEntityStatus& packetIn) = 0;
    virtual void handleEntityAttach(S1BPacketEntityAttach& packetIn) = 0;
    virtual void handleExplosion(S27PacketExplosion& packetIn) = 0;
    virtual void handleChangeGameState(S2BPacketChangeGameState& packetIn) = 0;
    virtual void handleKeepAlive(S00PacketKeepAlive& packetIn) = 0;
    virtual void handleChunkData(S21PacketChunkData& packetIn) = 0;
    virtual void handleMapChunkBulk(S26PacketMapChunkBulk& packetIn) = 0;
    virtual void handleEffect(S28PacketEffect& packetIn) = 0;
    virtual void handleJoinGame(S01PacketJoinGame& packetIn) = 0;
    virtual void handleEntityMovement(S14PacketEntity& packetIn) = 0;
    virtual void handlePlayerPosLook(S08PacketPlayerPosLook& packetIn) = 0;
    virtual void handleParticles(S2APacketParticles& packetIn) = 0;
    virtual void handlePlayerAbilities(S39PacketPlayerAbilities& packetIn) = 0;
    virtual void handlePlayerListItem(S38PacketPlayerListItem& packetIn) = 0;
    virtual void handleDestroyEntities(S13PacketDestroyEntities& packetIn) = 0;
    virtual void handleRemoveEntityEffect(S1EPacketRemoveEntityEffect& packetIn) = 0;
    virtual void handleRespawn(S07PacketRespawn& packetIn) = 0;
    virtual void handleEntityHeadLook(S19PacketEntityHeadLook& packetIn) = 0;
    virtual void handleHeldItemChange(S09PacketHeldItemChange& packetIn) = 0;
    virtual void handleEntityMetadata(S1CPacketEntityMetadata& packetIn) = 0;
    virtual void handleEntityVelocity(S12PacketEntityVelocity& packetIn) = 0;
    virtual void handleEntityEquipment(S04PacketEntityEquipment& packetIn) = 0;
    virtual void handleSetExperience(S1FPacketSetExperience& packetIn) = 0;
    virtual void handleUpdateHealth(S06PacketUpdateHealth& packetIn) = 0;
    virtual void handleTimeUpdate(S03PacketTimeUpdate& packetIn) = 0;
    virtual void handleUpdateSign(S33PacketUpdateSign& packetIn) = 0;
    virtual void handleSoundEffect(S29PacketSoundEffect& packetIn) = 0;
    virtual void handleCollectItem(S0DPacketCollectItem& packetIn) = 0;
    virtual void handleEntityTeleport(S18PacketEntityTeleport& packetIn) = 0;
    virtual void handleEntityProperties(S20PacketEntityProperties& packetIn) = 0;
    virtual void handleEntityEffect(S1DPacketEntityEffect& packetIn) = 0;
    virtual void handleCombatEvent(S42PacketCombatEvent& packetIn) = 0;
    virtual void handleServerDifficulty(S41PacketServerDifficulty& packetIn) = 0;
    virtual void handleCamera(S43PacketCamera& packetIn) = 0;
    virtual void handleWorldBorder(S44PacketWorldBorder& packetIn) = 0;
    virtual void handleTitle(S45PacketTitle& packetIn) = 0;
    virtual void handleSetCompressionLevel(S46PacketSetCompressionLevel& packetIn) = 0;
    virtual void handlePlayerListHeaderFooter(S47PacketPlayerListHeaderFooter& packetIn) = 0;
    virtual void handleResourcePack(S48PacketResourcePackSend& packetIn) = 0;
    virtual void handleEntityNBT(S49PacketUpdateEntityNBT& packetIn) = 0;
    virtual void handleSignEditorOpen(S36PacketSignEditorOpen& packetIn) = 0;
    virtual void handleSpawnPosition(S05PacketSpawnPosition& packetIn) = 0;
};

} // namespace mc

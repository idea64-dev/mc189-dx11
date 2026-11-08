#include "client/network/NetHandlerPlayClient.h"

#include "block/Block.h"
#include "client/Minecraft.h"
#include "client/gui/GuiChat.h"
#include "client/gui/GuiDisconnected.h"
#include "client/gui/GuiDownloadTerrain.h"
#include "client/gui/GuiIngame.h"
#include "client/gui/GuiMainMenu.h"
#include "client/gui/GuiNewChat.h"
#include "client/multiplayer/PlayerControllerMP.h"
#include "client/multiplayer/WorldClient.h"
#include "client/network/NetworkPlayerInfo.h"
#include "client/resources/Locale.h"
#include "client/settings/GameSettings.h"
#include "core/Log.h"
#include "entity/Entity.h"
#include "entity/EntityLivingBase.h"
#include "entity/ai/attributes/Attributes.h"
#include "entity/player/EntityOtherPlayerMP.h"
#include "entity/player/EntityPlayer.h"
#include "entity/player/EntityPlayerSP.h"
#include "entity/player/InventoryPlayer.h"
#include "entity/player/PlayerCapabilities.h"
#include "inventory/Container.h"
#include "inventory/Slot.h"
#include "item/Item.h"
#include "item/ItemStack.h"
#include "network/NetworkManager.h"
#include "network/play/ClientPlayPackets.h"
#include "network/play/ServerPlayPackets.h"
#include "potion/Potion.h"
#include "tileentity/TileEntity.h"
#include "util/ChatComponent.h"
#include "util/EnumParticleTypes.h"
#include "util/FoodStats.h"
#include "util/MathHelper.h"
#include "world/WorldProvider.h"
#include "world/WorldSettings.h"
#include "world/chunk/Chunk.h"
#include "world/storage/WorldInfo.h"

#include <cfloat>
#include <cmath>
#include <deque>
#include <filesystem>

namespace mc {

namespace {

// Java: (float)(angle * 360) / 256.0F for a packed byte angle.
float unpackAngle(int angle)
{
    return static_cast<float>(angle * 360) / 256.0f;
}

// Java: PacketBuffer.writeString -- a VarInt length and the UTF-8 bytes.
std::vector<uint8_t> writeString(const std::string& s)
{
    std::vector<uint8_t> out;
    uint32_t len = static_cast<uint32_t>(s.size());
    while ((len & ~0x7Fu) != 0) {
        out.push_back(static_cast<uint8_t>((len & 0x7F) | 0x80));
        len >>= 7;
    }
    out.push_back(static_cast<uint8_t>(len));
    out.insert(out.end(), s.begin(), s.end());
    return out;
}

// Java: PacketBuffer.readStringFromBuffer
std::string readString(const std::vector<uint8_t>& data)
{
    uint32_t len = 0;
    size_t pos = 0;
    int shift = 0;
    while (pos < data.size()) {
        const uint8_t b = data[pos++];
        len |= static_cast<uint32_t>(b & 0x7F) << shift;
        if ((b & 0x80) == 0)
            break;
        shift += 7;
    }
    const size_t n = std::min<size_t>(len, data.size() - pos);
    return std::string(data.begin() + static_cast<std::ptrdiff_t>(pos), data.begin() + static_cast<std::ptrdiff_t>(pos + n));
}

} // namespace

NetHandlerPlayClient::NetHandlerPlayClient(Minecraft* mcIn, std::shared_ptr<GuiScreen> guiScreen, NetworkManager* networkManager, GameProfile profileIn)
    : netManager(networkManager)
    , profile(std::move(profileIn))
    , guiScreenServer(std::move(guiScreen))
    , gameController(mcIn)
{
}

NetHandlerPlayClient::~NetHandlerPlayClient() = default;

void NetHandlerPlayClient::handleJoinGame(S01PacketJoinGame& packetIn)
{
    gameController->playerController = std::make_unique<PlayerControllerMP>(gameController, this);
    auto world = std::make_unique<WorldClient>(this, WorldSettings(0, packetIn.getGameType(), false, packetIn.isHardcoreMode(), packetIn.getWorldType()),
        packetIn.getDimension(), packetIn.getDifficulty(), gameController->mcProfiler);
    clientWorldController = world.get();
    gameController->gameSettings->difficulty = packetIn.getDifficulty();
    gameController->loadWorld(std::move(world));
    gameController->thePlayer->dimension = packetIn.getDimension();
    gameController->displayGuiScreen(std::make_shared<GuiDownloadTerrain>(this));
    gameController->thePlayer->setEntityId(packetIn.getEntityId());
    currentServerMaxPlayers = packetIn.getMaxPlayers();
    gameController->thePlayer->setReducedDebug(packetIn.isReducedDebugInfo());
    gameController->playerController->setGameType(packetIn.getGameType());
    gameController->gameSettings->sendSettingsToServer();
    // Java: ClientBrandRetriever.getClientModName()
    netManager->sendPacket(std::make_shared<C17PacketCustomPayload>("MC|Brand", writeString("vanilla")));
}

void NetHandlerPlayClient::handleSpawnObject(S0EPacketSpawnObject& packetIn)
{
    // Java: boats, items, minecarts, projectiles, falling blocks, TNT, item frames... by type -- with the entities
    // (stage 10).
    Log::warn("Skipping object spawn of type {} (entity {}): not ported yet", packetIn.getType(), packetIn.getEntityID());
}

void NetHandlerPlayClient::handleSpawnExperienceOrb(S11PacketSpawnExperienceOrb& packetIn)
{
    // Java: new EntityXPOrb -- stage 10.
    Log::warn("Skipping experience orb spawn (entity {}): not ported yet", packetIn.getEntityID());
}

void NetHandlerPlayClient::handleSpawnGlobalEntity(S2CPacketSpawnGlobalEntity& packetIn)
{
    // Java: new EntityLightningBolt for type 1 -- with the weather effects (stage 10).
    Log::warn("Skipping global entity spawn of type {}: not ported yet", packetIn.func_149053_g());
}

void NetHandlerPlayClient::handleSpawnPainting(S10PacketSpawnPainting& packetIn)
{
    // Java: new EntityPainting -- stage 10.
    Log::warn("Skipping painting spawn (entity {}): not ported yet", packetIn.getEntityID());
}

void NetHandlerPlayClient::handleEntityVelocity(S12PacketEntityVelocity& packetIn)
{
    Entity* entity = clientWorldController->getEntityByID(packetIn.getEntityID());

    if (entity != nullptr)
        entity->setVelocity(static_cast<double>(packetIn.getMotionX()) / 8000.0, static_cast<double>(packetIn.getMotionY()) / 8000.0,
            static_cast<double>(packetIn.getMotionZ()) / 8000.0);
}

void NetHandlerPlayClient::handleEntityMetadata(S1CPacketEntityMetadata& packetIn)
{
    Entity* entity = clientWorldController->getEntityByID(packetIn.getEntityId());

    // Java: a null list when nothing changed -- empty here.
    if (entity != nullptr && !packetIn.func_149376_c().empty())
        entity->getDataWatcher().updateWatchedObjectsFromList(packetIn.func_149376_c());
}

void NetHandlerPlayClient::handleSpawnPlayer(S0CPacketSpawnPlayer& packetIn)
{
    const double d0 = static_cast<double>(packetIn.getX()) / 32.0;
    const double d1 = static_cast<double>(packetIn.getY()) / 32.0;
    const double d2 = static_cast<double>(packetIn.getZ()) / 32.0;
    const float f = unpackAngle(packetIn.getYaw());
    const float f1 = unpackAngle(packetIn.getPitch());
    NetworkPlayerInfo* info = getPlayerInfo(packetIn.getPlayer());
    if (info == nullptr) {
        // Java: a NullPointerException on the network thread.
        Log::warn("Spawning unlisted player {}", packetIn.getPlayer().toString());
        return;
    }
    auto entityotherplayermp = Entity::create<EntityOtherPlayerMP>(gameController->theWorld, info->getGameProfile());
    entityotherplayermp->prevPosX = entityotherplayermp->lastTickPosX = static_cast<double>(entityotherplayermp->serverPosX = packetIn.getX());
    entityotherplayermp->prevPosY = entityotherplayermp->lastTickPosY = static_cast<double>(entityotherplayermp->serverPosY = packetIn.getY());
    entityotherplayermp->prevPosZ = entityotherplayermp->lastTickPosZ = static_cast<double>(entityotherplayermp->serverPosZ = packetIn.getZ());
    const int i = packetIn.getCurrentItemID();
    auto& held = entityotherplayermp->inventory.mainInventory[static_cast<size_t>(entityotherplayermp->inventory.currentItem)];

    if (i == 0)
        held = nullptr;
    else
        held = std::make_shared<ItemStack>(Item::getItemById(i), 1, 0);

    entityotherplayermp->setPositionAndRotation(d0, d1, d2, f, f1);
    clientWorldController->addEntityToWorld(packetIn.getEntityID(), entityotherplayermp);
    // Java reads the server's watcher when the list was not decoded -- always the case locally.
    entityotherplayermp->getDataWatcher().updateWatchedObjectsFromList(packetIn.func_148944_c());
}

void NetHandlerPlayClient::handleEntityTeleport(S18PacketEntityTeleport& packetIn)
{
    Entity* entity = clientWorldController->getEntityByID(packetIn.getEntityId());

    if (entity != nullptr) {
        entity->serverPosX = packetIn.getX();
        entity->serverPosY = packetIn.getY();
        entity->serverPosZ = packetIn.getZ();
        const double d0 = static_cast<double>(entity->serverPosX) / 32.0;
        const double d1 = static_cast<double>(entity->serverPosY) / 32.0;
        const double d2 = static_cast<double>(entity->serverPosZ) / 32.0;
        const float f = unpackAngle(packetIn.getYaw());
        const float f1 = unpackAngle(packetIn.getPitch());

        if (std::abs(entity->posX - d0) < 0.03125 && std::abs(entity->posY - d1) < 0.015625 && std::abs(entity->posZ - d2) < 0.03125)
            entity->setPositionAndRotation2(entity->posX, entity->posY, entity->posZ, f, f1, 3, true);
        else
            entity->setPositionAndRotation2(d0, d1, d2, f, f1, 3, true);

        entity->onGround = packetIn.getOnGround();
    }
}

void NetHandlerPlayClient::handleHeldItemChange(S09PacketHeldItemChange& packetIn)
{
    if (packetIn.getHeldItemHotbarIndex() >= 0 && packetIn.getHeldItemHotbarIndex() < InventoryPlayer::getHotbarSize())
        gameController->thePlayer->inventory.currentItem = packetIn.getHeldItemHotbarIndex();
}

void NetHandlerPlayClient::handleEntityMovement(S14PacketEntity& packetIn)
{
    Entity* entity = packetIn.getEntity(clientWorldController);

    if (entity != nullptr) {
        entity->serverPosX += packetIn.func_149062_c();
        entity->serverPosY += packetIn.func_149061_d();
        entity->serverPosZ += packetIn.func_149064_e();
        const double d0 = static_cast<double>(entity->serverPosX) / 32.0;
        const double d1 = static_cast<double>(entity->serverPosY) / 32.0;
        const double d2 = static_cast<double>(entity->serverPosZ) / 32.0;
        const float f = packetIn.func_149060_h() ? unpackAngle(packetIn.func_149066_f()) : entity->rotationYaw;
        const float f1 = packetIn.func_149060_h() ? unpackAngle(packetIn.func_149063_g()) : entity->rotationPitch;
        entity->setPositionAndRotation2(d0, d1, d2, f, f1, 3, false);
        entity->onGround = packetIn.getOnGround();
    }
}

void NetHandlerPlayClient::handleEntityHeadLook(S19PacketEntityHeadLook& packetIn)
{
    Entity* entity = packetIn.getEntity(clientWorldController);

    if (entity != nullptr)
        entity->setRotationYawHead(unpackAngle(packetIn.getYaw()));
}

void NetHandlerPlayClient::handleDestroyEntities(S13PacketDestroyEntities& packetIn)
{
    for (const int id : packetIn.getEntityIDs())
        clientWorldController->removeEntityFromWorld(id);
}

void NetHandlerPlayClient::handlePlayerPosLook(S08PacketPlayerPosLook& packetIn)
{
    EntityPlayer* entityplayer = gameController->thePlayer.get();
    double d0 = packetIn.getX();
    double d1 = packetIn.getY();
    double d2 = packetIn.getZ();
    float f = packetIn.getYaw();
    float f1 = packetIn.getPitch();
    const auto& flags = packetIn.func_179834_f();
    using Flags = S08PacketPlayerPosLook::EnumFlags;

    if (flags.contains(Flags::X))
        d0 += entityplayer->posX;
    else
        entityplayer->motionX = 0.0;

    if (flags.contains(Flags::Y))
        d1 += entityplayer->posY;
    else
        entityplayer->motionY = 0.0;

    if (flags.contains(Flags::Z))
        d2 += entityplayer->posZ;
    else
        entityplayer->motionZ = 0.0;

    if (flags.contains(Flags::X_ROT))
        f1 += entityplayer->rotationPitch;

    if (flags.contains(Flags::Y_ROT))
        f += entityplayer->rotationYaw;

    entityplayer->setPositionAndRotation(d0, d1, d2, f, f1);
    netManager->sendPacket(std::make_shared<C03PacketPlayer::C06PacketPlayerPosLook>(
        entityplayer->posX, entityplayer->getEntityBoundingBox().minY, entityplayer->posZ, entityplayer->rotationYaw, entityplayer->rotationPitch, false));

    if (!doneLoadingTerrain) {
        gameController->thePlayer->prevPosX = gameController->thePlayer->posX;
        gameController->thePlayer->prevPosY = gameController->thePlayer->posY;
        gameController->thePlayer->prevPosZ = gameController->thePlayer->posZ;
        doneLoadingTerrain = true;
        gameController->displayGuiScreen(nullptr);
    }
}

void NetHandlerPlayClient::handleMultiBlockChange(S22PacketMultiBlockChange& packetIn)
{
    for (const auto& data : packetIn.getChangedBlocks())
        clientWorldController->invalidateRegionAndSetBlock(data.getPos(), data.getBlockState());
}

void NetHandlerPlayClient::handleChunkData(S21PacketChunkData& packetIn)
{
    if (packetIn.func_149274_i()) {
        if (packetIn.getExtractedSize() == 0) {
            clientWorldController->doPreChunk(packetIn.getChunkX(), packetIn.getChunkZ(), false);
            return;
        }

        clientWorldController->doPreChunk(packetIn.getChunkX(), packetIn.getChunkZ(), true);
    }

    clientWorldController->invalidateBlockReceiveRegion(packetIn.getChunkX() << 4, 0, packetIn.getChunkZ() << 4, (packetIn.getChunkX() << 4) + 15, 256,
        (packetIn.getChunkZ() << 4) + 15);
    Chunk* chunk = clientWorldController->getChunkFromChunkCoords(packetIn.getChunkX(), packetIn.getChunkZ());
    chunk->fillChunk(packetIn.getExtractedDataBytes(), packetIn.getExtractedSize(), packetIn.func_149274_i());
    clientWorldController->markBlockRangeForRenderUpdate(packetIn.getChunkX() << 4, 0, packetIn.getChunkZ() << 4, (packetIn.getChunkX() << 4) + 15, 256,
        (packetIn.getChunkZ() << 4) + 15);

    if (!packetIn.func_149274_i() || dynamic_cast<WorldProviderSurface*>(clientWorldController->provider.get()) == nullptr)
        chunk->resetRelightChecks();
}

void NetHandlerPlayClient::handleBlockChange(S23PacketBlockChange& packetIn)
{
    clientWorldController->invalidateRegionAndSetBlock(packetIn.getBlockPosition(), packetIn.getBlockState());
}

void NetHandlerPlayClient::handleDisconnect(S40PacketDisconnect& packetIn)
{
    netManager->closeChannel(packetIn.getReason());
}

void NetHandlerPlayClient::onDisconnect(const std::shared_ptr<IChatComponent>& reason)
{
    gameController->loadWorld(nullptr);

    // Java: GuiMultiplayer(GuiMainMenu) without a parent screen -- the multiplayer screen comes with stage 14.
    std::shared_ptr<GuiScreen> parent = guiScreenServer ? guiScreenServer : std::make_shared<GuiMainMenu>();
    gameController->displayGuiScreen(std::make_shared<GuiDisconnected>(parent, "disconnect.lost", reason));
}

void NetHandlerPlayClient::addToSendQueue(std::shared_ptr<Packet> packet)
{
    netManager->sendPacket(std::move(packet));
}

void NetHandlerPlayClient::handleCollectItem(S0DPacketCollectItem& packetIn)
{
    Entity* entity = clientWorldController->getEntityByID(packetIn.getCollectedItemEntityID());

    if (entity != nullptr) {
        const float a = avRandomizer.nextFloat();
        const float b = avRandomizer.nextFloat();
        const float pitch = ((a - b) * 0.7f + 1.0f) * 2.0f;

        if (entity->getClassName() == "EntityXPOrb")
            clientWorldController->playSoundAtEntity(entity, "random.orb", 0.2f, pitch);
        else
            clientWorldController->playSoundAtEntity(entity, "random.pop", 0.2f, pitch);

        // Java: effectRenderer.addEffect(new EntityPickupFX(...)) -- the particles of stage 15.
        clientWorldController->removeEntityFromWorld(packetIn.getCollectedItemEntityID());
    }
}

void NetHandlerPlayClient::handleChat(S02PacketChat& packetIn)
{
    if (packetIn.getType() == 2)
        gameController->ingameGUI->setRecordPlaying(packetIn.getChatComponent(), false);
    else
        gameController->ingameGUI->getChatGUI().printChatMessage(packetIn.getChatComponent());
}

void NetHandlerPlayClient::handleAnimation(S0BPacketAnimation& packetIn)
{
    Entity* entity = clientWorldController->getEntityByID(packetIn.getEntityID());

    if (entity == nullptr)
        return;

    if (packetIn.getAnimationType() == 0) {
        if (auto* entitylivingbase = dynamic_cast<EntityLivingBase*>(entity))
            entitylivingbase->swingItem();
    } else if (packetIn.getAnimationType() == 1) {
        entity->performHurtAnimation();
    } else if (packetIn.getAnimationType() == 2) {
        if (auto* entityplayer = dynamic_cast<EntityPlayer*>(entity))
            entityplayer->wakeUpPlayer(false, false, false);
    }
    // 4, 5: the critical hit particles (stage 15).
}

void NetHandlerPlayClient::handleUseBed(S0APacketUseBed& packetIn)
{
    if (EntityPlayer* player = packetIn.getPlayer(clientWorldController))
        player->trySleep(packetIn.getBedPosition());
}

void NetHandlerPlayClient::handleSpawnMob(S0FPacketSpawnMob& packetIn)
{
    // Java: EntityList.createEntityByID -- the mobs come with stage 10.
    Log::warn("Skipping mob spawn of type {} (entity {}): not ported yet", packetIn.getEntityType(), packetIn.getEntityID());
}

void NetHandlerPlayClient::handleTimeUpdate(S03PacketTimeUpdate& packetIn)
{
    gameController->theWorld->setTotalWorldTime(packetIn.getTotalWorldTime());
    gameController->theWorld->setWorldTime(packetIn.getWorldTime());
}

void NetHandlerPlayClient::handleSpawnPosition(S05PacketSpawnPosition& packetIn)
{
    gameController->thePlayer->setSpawnPoint(packetIn.getSpawnPos(), true);
    gameController->theWorld->getWorldInfo().setSpawn(packetIn.getSpawnPos());
}

void NetHandlerPlayClient::handleEntityAttach(S1BPacketEntityAttach& packetIn)
{
    Entity* entity = clientWorldController->getEntityByID(packetIn.getEntityId());
    Entity* entity1 = clientWorldController->getEntityByID(packetIn.getVehicleEntityId());

    if (packetIn.getLeash() == 0) {
        bool flag = false;

        if (packetIn.getEntityId() == gameController->thePlayer->getEntityId()) {
            entity = gameController->thePlayer.get();
            // Java: EntityBoat.setIsBoatEmpty(false) -- with the boats (stage 10).
            flag = entity->ridingEntity == nullptr && entity1 != nullptr;
        }
        // Java: else EntityBoat.setIsBoatEmpty(true).

        if (entity == nullptr)
            return;

        entity->mountEntity(entity1);

        if (flag) {
            GameSettings& gamesettings = *gameController->gameSettings;
            gameController->ingameGUI->setRecordPlaying(I18n::format("mount.onboard", GameSettings::getKeyDisplayString(gamesettings.keyBindSneak.getKeyCode())), false);
        }
    }
    // Java: leash 1 -- EntityLiving.setLeashedToEntity / clearLeashed, with the mobs (stage 10).
}

void NetHandlerPlayClient::handleEntityStatus(S19PacketEntityStatus& packetIn)
{
    Entity* entity = packetIn.getEntity(clientWorldController);

    if (entity != nullptr) {
        // Java: 21 plays GuardianSound -- with the guardians (stage 10).
        if (packetIn.getOpCode() != 21)
            entity->handleStatusUpdate(packetIn.getOpCode());
    }
}

void NetHandlerPlayClient::handleUpdateHealth(S06PacketUpdateHealth& packetIn)
{
    gameController->thePlayer->setPlayerSPHealth(packetIn.getHealth());
    gameController->thePlayer->getFoodStats().setFoodLevel(packetIn.getFoodLevel());
    gameController->thePlayer->getFoodStats().setFoodSaturationLevel(packetIn.getSaturationLevel());
}

void NetHandlerPlayClient::handleSetExperience(S1FPacketSetExperience& packetIn)
{
    gameController->thePlayer->setXPStats(packetIn.func_149397_c(), packetIn.getTotalExperience(), packetIn.getLevel());
}

void NetHandlerPlayClient::handleRespawn(S07PacketRespawn& packetIn)
{
    if (packetIn.getDimensionID() != gameController->thePlayer->dimension) {
        doneLoadingTerrain = false;
        // Java: carries the scoreboard over (stage 12).
        auto world = std::make_unique<WorldClient>(this,
            WorldSettings(0, packetIn.getGameType(), false, gameController->theWorld->getWorldInfo().isHardcoreModeEnabled(), packetIn.getWorldType()),
            packetIn.getDimensionID(), packetIn.getDifficulty(), gameController->mcProfiler);
        clientWorldController = world.get();
        gameController->loadWorld(std::move(world));
        gameController->thePlayer->dimension = packetIn.getDimensionID();
        gameController->displayGuiScreen(std::make_shared<GuiDownloadTerrain>(this));
    }

    gameController->setDimensionAndSpawnPlayer(packetIn.getDimensionID());
    gameController->playerController->setGameType(packetIn.getGameType());
}

void NetHandlerPlayClient::handleExplosion(S27PacketExplosion& packetIn)
{
    // Java: new Explosion(...).doExplosionB(true) -- the sound, particles and block removal; Explosion comes with
    // TNT (stage 10). The knockback applies.
    gameController->thePlayer->motionX += static_cast<double>(packetIn.func_149149_c());
    gameController->thePlayer->motionY += static_cast<double>(packetIn.func_149144_d());
    gameController->thePlayer->motionZ += static_cast<double>(packetIn.func_149147_e());
}

void NetHandlerPlayClient::handleOpenWindow(S2DPacketOpenWindow& packetIn)
{
    // Java: chests, villagers, horses, furnaces... -- the container screens come with stage 11.
    Log::warn("Skipping window {} ({}): not ported yet", packetIn.getWindowId(), packetIn.getGuiId());
}

void NetHandlerPlayClient::handleSetSlot(S2FPacketSetSlot& packetIn)
{
    EntityPlayer* entityplayer = gameController->thePlayer.get();

    if (packetIn.func_149175_c() == -1) {
        entityplayer->inventory.setItemStack(packetIn.func_149174_e());
        return;
    }

    // Java: true in the creative inventory unless its inventory tab is selected (GuiContainerCreative, stage 11).
    const bool flag = false;

    if (packetIn.func_149175_c() == 0 && packetIn.func_149173_d() >= 36 && packetIn.func_149173_d() < 45) {
        const std::shared_ptr<ItemStack> itemstack = entityplayer->inventoryContainer->getSlot(packetIn.func_149173_d())->getStack();

        if (packetIn.func_149174_e() != nullptr && (itemstack == nullptr || itemstack->stackSize < packetIn.func_149174_e()->stackSize))
            packetIn.func_149174_e()->animationsToGo = 5;

        entityplayer->inventoryContainer->putStackInSlot(packetIn.func_149173_d(), packetIn.func_149174_e());
    } else if (packetIn.func_149175_c() == entityplayer->openContainer->windowId && (packetIn.func_149175_c() != 0 || !flag)) {
        entityplayer->openContainer->putStackInSlot(packetIn.func_149173_d(), packetIn.func_149174_e());
    }
}

void NetHandlerPlayClient::handleConfirmTransaction(S32PacketConfirmTransaction& packetIn)
{
    bool hasContainer = false;
    EntityPlayer* entityplayer = gameController->thePlayer.get();

    if (packetIn.getWindowId() == 0)
        hasContainer = entityplayer->inventoryContainer != nullptr;
    else if (packetIn.getWindowId() == entityplayer->openContainer->windowId)
        hasContainer = entityplayer->openContainer != nullptr;

    if (hasContainer && !packetIn.func_148888_e())
        addToSendQueue(std::make_shared<C0FPacketConfirmTransaction>(packetIn.getWindowId(), packetIn.getActionNumber(), true));
}

void NetHandlerPlayClient::handleWindowItems(S30PacketWindowItems& packetIn)
{
    EntityPlayer* entityplayer = gameController->thePlayer.get();

    if (packetIn.func_148911_c() == 0)
        entityplayer->inventoryContainer->putStacksInSlots(packetIn.getItemStacks());
    else if (packetIn.func_148911_c() == entityplayer->openContainer->windowId)
        entityplayer->openContainer->putStacksInSlots(packetIn.getItemStacks());
}

void NetHandlerPlayClient::handleSignEditorOpen(S36PacketSignEditorOpen& packetIn)
{
    // Java: thePlayer.openEditSign(TileEntitySign) -- the sign editor comes with the signs.
    Log::warn("Skipping sign editor at {}: not ported yet", packetIn.getSignPosition().toString());
}

void NetHandlerPlayClient::handleUpdateSign(S33PacketUpdateSign& packetIn)
{
    bool flag = false;

    if (gameController->theWorld->isBlockLoaded(packetIn.getPos())) {
        TileEntity* tileentity = gameController->theWorld->getTileEntity(packetIn.getPos());

        // Java: copies the lines into an editable TileEntitySign (not ported yet -- the text stays as loaded).
        if (tileentity != nullptr && tileentity->getMappingName() == "Sign")
            flag = true;
    }

    if (!flag && gameController->thePlayer != nullptr) {
        gameController->thePlayer->addChatMessage(std::make_shared<ChatComponentText>("Unable to locate sign at " + std::to_string(packetIn.getPos().getX()) + ", "
            + std::to_string(packetIn.getPos().getY()) + ", " + std::to_string(packetIn.getPos().getZ())));
    }
}

void NetHandlerPlayClient::handleUpdateTileEntity(S35PacketUpdateTileEntity& packetIn)
{
    if (!gameController->theWorld->isBlockLoaded(packetIn.getPos()))
        return;

    TileEntity* tileentity = gameController->theWorld->getTileEntity(packetIn.getPos());
    if (tileentity == nullptr)
        return;

    const int i = packetIn.getTileEntityType();
    const std::string_view name = tileentity->getMappingName();

    // Java: instanceof checks; each of these classes has no subclasses, so its id identifies it.
    if ((i == 1 && name == "MobSpawner") || (i == 2 && name == "Control") || (i == 3 && name == "Beacon") || (i == 4 && name == "Skull")
        || (i == 5 && name == "FlowerPot") || (i == 6 && name == "Banner"))
        tileentity->readFromNBT(*packetIn.getNbtCompound());
}

void NetHandlerPlayClient::handleWindowProperty(S31PacketWindowProperty& packetIn)
{
    EntityPlayer* entityplayer = gameController->thePlayer.get();

    if (entityplayer->openContainer != nullptr && entityplayer->openContainer->windowId == packetIn.getWindowId())
        entityplayer->openContainer->updateProgressBar(packetIn.getVarIndex(), packetIn.getVarValue());
}

void NetHandlerPlayClient::handleEntityEquipment(S04PacketEntityEquipment& packetIn)
{
    Entity* entity = clientWorldController->getEntityByID(packetIn.getEntityID());

    if (entity != nullptr)
        entity->setCurrentItemOrArmor(packetIn.getEquipmentSlot(), packetIn.getItemStack());
}

void NetHandlerPlayClient::handleCloseWindow(S2EPacketCloseWindow&)
{
    gameController->thePlayer->closeScreenAndDropStack();
}

void NetHandlerPlayClient::handleBlockAction(S24PacketBlockAction& packetIn)
{
    gameController->theWorld->addBlockEvent(packetIn.getBlockPosition(), packetIn.getBlockType(), packetIn.getData1(), packetIn.getData2());
}

void NetHandlerPlayClient::handleBlockBreakAnim(S25PacketBlockBreakAnim& packetIn)
{
    gameController->theWorld->sendBlockBreakProgress(packetIn.getBreakerId(), packetIn.getPosition(), packetIn.getProgress());
}

void NetHandlerPlayClient::handleMapChunkBulk(S26PacketMapChunkBulk& packetIn)
{
    for (int i = 0; i < packetIn.getChunkCount(); ++i) {
        const int j = packetIn.getChunkX(i);
        const int k = packetIn.getChunkZ(i);
        clientWorldController->doPreChunk(j, k, true);
        clientWorldController->invalidateBlockReceiveRegion(j << 4, 0, k << 4, (j << 4) + 15, 256, (k << 4) + 15);
        Chunk* chunk = clientWorldController->getChunkFromChunkCoords(j, k);
        chunk->fillChunk(packetIn.getChunkBytes(i), packetIn.getChunkSize(i), true);
        clientWorldController->markBlockRangeForRenderUpdate(j << 4, 0, k << 4, (j << 4) + 15, 256, (k << 4) + 15);

        if (dynamic_cast<WorldProviderSurface*>(clientWorldController->provider.get()) == nullptr)
            chunk->resetRelightChecks();
    }
}

void NetHandlerPlayClient::handleChangeGameState(S2BPacketChangeGameState& packetIn)
{
    EntityPlayer* entityplayer = gameController->thePlayer.get();
    const int i = packetIn.getGameState();
    const float f = packetIn.func_149137_d();
    const int j = MathHelper::floor_float(f + 0.5f);

    if (i >= 0 && i < static_cast<int>(std::size(S2BPacketChangeGameState::MESSAGE_NAMES)) && S2BPacketChangeGameState::MESSAGE_NAMES[i] != nullptr)
        entityplayer->addChatComponentMessage(std::make_shared<ChatComponentTranslation>(S2BPacketChangeGameState::MESSAGE_NAMES[i]));

    if (i == 1) {
        clientWorldController->getWorldInfo().setRaining(true);
        clientWorldController->setRainStrength(0.0f);
    } else if (i == 2) {
        clientWorldController->getWorldInfo().setRaining(false);
        clientWorldController->setRainStrength(1.0f);
    } else if (i == 3) {
        gameController->playerController->setGameType(getGameTypeByID(j));
    } else if (i == 4) {
        // Java: GuiWinGame -- the end poem comes with the End (stage 12).
        Log::warn("Skipping the end credits: not ported yet");
    } else if (i == 5) {
        // Java: the demo screens (demo mode is out of scope).
    } else if (i == 6) {
        clientWorldController->playSound(entityplayer->posX, entityplayer->posY + static_cast<double>(entityplayer->getEyeHeight()), entityplayer->posZ,
            "random.successful_hit", 0.18f, 0.45f, false);
    } else if (i == 7) {
        clientWorldController->setRainStrength(f);
    } else if (i == 8) {
        clientWorldController->setThunderStrength(f);
    } else if (i == 10) {
        clientWorldController->spawnParticle(getParticleID(EnumParticleTypes::MOB_APPEARANCE), false, entityplayer->posX, entityplayer->posY, entityplayer->posZ, 0.0,
            0.0, 0.0);
        clientWorldController->playSound(entityplayer->posX, entityplayer->posY, entityplayer->posZ, "mob.guardian.curse", 1.0f, 1.0f, false);
    }
}

void NetHandlerPlayClient::handleEffect(S28PacketEffect& packetIn)
{
    if (packetIn.isSoundServerwide())
        gameController->theWorld->playBroadcastSound(packetIn.getSoundType(), packetIn.getSoundPos(), packetIn.getSoundData());
    else
        gameController->theWorld->playAuxSFX(packetIn.getSoundType(), packetIn.getSoundPos(), packetIn.getSoundData());
}

void NetHandlerPlayClient::handleEntityEffect(S1DPacketEntityEffect& packetIn)
{
    Entity* entity = clientWorldController->getEntityByID(packetIn.getEntityId());

    if (auto* living = dynamic_cast<EntityLivingBase*>(entity)) {
        PotionEffect potioneffect(packetIn.getEffectId(), packetIn.getDuration(), packetIn.getAmplifier(), false, packetIn.func_179707_f());
        potioneffect.setPotionDurationMax(packetIn.func_149429_c());
        living->addPotionEffect(potioneffect);
    }
}

void NetHandlerPlayClient::handleCombatEvent(S42PacketCombatEvent&)
{
    // Java: only feeds the Twitch stream metadata (out of scope).
}

void NetHandlerPlayClient::handleServerDifficulty(S41PacketServerDifficulty& packetIn)
{
    gameController->theWorld->getWorldInfo().setDifficulty(packetIn.getDifficulty());
    gameController->theWorld->getWorldInfo().setDifficultyLocked(packetIn.isDifficultyLocked());
}

void NetHandlerPlayClient::handleCamera(S43PacketCamera& packetIn)
{
    Entity* entity = packetIn.getEntity(clientWorldController);

    if (entity != nullptr)
        gameController->setRenderViewEntity(entity);
}

void NetHandlerPlayClient::handleWorldBorder(S44PacketWorldBorder& packetIn)
{
    packetIn.func_179788_a(clientWorldController->getWorldBorder());
}

void NetHandlerPlayClient::handleTitle(S45PacketTitle& packetIn)
{
    const S45PacketTitle::Type s45packettitle$type = packetIn.getType();
    std::optional<std::string> s;
    std::optional<std::string> s1;
    const std::string s2 = packetIn.getMessage() != nullptr ? packetIn.getMessage()->getFormattedText() : "";

    switch (s45packettitle$type) {
    case S45PacketTitle::Type::TITLE:
        s = s2;
        break;
    case S45PacketTitle::Type::SUBTITLE:
        s1 = s2;
        break;
    case S45PacketTitle::Type::RESET:
        gameController->ingameGUI->displayTitle(std::string(), std::string(), -1, -1, -1);
        gameController->ingameGUI->setDefaultTitlesTimes();
        return;
    default:
        break;
    }

    gameController->ingameGUI->displayTitle(s, s1, packetIn.getFadeInTime(), packetIn.getDisplayTime(), packetIn.getFadeOutTime());
}

void NetHandlerPlayClient::handleSetCompressionLevel(S46PacketSetCompressionLevel&)
{
    // Java: only for a remote connection; the local one never compresses.
}

void NetHandlerPlayClient::handlePlayerListHeaderFooter(S47PacketPlayerListHeaderFooter& packetIn)
{
    const auto& header = packetIn.getHeader();
    const auto& footer = packetIn.getFooter();
    gameController->ingameGUI->getTabList().setHeader(header == nullptr || header->getFormattedText().empty() ? nullptr : header);
    gameController->ingameGUI->getTabList().setFooter(footer == nullptr || footer->getFormattedText().empty() ? nullptr : footer);
}

void NetHandlerPlayClient::handleRemoveEntityEffect(S1EPacketRemoveEntityEffect& packetIn)
{
    Entity* entity = clientWorldController->getEntityByID(packetIn.getEntityId());

    if (auto* living = dynamic_cast<EntityLivingBase*>(entity))
        living->removePotionEffectClient(packetIn.getEffectId());
}

void NetHandlerPlayClient::handlePlayerListItem(S38PacketPlayerListItem& packetIn)
{
    using Action = S38PacketPlayerListItem::Action;

    for (const auto& data : packetIn.getEntries()) {
        const UUID id = data.getProfile().getId().value_or(UUID());

        if (packetIn.getAction() == Action::REMOVE_PLAYER) {
            playerInfoMap.erase(id);
            continue;
        }

        NetworkPlayerInfo* networkplayerinfo = nullptr;
        if (auto* found = playerInfoMap.find(id))
            networkplayerinfo = found->get();

        if (packetIn.getAction() == Action::ADD_PLAYER) {
            auto info = std::make_unique<NetworkPlayerInfo>(data);
            networkplayerinfo = info.get();
            playerInfoMap.put(networkplayerinfo->getGameProfile().getId().value_or(UUID()), std::move(info));
        }

        if (networkplayerinfo != nullptr) {
            switch (packetIn.getAction()) {
            case Action::ADD_PLAYER:
                networkplayerinfo->setGameType(data.getGameMode());
                networkplayerinfo->setResponseTime(data.getPing());
                break;
            case Action::UPDATE_GAME_MODE:
                networkplayerinfo->setGameType(data.getGameMode());
                break;
            case Action::UPDATE_LATENCY:
                networkplayerinfo->setResponseTime(data.getPing());
                break;
            case Action::UPDATE_DISPLAY_NAME:
                networkplayerinfo->setDisplayName(data.getDisplayName());
                break;
            default:
                break;
            }
        }
    }
}

void NetHandlerPlayClient::handleKeepAlive(S00PacketKeepAlive& packetIn)
{
    addToSendQueue(std::make_shared<C00PacketKeepAlive>(packetIn.func_149134_c()));
}

void NetHandlerPlayClient::handlePlayerAbilities(S39PacketPlayerAbilities& packetIn)
{
    EntityPlayer* entityplayer = gameController->thePlayer.get();
    entityplayer->capabilities.isFlying = packetIn.isFlying();
    entityplayer->capabilities.isCreativeMode = packetIn.isCreativeMode();
    entityplayer->capabilities.disableDamage = packetIn.isInvulnerable();
    entityplayer->capabilities.allowFlying = packetIn.isAllowFlying();
    entityplayer->capabilities.setFlySpeed(packetIn.getFlySpeed());
    entityplayer->capabilities.setPlayerWalkSpeed(packetIn.getWalkSpeed());
}

void NetHandlerPlayClient::handleTabComplete(S3APacketTabComplete& packetIn)
{
    // Keep the screen alive while it handles the matches.
    const std::shared_ptr<GuiScreen> screen = gameController->currentScreen;
    if (auto* guichat = dynamic_cast<GuiChat*>(screen.get()))
        guichat->onAutocompleteResponse(packetIn.func_149630_c());
}

void NetHandlerPlayClient::handleSoundEffect(S29PacketSoundEffect& packetIn)
{
    gameController->theWorld->playSound(packetIn.getX(), packetIn.getY(), packetIn.getZ(), packetIn.getSoundName(), packetIn.getVolume(), packetIn.getPitch(), false);
}

void NetHandlerPlayClient::handleResourcePack(S48PacketResourcePackSend& packetIn)
{
    const std::string& s = packetIn.getURL();
    const std::string& s1 = packetIn.getHash();

    // Java: a world's resources.zip (level://) is applied; a server's pack is downloaded or asked about. Server
    // resource packs are not ported yet: a missing level pack fails as in Java, any other one is declined.
    if (s.starts_with("level://")) {
        const std::filesystem::path file2 = std::filesystem::path("saves") / s.substr(std::string_view("level://").size());

        if (!std::filesystem::is_regular_file(file2)) {
            netManager->sendPacket(std::make_shared<C19PacketResourcePackStatus>(s1, C19PacketResourcePackStatus::Action::FAILED_DOWNLOAD));
            return;
        }
    }

    netManager->sendPacket(std::make_shared<C19PacketResourcePackStatus>(s1, C19PacketResourcePackStatus::Action::DECLINED));
}

void NetHandlerPlayClient::handleEntityNBT(S49PacketUpdateEntityNBT&)
{
    // Java: entity.clientUpdateEntityNBT -- only the armor stands use it (stage 10).
}

void NetHandlerPlayClient::handleCustomPayload(S3FPacketCustomPayload& packetIn)
{
    if (packetIn.getChannelName() == "MC|Brand")
        gameController->thePlayer->setClientBrand(readString(packetIn.getBufferData()));
    // Java: MC|TrList (trading, stage 12) and MC|BOpen (books).
}

void NetHandlerPlayClient::handleParticles(S2APacketParticles& packetIn)
{
    const int particleID = getParticleID(packetIn.getParticleType());

    if (packetIn.getParticleCount() == 0) {
        const double d0 = static_cast<double>(packetIn.getParticleSpeed() * packetIn.getXOffset());
        const double d2 = static_cast<double>(packetIn.getParticleSpeed() * packetIn.getYOffset());
        const double d4 = static_cast<double>(packetIn.getParticleSpeed() * packetIn.getZOffset());
        clientWorldController->spawnParticle(particleID, packetIn.isLongDistance(), packetIn.getXCoordinate(), packetIn.getYCoordinate(), packetIn.getZCoordinate(), d0,
            d2, d4, packetIn.getParticleArgs());
    } else {
        for (int i = 0; i < packetIn.getParticleCount(); ++i) {
            const double d1 = avRandomizer.nextGaussian() * static_cast<double>(packetIn.getXOffset());
            const double d3 = avRandomizer.nextGaussian() * static_cast<double>(packetIn.getYOffset());
            const double d5 = avRandomizer.nextGaussian() * static_cast<double>(packetIn.getZOffset());
            const double d6 = avRandomizer.nextGaussian() * static_cast<double>(packetIn.getParticleSpeed());
            const double d7 = avRandomizer.nextGaussian() * static_cast<double>(packetIn.getParticleSpeed());
            const double d8 = avRandomizer.nextGaussian() * static_cast<double>(packetIn.getParticleSpeed());
            clientWorldController->spawnParticle(particleID, packetIn.isLongDistance(), packetIn.getXCoordinate() + d1, packetIn.getYCoordinate() + d3,
                packetIn.getZCoordinate() + d5, d6, d7, d8, packetIn.getParticleArgs());
        }
    }
}

void NetHandlerPlayClient::handleEntityProperties(S20PacketEntityProperties& packetIn)
{
    Entity* entity = clientWorldController->getEntityByID(packetIn.getEntityId());

    if (entity == nullptr)
        return;

    auto* living = dynamic_cast<EntityLivingBase*>(entity);
    if (living == nullptr) {
        // Java: IllegalStateException on the network thread.
        Log::warn("Server tried to update attributes of a non-living entity (actually: {})", entity->toString());
        return;
    }

    BaseAttributeMap& baseattributemap = living->getAttributeMap();
    // The attributes the server names that the client lacks; the map keeps references, so they live on.
    static std::deque<std::unique_ptr<RangedAttribute>> clientOnlyAttributes;

    for (const auto& snapshot : packetIn.func_149441_d()) {
        IAttributeInstance* iattributeinstance = baseattributemap.getAttributeInstanceByName(snapshot.func_151409_a());

        if (iattributeinstance == nullptr) {
            clientOnlyAttributes.push_back(std::make_unique<RangedAttribute>(nullptr, snapshot.func_151409_a(), 0.0, 2.2250738585072014E-308, DBL_MAX));
            iattributeinstance = &baseattributemap.registerAttribute(*clientOnlyAttributes.back());
        }

        iattributeinstance->setBaseValue(snapshot.func_151410_b());
        iattributeinstance->removeAllModifiers();

        for (const AttributeModifier& attributemodifier : snapshot.func_151408_c())
            iattributeinstance->applyModifier(attributemodifier);
    }
}

std::vector<NetworkPlayerInfo*> NetHandlerPlayClient::getPlayerInfoMap() const
{
    std::vector<NetworkPlayerInfo*> list;
    list.reserve(playerInfoMap.size());
    for (const auto& entry : playerInfoMap)
        list.push_back(entry.second.get());
    return list;
}

NetworkPlayerInfo* NetHandlerPlayClient::getPlayerInfo(const UUID& id) const
{
    const auto* found = playerInfoMap.find(id);
    return found ? found->get() : nullptr;
}

NetworkPlayerInfo* NetHandlerPlayClient::getPlayerInfo(const std::string& name) const
{
    for (const auto& entry : playerInfoMap) {
        if (entry.second->getGameProfile().getName() == name)
            return entry.second.get();
    }
    return nullptr;
}

} // namespace mc

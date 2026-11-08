#include "network/NetHandlerPlayServer.h"

#include "block/Block.h"
#include "core/JavaTime.h"
#include "core/Log.h"
#include "core/StringUtil.h"
#include "entity/player/EntityPlayerMP.h"
#include "entity/player/InventoryPlayer.h"
#include "inventory/Container.h"
#include "item/ItemStack.h"
#include "nbt/NBTTagCompound.h"
#include "network/NetworkManager.h"
#include "network/play/ServerPlayPackets.h"
#include "server/MinecraftServer.h"
#include "server/management/ItemInWorldManager.h"
#include "server/management/ServerConfigurationManager.h"
#include "tileentity/TileEntity.h"
#include "util/Vec3.h"
#include "util/ChatAllowedCharacters.h"
#include "util/ChatComponent.h"
#include "world/WorldServer.h"
#include "world/border/WorldBorder.h"
#include "world/storage/WorldInfo.h"

#include <cmath>

namespace mc {

namespace {

// Commons Lang StringUtils.normalizeSpace: trim, then every whitespace run becomes one space.
std::u16string normalizeSpace(const std::u16string& text)
{
    size_t begin = 0;
    size_t end = text.size();
    while (begin < end && text[begin] <= u' ')
        ++begin;
    while (end > begin && text[end - 1] <= u' ')
        --end;
    const auto isWhitespace = [](char16_t c) { return c == u' ' || c == u'\t' || c == u'\n' || c == u'\x0B' || c == u'\f' || c == u'\r'; };
    std::u16string result;
    for (size_t i = begin; i < end;) {
        if (isWhitespace(text[i])) {
            size_t j = i + 1;
            while (j < end && isWhitespace(text[j]))
                ++j;
            result.push_back(u' ');
            i = j;
        } else {
            result.push_back(text[i++]);
        }
    }
    return result;
}

std::shared_ptr<IChatComponent> coloured(std::shared_ptr<IChatComponent> component, char colour)
{
    component->getChatStyle()->setColor(colour);
    return component;
}

} // namespace

std::shared_ptr<NetHandlerPlayServer> NetHandlerPlayServer::create(
    MinecraftServer* server, std::shared_ptr<NetworkManager> networkManagerIn, std::shared_ptr<EntityPlayerMP> playerIn)
{
    auto handler = std::make_shared<NetHandlerPlayServer>(server, networkManagerIn, playerIn);
    networkManagerIn->setNetHandler(handler);
    playerIn->playerNetServerHandler = handler;
    return handler;
}

NetHandlerPlayServer::NetHandlerPlayServer(MinecraftServer* server, std::shared_ptr<NetworkManager> networkManagerIn, std::shared_ptr<EntityPlayerMP> playerIn)
    : netManager(std::move(networkManagerIn))
    , playerEntity(std::move(playerIn))
    , serverController(server)
{
}

int64_t NetHandlerPlayServer::currentTimeMillis()
{
    return java::nanoTime() / 1000000;
}

void NetHandlerPlayServer::update()
{
    field_147366_g = false;
    ++networkTickCount;
    serverController->theProfiler.startSection("keepAlive");

    if (static_cast<int64_t>(networkTickCount) - lastSentPingPacket > 40) {
        lastSentPingPacket = networkTickCount;
        lastPingTime = currentTimeMillis();
        field_147378_h = static_cast<int>(lastPingTime);
        sendPacket(std::make_shared<S00PacketKeepAlive>(field_147378_h));
    }

    serverController->theProfiler.endSection();

    if (chatSpamThresholdCount > 0)
        --chatSpamThresholdCount;

    if (itemDropThreshold > 0)
        --itemDropThreshold;

    if (playerEntity->getLastActiveTime() > 0 && serverController->getMaxPlayerIdleMinutes() > 0
        && MinecraftServer::getCurrentTimeMillis() - playerEntity->getLastActiveTime() > static_cast<int64_t>(serverController->getMaxPlayerIdleMinutes()) * 1000 * 60)
        kickPlayerFromServer("You have been idle for too long!");
}

void NetHandlerPlayServer::kickPlayerFromServer(const std::string& reason)
{
    const auto chatcomponenttext = std::make_shared<ChatComponentText>(reason);
    // Java closes the channel once the packet is out; the local channel hands it over at once.
    netManager->sendPacket(std::make_shared<S40PacketDisconnect>(chatcomponenttext));
    netManager->closeChannel(chatcomponenttext);
    netManager->disableAutoRead();
    const std::shared_ptr<NetworkManager> manager = netManager;
    serverController->addScheduledTask([manager] { manager->checkDisconnected(); });
}

void NetHandlerPlayServer::processInput(C0CPacketInput& packetIn)
{
    playerEntity->setEntityActionState(packetIn.getStrafeSpeed(), packetIn.getForwardSpeed(), packetIn.isJumping(), packetIn.isSneaking());
}

bool NetHandlerPlayServer::func_183006_b(const C03PacketPlayer& packet)
{
    return !std::isfinite(packet.getPositionX()) || !std::isfinite(packet.getPositionY()) || !std::isfinite(packet.getPositionZ())
        || !std::isfinite(packet.getPitch()) || !std::isfinite(packet.getYaw());
}

void NetHandlerPlayServer::processPlayer(C03PacketPlayer& packetIn)
{
    if (func_183006_b(packetIn)) {
        kickPlayerFromServer("Invalid move packet received");
        return;
    }

    // The handler may swap its player (respawn); keep this one for the whole packet.
    const std::shared_ptr<EntityPlayerMP> player = playerEntity;
    WorldServer* worldserver = serverController->worldServerForDimension(player->dimension);
    field_147366_g = true;

    if (player->playerConqueredTheEnd)
        return;

    const double d0 = player->posX;
    const double d1 = player->posY;
    const double d2 = player->posZ;
    double d3 = 0.0;
    const double d4 = packetIn.getPositionX() - lastPosX;
    const double d5 = packetIn.getPositionY() - lastPosY;
    const double d6 = packetIn.getPositionZ() - lastPosZ;

    if (packetIn.isMoving()) {
        d3 = d4 * d4 + d5 * d5 + d6 * d6;
        if (!hasMoved && d3 < 0.25)
            hasMoved = true;
    }

    if (!hasMoved) {
        if (networkTickCount - field_175090_f > 20)
            setPlayerLocation(lastPosX, lastPosY, lastPosZ, player->rotationYaw, player->rotationPitch);
        return;
    }

    field_175090_f = networkTickCount;

    if (player->ridingEntity != nullptr) {
        float f4 = player->rotationYaw;
        float f = player->rotationPitch;
        player->ridingEntity->updateRiderPosition();
        const double d16 = player->posX;
        const double d17 = player->posY;
        const double d18 = player->posZ;

        if (packetIn.getRotating()) {
            f4 = packetIn.getYaw();
            f = packetIn.getPitch();
        }

        player->onGround = packetIn.isOnGround();
        player->onUpdateEntity();
        player->setPositionAndRotation(d16, d17, d18, f4, f);

        if (player->ridingEntity != nullptr)
            player->ridingEntity->updateRiderPosition();

        serverController->getConfigurationManager()->serverUpdateMountedMovingPlayer(player.get());

        if (player->ridingEntity != nullptr) {
            if (d3 > 4.0) {
                Entity* entity = player->ridingEntity.get();
                player->playerNetServerHandler->sendPacket(std::make_shared<S18PacketEntityTeleport>(entity));
                setPlayerLocation(player->posX, player->posY, player->posZ, player->rotationYaw, player->rotationPitch);
            }
            player->ridingEntity->isAirBorne = true;
        }

        if (hasMoved) {
            lastPosX = player->posX;
            lastPosY = player->posY;
            lastPosZ = player->posZ;
        }

        worldserver->updateEntity(player.get());
        return;
    }

    if (player->isPlayerSleeping()) {
        player->onUpdateEntity();
        player->setPositionAndRotation(lastPosX, lastPosY, lastPosZ, player->rotationYaw, player->rotationPitch);
        worldserver->updateEntity(player.get());
        return;
    }

    const double d7 = player->posY;
    lastPosX = player->posX;
    lastPosY = player->posY;
    lastPosZ = player->posZ;
    double d8 = player->posX;
    double d9 = player->posY;
    double d10 = player->posZ;
    float f1 = player->rotationYaw;
    float f2 = player->rotationPitch;

    if (packetIn.isMoving() && packetIn.getPositionY() == -999.0)
        packetIn.setMoving(false);

    if (packetIn.isMoving()) {
        d8 = packetIn.getPositionX();
        d9 = packetIn.getPositionY();
        d10 = packetIn.getPositionZ();
        if (std::abs(packetIn.getPositionX()) > 3.0E7 || std::abs(packetIn.getPositionZ()) > 3.0E7) {
            kickPlayerFromServer("Illegal position");
            return;
        }
    }

    if (packetIn.getRotating()) {
        f1 = packetIn.getYaw();
        f2 = packetIn.getPitch();
    }

    player->onUpdateEntity();
    player->setPositionAndRotation(lastPosX, lastPosY, lastPosZ, f1, f2);

    if (!hasMoved)
        return;

    double d11 = d8 - player->posX;
    double d12 = d9 - player->posY;
    double d13 = d10 - player->posZ;
    const double d14 = player->motionX * player->motionX + player->motionY * player->motionY + player->motionZ * player->motionZ;
    double d15 = d11 * d11 + d12 * d12 + d13 * d13;

    if (d15 - d14 > 100.0 && (!serverController->isSinglePlayer() || serverController->getServerOwner() != player->getName())) {
        Log::warn("{} moved too quickly! {},{},{} ({}, {}, {})", player->getName(), d11, d12, d13, d11, d12, d13);
        setPlayerLocation(lastPosX, lastPosY, lastPosZ, player->rotationYaw, player->rotationPitch);
        return;
    }

    const float f3 = 0.0625f;
    const bool flag = worldserver->getCollidingBoundingBoxes(player.get(), player->getEntityBoundingBox().contract(f3, f3, f3)).empty();

    if (player->onGround && !packetIn.isOnGround() && d12 > 0.0)
        player->jump();

    player->moveEntity(d11, d12, d13);
    player->onGround = packetIn.isOnGround();
    d11 = d8 - player->posX;
    d12 = d9 - player->posY;

    // Java: always true (a vanilla slip), so the vertical difference never counts.
    if (d12 > -0.5 || d12 < 0.5)
        d12 = 0.0;

    d13 = d10 - player->posZ;
    d15 = d11 * d11 + d12 * d12 + d13 * d13;
    bool flag1 = false;

    if (d15 > 0.0625 && !player->isPlayerSleeping() && !player->theItemInWorldManager->isCreative()) {
        flag1 = true;
        Log::warn("{} moved wrongly!", player->getName());
    }

    player->setPositionAndRotation(d8, d9, d10, f1, f2);
    player->addMovementStat(player->posX - d0, player->posY - d1, player->posZ - d2);

    if (!player->noClip) {
        const bool flag2 = worldserver->getCollidingBoundingBoxes(player.get(), player->getEntityBoundingBox().contract(f3, f3, f3)).empty();
        if (flag && (flag1 || !flag2) && !player->isPlayerSleeping()) {
            setPlayerLocation(lastPosX, lastPosY, lastPosZ, f1, f2);
            return;
        }
    }

    const AxisAlignedBB axisalignedbb = player->getEntityBoundingBox().expand(f3, f3, f3).addCoord(0.0, -0.55, 0.0);

    if (!serverController->isFlightAllowed() && !player->capabilities.allowFlying && !worldserver->checkBlockCollision(axisalignedbb)) {
        if (d12 >= -0.03125) {
            ++floatingTickCount;
            if (floatingTickCount > 80) {
                Log::warn("{} was kicked for floating too long!", player->getName());
                kickPlayerFromServer("Flying is not enabled on this server");
                return;
            }
        }
    } else {
        floatingTickCount = 0;
    }

    player->onGround = packetIn.isOnGround();
    serverController->getConfigurationManager()->serverUpdateMountedMovingPlayer(player.get());
    player->handleFalling(player->posY - d7, packetIn.isOnGround());
}

void NetHandlerPlayServer::setPlayerLocation(double x, double y, double z, float yaw, float pitch, const std::set<S08PacketPlayerPosLook::EnumFlags>& relativeSet)
{
    using Flags = S08PacketPlayerPosLook::EnumFlags;
    hasMoved = false;
    lastPosX = x;
    lastPosY = y;
    lastPosZ = z;

    if (relativeSet.contains(Flags::X))
        lastPosX += playerEntity->posX;
    if (relativeSet.contains(Flags::Y))
        lastPosY += playerEntity->posY;
    if (relativeSet.contains(Flags::Z))
        lastPosZ += playerEntity->posZ;

    float f = yaw;
    float f1 = pitch;

    if (relativeSet.contains(Flags::Y_ROT))
        f = yaw + playerEntity->rotationYaw;
    if (relativeSet.contains(Flags::X_ROT))
        f1 = pitch + playerEntity->rotationPitch;

    playerEntity->setPositionAndRotation(lastPosX, lastPosY, lastPosZ, f, f1);
    playerEntity->playerNetServerHandler->sendPacket(std::make_shared<S08PacketPlayerPosLook>(x, y, z, yaw, pitch, relativeSet));
}

void NetHandlerPlayServer::processPlayerDigging(C07PacketPlayerDigging& packetIn)
{
    using Action = C07PacketPlayerDigging::Action;
    WorldServer* worldserver = serverController->worldServerForDimension(playerEntity->dimension);
    const BlockPos blockpos = packetIn.getPosition();
    playerEntity->markPlayerActive();

    switch (packetIn.getStatus()) {
    case Action::DROP_ITEM:
        if (!playerEntity->isSpectator())
            playerEntity->dropOneItem(false);
        return;
    case Action::DROP_ALL_ITEMS:
        if (!playerEntity->isSpectator())
            playerEntity->dropOneItem(true);
        return;
    case Action::RELEASE_USE_ITEM:
        playerEntity->stopUsingItem();
        return;
    case Action::START_DESTROY_BLOCK:
    case Action::ABORT_DESTROY_BLOCK:
    case Action::STOP_DESTROY_BLOCK: {
        const double d0 = playerEntity->posX - (blockpos.getX() + 0.5);
        const double d1 = playerEntity->posY - (blockpos.getY() + 0.5) + 1.5;
        const double d2 = playerEntity->posZ - (blockpos.getZ() + 0.5);
        const double d3 = d0 * d0 + d1 * d1 + d2 * d2;

        if (d3 > 36.0)
            return;
        if (blockpos.getY() >= serverController->getBuildLimit())
            return;

        if (packetIn.getStatus() == Action::START_DESTROY_BLOCK) {
            if (!serverController->isBlockProtected(worldserver, blockpos, playerEntity.get()) && worldserver->getWorldBorder().contains(blockpos))
                playerEntity->theItemInWorldManager->onBlockClicked(blockpos, packetIn.getFacing());
            else
                playerEntity->playerNetServerHandler->sendPacket(std::make_shared<S23PacketBlockChange>(worldserver, blockpos));
        } else {
            if (packetIn.getStatus() == Action::STOP_DESTROY_BLOCK)
                playerEntity->theItemInWorldManager->blockRemoving(blockpos);
            else if (packetIn.getStatus() == Action::ABORT_DESTROY_BLOCK)
                playerEntity->theItemInWorldManager->cancelDestroyingBlock();

            if (&worldserver->getBlockState(blockpos)->getBlock()->getMaterial() != &Material::air)
                playerEntity->playerNetServerHandler->sendPacket(std::make_shared<S23PacketBlockChange>(worldserver, blockpos));
        }
        return;
    }
    }
    throw std::invalid_argument("Invalid player action");
}

void NetHandlerPlayServer::processPlayerBlockPlacement(C08PacketPlayerBlockPlacement& packetIn)
{
    WorldServer* worldserver = serverController->worldServerForDimension(playerEntity->dimension);
    std::shared_ptr<ItemStack> itemstack = playerEntity->inventory->getCurrentItem();
    bool flag = false;
    const BlockPos blockpos = packetIn.getPosition();
    const EnumFacing enumfacing = getFront(packetIn.getPlacedBlockDirection());
    playerEntity->markPlayerActive();

    if (packetIn.getPlacedBlockDirection() == 255) {
        if (!itemstack)
            return;
        playerEntity->theItemInWorldManager->tryUseItem(playerEntity.get(), worldserver, itemstack);
    } else if (blockpos.getY() < serverController->getBuildLimit() - 1 || (enumfacing != EnumFacing::UP && blockpos.getY() < serverController->getBuildLimit())) {
        if (hasMoved && playerEntity->getDistanceSq(blockpos.getX() + 0.5, blockpos.getY() + 0.5, blockpos.getZ() + 0.5) < 64.0
            && !serverController->isBlockProtected(worldserver, blockpos, playerEntity.get()) && worldserver->getWorldBorder().contains(blockpos)) {
            playerEntity->theItemInWorldManager->activateBlockOrUseItem(playerEntity.get(), worldserver, itemstack, blockpos, enumfacing,
                packetIn.getPlacedBlockOffsetX(), packetIn.getPlacedBlockOffsetY(), packetIn.getPlacedBlockOffsetZ());
        }
        flag = true;
    } else {
        auto chatcomponenttranslation = std::make_shared<ChatComponentTranslation>(
            "build.tooHigh", std::vector<ChatComponentTranslation::Argument> { std::to_string(serverController->getBuildLimit()) });
        playerEntity->playerNetServerHandler->sendPacket(std::make_shared<S02PacketChat>(coloured(chatcomponenttranslation, 'c')));
        flag = true;
    }

    if (flag) {
        playerEntity->playerNetServerHandler->sendPacket(std::make_shared<S23PacketBlockChange>(worldserver, blockpos));
        playerEntity->playerNetServerHandler->sendPacket(std::make_shared<S23PacketBlockChange>(worldserver, blockpos.offset(enumfacing)));
    }

    itemstack = playerEntity->inventory->getCurrentItem();
    InventoryPlayer& inventory = *playerEntity->inventory;

    if (itemstack && itemstack->stackSize == 0) {
        inventory.mainInventory[static_cast<size_t>(inventory.currentItem)] = nullptr;
        itemstack = nullptr;
    }

    if (!itemstack || itemstack->getMaxItemUseDuration() == 0) {
        playerEntity->isChangingQuantityOnly = true;
        inventory.mainInventory[static_cast<size_t>(inventory.currentItem)] = ItemStack::copyItemStack(inventory.mainInventory[static_cast<size_t>(inventory.currentItem)].get());
        Slot* slot = playerEntity->openContainer->getSlotFromInventory(&inventory, inventory.currentItem);
        playerEntity->openContainer->detectAndSendChanges();
        playerEntity->isChangingQuantityOnly = false;

        if (!ItemStack::areItemStacksEqual(inventory.getCurrentItem().get(), packetIn.getStack().get()))
            sendPacket(std::make_shared<S2FPacketSetSlot>(playerEntity->openContainer->windowId, slot->slotNumber, inventory.getCurrentItem().get()));
    }
}

void NetHandlerPlayServer::handleSpectate(C18PacketSpectate& packetIn)
{
    if (!playerEntity->isSpectator())
        return;

    Entity* entity = nullptr;
    for (const auto& worldserver : serverController->worldServers) {
        if (worldserver) {
            entity = worldserver->getEntityFromUuid(packetIn.getId());
            if (entity != nullptr)
                break;
        }
    }

    if (entity == nullptr)
        return;

    playerEntity->setSpectatingEntity(playerEntity.get());
    playerEntity->mountEntity(nullptr);

    if (entity->worldObj != playerEntity->worldObj) {
        WorldServer* worldserver1 = playerEntity->getServerForPlayer();
        auto* worldserver2 = static_cast<WorldServer*>(entity->worldObj);
        playerEntity->dimension = entity->dimension;
        sendPacket(std::make_shared<S07PacketRespawn>(
            playerEntity->dimension, worldserver1->getDifficulty(), worldserver1->getWorldInfo().getTerrainType(), playerEntity->theItemInWorldManager->getGameType()));
        const std::shared_ptr<EntityPlayerMP> keepAlive = playerEntity;
        worldserver1->removePlayerEntityDangerously(playerEntity.get());
        playerEntity->isDead = false;
        playerEntity->setLocationAndAngles(entity->posX, entity->posY, entity->posZ, entity->rotationYaw, entity->rotationPitch);

        if (playerEntity->isEntityAlive()) {
            worldserver1->updateEntityWithOptionalForce(playerEntity.get(), false);
            worldserver2->spawnEntityInWorld(playerEntity);
            worldserver2->updateEntityWithOptionalForce(playerEntity.get(), false);
        }

        playerEntity->setWorld(worldserver2);
        serverController->getConfigurationManager()->preparePlayer(playerEntity.get(), worldserver1);
        playerEntity->setPositionAndUpdate(entity->posX, entity->posY, entity->posZ);
        playerEntity->theItemInWorldManager->setWorld(worldserver2);
        serverController->getConfigurationManager()->updateTimeAndWeatherForPlayer(playerEntity.get(), worldserver2);
        serverController->getConfigurationManager()->syncPlayerInventory(playerEntity.get());
    } else {
        playerEntity->setPositionAndUpdate(entity->posX, entity->posY, entity->posZ);
    }
}

void NetHandlerPlayServer::onDisconnect(const std::shared_ptr<IChatComponent>& reason)
{
    Log::info("{} lost connection: {}", playerEntity->getName(), reason->getUnformattedText());
    auto chatcomponenttranslation
        = std::make_shared<ChatComponentTranslation>("multiplayer.player.left", std::vector<ChatComponentTranslation::Argument> { playerEntity->getDisplayName() });
    serverController->getConfigurationManager()->sendChatMsg(coloured(chatcomponenttranslation, 'e'));
    playerEntity->mountEntityAndWakeUp();
    serverController->getConfigurationManager()->playerLoggedOut(playerEntity.get());
    // Break the player <-> handler cycle (Java's garbage collector frees both).
    const std::shared_ptr<NetHandlerPlayServer> self = shared_from_this();
    const std::shared_ptr<EntityPlayerMP> player = playerEntity;
    if (player->playerNetServerHandler.get() == this)
        player->playerNetServerHandler.reset();

    if (serverController->isSinglePlayer() && player->getName() == serverController->getServerOwner()) {
        Log::info("Stopping singleplayer server as player logged out");
        serverController->initiateShutdown();
    }
}

void NetHandlerPlayServer::sendPacket(const std::shared_ptr<Packet>& packetIn)
{
    if (const auto* s02packetchat = dynamic_cast<const S02PacketChat*>(packetIn.get())) {
        const EnumChatVisibility visibility = playerEntity->getChatVisibility();
        if (visibility == EnumChatVisibility::HIDDEN)
            return;
        if (visibility == EnumChatVisibility::SYSTEM && !s02packetchat->isChat())
            return;
    }

    netManager->sendPacket(packetIn);
}

void NetHandlerPlayServer::processHeldItemChange(C09PacketHeldItemChange& packetIn)
{
    if (packetIn.getSlotId() >= 0 && packetIn.getSlotId() < InventoryPlayer::getHotbarSize()) {
        playerEntity->inventory->currentItem = packetIn.getSlotId();
        playerEntity->markPlayerActive();
    } else {
        Log::warn("{} tried to set an invalid carried item", playerEntity->getName());
    }
}

void NetHandlerPlayServer::processChatMessage(C01PacketChatMessage& packetIn)
{
    if (playerEntity->getChatVisibility() == EnumChatVisibility::HIDDEN) {
        sendPacket(std::make_shared<S02PacketChat>(coloured(std::make_shared<ChatComponentTranslation>("chat.cannotSend"), 'c')));
        return;
    }

    playerEntity->markPlayerActive();
    const std::u16string s = normalizeSpace(toUtf16(packetIn.getMessage()));

    for (char16_t c : s) {
        if (!ChatAllowedCharacters::isAllowedCharacter(c)) {
            kickPlayerFromServer("Illegal characters in chat");
            return;
        }
    }

    const std::string message = fromUtf16(s);
    if (message.starts_with("/")) {
        // Java: serverController.getCommandManager().executeCommand -- the commands, stage 12.
        playerEntity->addChatMessage(coloured(std::make_shared<ChatComponentTranslation>("commands.generic.notFound"), 'c'));
    } else {
        auto ichatcomponent = std::make_shared<ChatComponentTranslation>(
            "chat.type.text", std::vector<ChatComponentTranslation::Argument> { playerEntity->getDisplayName(), message });
        serverController->getConfigurationManager()->sendChatMsgImpl(ichatcomponent, false);
    }

    chatSpamThresholdCount += 20;
    if (chatSpamThresholdCount > 200 && !serverController->getConfigurationManager()->canSendCommands(playerEntity->getGameProfile()))
        kickPlayerFromServer("disconnect.spam");
}

void NetHandlerPlayServer::handleAnimation(C0APacketAnimation&)
{
    playerEntity->markPlayerActive();
    playerEntity->swingItem();
}

void NetHandlerPlayServer::processEntityAction(C0BPacketEntityAction& packetIn)
{
    using Action = C0BPacketEntityAction::Action;
    playerEntity->markPlayerActive();

    switch (packetIn.getAction()) {
    case Action::START_SNEAKING:
        playerEntity->setSneaking(true);
        break;
    case Action::STOP_SNEAKING:
        playerEntity->setSneaking(false);
        break;
    case Action::START_SPRINTING:
        playerEntity->setSprinting(true);
        break;
    case Action::STOP_SPRINTING:
        playerEntity->setSprinting(false);
        break;
    case Action::STOP_SLEEPING:
        playerEntity->wakeUpPlayer(false, true, true);
        hasMoved = false;
        break;
    case Action::RIDING_JUMP:
    case Action::OPEN_INVENTORY:
        // Java: the horse's jump power and inventory -- the horse, stage 10.
        break;
    }
}

void NetHandlerPlayServer::processUseEntity(C02PacketUseEntity& packetIn)
{
    using Action = C02PacketUseEntity::Action;
    WorldServer* worldserver = serverController->worldServerForDimension(playerEntity->dimension);
    Entity* entity = packetIn.getEntityFromWorld(worldserver);
    playerEntity->markPlayerActive();

    if (entity == nullptr)
        return;

    const bool flag = playerEntity->canEntityBeSeen(entity);
    double d0 = 36.0;
    if (!flag)
        d0 = 9.0;

    if (playerEntity->getDistanceSqToEntity(entity) < d0) {
        if (packetIn.getAction() == Action::INTERACT) {
            playerEntity->interactWith(entity);
        } else if (packetIn.getAction() == Action::INTERACT_AT) {
            entity->interactAt(playerEntity.get(), *packetIn.getHitVec());
        } else if (packetIn.getAction() == Action::ATTACK) {
            // Java: also items, experience orbs and arrows -- with those entities (stage 10).
            if (entity == playerEntity.get()) {
                kickPlayerFromServer("Attempting to attack an invalid entity");
                Log::warn("Player {} tried to attack an invalid entity", playerEntity->getName());
                return;
            }
            playerEntity->attackTargetEntityWithCurrentItem(entity);
        }
    }
}

void NetHandlerPlayServer::processClientStatus(C16PacketClientStatus& packetIn)
{
    using EnumState = C16PacketClientStatus::EnumState;
    playerEntity->markPlayerActive();

    switch (packetIn.getStatus()) {
    case EnumState::PERFORM_RESPAWN:
        if (playerEntity->playerConqueredTheEnd) {
            playerEntity = serverController->getConfigurationManager()->recreatePlayerEntity(playerEntity.get(), 0, true);
        } else if (playerEntity->getServerForPlayer()->getWorldInfo().isHardcoreModeEnabled()) {
            if (serverController->isSinglePlayer() && playerEntity->getName() == serverController->getServerOwner()) {
                playerEntity->playerNetServerHandler->kickPlayerFromServer("You have died. Game over, man, it's game over!");
                serverController->deleteWorldAndStopServer();
            } else {
                // Java: bans the profile ("Death in Hardcore") -- the ban list comes with multiplayer.
                playerEntity->playerNetServerHandler->kickPlayerFromServer("You have died. Game over, man, it's game over!");
            }
        } else {
            if (playerEntity->getHealth() > 0.0f)
                return;
            playerEntity = serverController->getConfigurationManager()->recreatePlayerEntity(playerEntity.get(), 0, false);
        }
        break;
    case EnumState::REQUEST_STATS:
    case EnumState::OPEN_INVENTORY_ACHIEVEMENT:
        // Java: the statistics and the "Taking Inventory" achievement -- stage 12.
        break;
    }
}

void NetHandlerPlayServer::processCloseWindow(C0DPacketCloseWindow&)
{
    playerEntity->closeContainer();
}

void NetHandlerPlayServer::processClickWindow(C0EPacketClickWindow& packetIn)
{
    playerEntity->markPlayerActive();
    Container* container = playerEntity->openContainer;

    if (container->windowId != packetIn.getWindowId() || !container->getCanCraft(playerEntity.get()))
        return;

    const auto allStacks = [container] {
        std::vector<std::shared_ptr<ItemStack>> list;
        for (size_t i = 0; i < container->inventorySlots.size(); ++i)
            list.push_back(container->inventorySlots[i]->getStack());
        return list;
    };

    if (playerEntity->isSpectator()) {
        playerEntity->updateCraftingInventory(container, allStacks());
        return;
    }

    const std::shared_ptr<ItemStack> itemstack = container->slotClick(packetIn.getSlotId(), packetIn.getUsedButton(), packetIn.getMode(), playerEntity.get());

    if (ItemStack::areItemStacksEqual(packetIn.getClickedItem().get(), itemstack.get())) {
        playerEntity->playerNetServerHandler->sendPacket(std::make_shared<S32PacketConfirmTransaction>(packetIn.getWindowId(), packetIn.getActionNumber(), true));
        playerEntity->isChangingQuantityOnly = true;
        container->detectAndSendChanges();
        playerEntity->updateHeldItem();
        playerEntity->isChangingQuantityOnly = false;
    } else {
        field_147372_n[container->windowId] = packetIn.getActionNumber();
        playerEntity->playerNetServerHandler->sendPacket(std::make_shared<S32PacketConfirmTransaction>(packetIn.getWindowId(), packetIn.getActionNumber(), false));
        container->setCanCraft(playerEntity.get(), false);
        playerEntity->updateCraftingInventory(container, allStacks());
    }
}

void NetHandlerPlayServer::processEnchantItem(C11PacketEnchantItem& packetIn)
{
    playerEntity->markPlayerActive();
    Container* container = playerEntity->openContainer;
    if (container->windowId == packetIn.getWindowId() && container->getCanCraft(playerEntity.get()) && !playerEntity->isSpectator()) {
        container->enchantItem(playerEntity.get(), packetIn.getButton());
        container->detectAndSendChanges();
    }
}

void NetHandlerPlayServer::processCreativeInventoryAction(C10PacketCreativeInventoryAction& packetIn)
{
    if (!playerEntity->theItemInWorldManager->isCreative())
        return;

    const bool flag = packetIn.getSlotId() < 0;
    const std::shared_ptr<ItemStack> itemstack = packetIn.getStack();

    if (itemstack && itemstack->hasTagCompound() && itemstack->getTagCompound()->hasKey("BlockEntityTag", 10)) {
        const auto nbttagcompound = itemstack->getTagCompound()->getCompoundTag("BlockEntityTag");
        if (nbttagcompound->hasKey("x") && nbttagcompound->hasKey("y") && nbttagcompound->hasKey("z")) {
            const BlockPos blockpos(nbttagcompound->getInteger("x"), nbttagcompound->getInteger("y"), nbttagcompound->getInteger("z"));
            if (TileEntity* tileentity = playerEntity->worldObj->getTileEntity(blockpos)) {
                auto nbttagcompound1 = std::make_shared<NBTTagCompound>();
                tileentity->writeToNBT(*nbttagcompound1);
                nbttagcompound1->removeTag("x");
                nbttagcompound1->removeTag("y");
                nbttagcompound1->removeTag("z");
                itemstack->setTagInfo("BlockEntityTag", nbttagcompound1);
            }
        }
    }

    const bool flag1 = packetIn.getSlotId() >= 1 && packetIn.getSlotId() < 36 + InventoryPlayer::getHotbarSize();
    const bool flag2 = !itemstack || itemstack->getItem() != nullptr;
    const bool flag3 = !itemstack || (itemstack->getMetadata() >= 0 && itemstack->stackSize <= 64 && itemstack->stackSize > 0);

    if (flag1 && flag2 && flag3) {
        playerEntity->inventoryContainer->putStackInSlot(packetIn.getSlotId(), itemstack);
        playerEntity->inventoryContainer->setCanCraft(playerEntity.get(), true);
    } else if (flag && flag2 && flag3 && itemDropThreshold < 200) {
        itemDropThreshold += 20;
        // Java: dropPlayerItemWithRandomChoice and setAgeToCreativeDespawnTime -- item entities, stage 10.
        playerEntity->dropPlayerItemWithRandomChoice(itemstack, true);
    }
}

void NetHandlerPlayServer::processConfirmTransaction(C0FPacketConfirmTransaction& packetIn)
{
    Container* container = playerEntity->openContainer;
    const auto it = field_147372_n.find(container->windowId);
    if (it != field_147372_n.end() && packetIn.getUid() == it->second && container->windowId == packetIn.getWindowId() && !container->getCanCraft(playerEntity.get())
        && !playerEntity->isSpectator())
        container->setCanCraft(playerEntity.get(), true);
}

void NetHandlerPlayServer::processUpdateSign(C12PacketUpdateSign&)
{
    playerEntity->markPlayerActive();
    // Java: writes the lines into the TileEntitySign the player is editing -- with the sign (stage 11).
}

void NetHandlerPlayServer::processKeepAlive(C00PacketKeepAlive& packetIn)
{
    if (packetIn.getKey() == field_147378_h) {
        const int i = static_cast<int>(currentTimeMillis() - lastPingTime);
        playerEntity->ping = (playerEntity->ping * 3 + i) / 4;
    }
}

void NetHandlerPlayServer::processPlayerAbilities(C13PacketPlayerAbilities& packetIn)
{
    playerEntity->capabilities.isFlying = packetIn.isFlying() && playerEntity->capabilities.allowFlying;
}

void NetHandlerPlayServer::processTabComplete(C14PacketTabComplete&)
{
    // Java: serverController.getTabCompletions -- the commands, stage 12. No completions.
    playerEntity->playerNetServerHandler->sendPacket(std::make_shared<S3APacketTabComplete>(std::vector<std::string> {}));
}

void NetHandlerPlayServer::processClientSettings(C15PacketClientSettings& packetIn)
{
    playerEntity->handleClientSettings(packetIn);
}

void NetHandlerPlayServer::processVanilla250Packet(C17PacketCustomPayload&)
{
    // Java: books, trades, command blocks, beacons and item names -- stages 11 and 12. MC|Brand is ignored.
}

} // namespace mc

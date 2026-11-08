#include "entity/player/EntityPlayerMP.h"

#include "core/Log.h"

#include "block/AllBlocks.h"
#include "block/material/Material.h"
#include "entity/EntityTracker.h"
#include "init/Items.h"
#include "inventory/ContainerChest.h"
#include "inventory/InventoryCrafting.h"
#include "item/ItemStack.h"
#include "nbt/NBTTagCompound.h"
#include "network/NetHandlerPlayServer.h"
#include "network/play/ServerPlayPackets.h"
#include "server/MinecraftServer.h"
#include "server/management/ItemInWorldManager.h"
#include "server/management/ServerConfigurationManager.h"
#include "tileentity/TileEntities.h"
#include "util/ChatComponent.h"
#include "util/DamageSource.h"
#include "util/MathHelper.h"
#include "world/GameRules.h"
#include "world/WorldProvider.h"
#include "world/WorldServer.h"
#include "world/border/WorldBorder.h"
#include "world/chunk/Chunk.h"
#include "world/storage/WorldInfo.h"

#include <algorithm>
#include <chrono>
#include <climits>

namespace mc {

namespace {

int64_t currentTimeMillis()
{
    return std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
}

// Java: IWorldNameable.getDisplayName for inventories.
std::shared_ptr<IChatComponent> inventoryDisplayName(const IInventory& inventory)
{
    if (inventory.hasCustomName())
        return std::make_shared<ChatComponentText>(inventory.getName());
    return std::make_shared<ChatComponentTranslation>(inventory.getName());
}

} // namespace

EntityPlayerMP::EntityPlayerMP(MinecraftServer* server, WorldServer* worldIn, const GameProfile& profile, std::unique_ptr<ItemInWorldManager> interactionManager)
    : EntityPlayer(worldIn, profile)
    , mcServer(server)
    , theItemInWorldManager(std::move(interactionManager))
    , playerLastActiveTime(currentTimeMillis())
{
}

EntityPlayerMP::~EntityPlayerMP() = default;

void EntityPlayerMP::construct()
{
    EntityPlayer::construct();
    theItemInWorldManager->thisPlayerMP = this;
    BlockPos blockpos = worldObj->getSpawnPoint();
    if (!worldObj->provider->getHasNoSky() && worldObj->getWorldInfo().getGameType() != GameType::ADVENTURE) {
        int i = std::max(5, mcServer->getSpawnProtectionSize() - 6);
        const int j = MathHelper::floor_double(worldObj->getWorldBorder().getClosestDistance(static_cast<double>(blockpos.getX()), static_cast<double>(blockpos.getZ())));
        if (j < i)
            i = j;
        if (j <= 1)
            i = 1;
        const int dx = rand.nextInt(i * 2) - i;
        const int dz = rand.nextInt(i * 2) - i;
        blockpos = worldObj->getTopSolidOrLiquidBlock(blockpos.add(dx, 0, dz));
    }
    // Java: statsFile = server.getConfigurationManager().getPlayerStatsFile(this) -- statistics.
    stepHeight = 0.0f;
    moveToBlockPosAndAngles(blockpos, 0.0f, 0.0f);
    while (!worldObj->getCollidingBoundingBoxes(this, getEntityBoundingBox()).empty() && posY < 255.0)
        setPosition(posX, posY + 1.0, posZ);
}

void EntityPlayerMP::readEntityFromNBT(const NBTTagCompound& tagCompund)
{
    EntityPlayer::readEntityFromNBT(tagCompund);
    if (tagCompund.hasKey("playerGameType", 99)) {
        if (mcServer->getForceGamemode())
            theItemInWorldManager->setGameType(mcServer->getGameType());
        else
            theItemInWorldManager->setGameType(getGameTypeByID(tagCompund.getInteger("playerGameType")));
    }
}

void EntityPlayerMP::writeEntityToNBT(NBTTagCompound& tagCompound)
{
    EntityPlayer::writeEntityToNBT(tagCompound);
    tagCompound.setInteger("playerGameType", getGameTypeID(theItemInWorldManager->getGameType()));
}

void EntityPlayerMP::addExperienceLevel(int levels)
{
    EntityPlayer::addExperienceLevel(levels);
    lastExperience = -1;
}

void EntityPlayerMP::removeExperienceLevel(int levels)
{
    EntityPlayer::removeExperienceLevel(levels);
    lastExperience = -1;
}

void EntityPlayerMP::addSelfToInternalCraftingInventory()
{
    openContainer->onCraftGuiOpened(this);
}

void EntityPlayerMP::sendEnterCombat()
{
    EntityPlayer::sendEnterCombat();
    playerNetServerHandler->sendPacket(std::make_shared<S42PacketCombatEvent>(getCombatTracker(), S42PacketCombatEvent::Event::ENTER_COMBAT));
}

void EntityPlayerMP::sendEndCombat()
{
    EntityPlayer::sendEndCombat();
    playerNetServerHandler->sendPacket(std::make_shared<S42PacketCombatEvent>(getCombatTracker(), S42PacketCombatEvent::Event::END_COMBAT));
}

void EntityPlayerMP::onUpdate()
{
    theItemInWorldManager->updateBlockRemoving();
    --respawnInvulnerabilityTicks;
    if (hurtResistantTime > 0)
        --hurtResistantTime;
    openContainer->detectAndSendChanges();
    if (!worldObj->isRemote && !openContainer->canInteractWith(this)) {
        closeScreen();
        setOpenContainer(inventoryContainer);
    }
    while (!destroyedItemsNetCache.empty()) {
        const size_t i = std::min(destroyedItemsNetCache.size(), static_cast<size_t>(INT_MAX));
        std::vector<int> aint;
        aint.reserve(i);
        while (!destroyedItemsNetCache.empty() && aint.size() < i) {
            aint.push_back(destroyedItemsNetCache.front());
            destroyedItemsNetCache.pop_front();
        }
        playerNetServerHandler->sendPacket(std::make_shared<S13PacketDestroyEntities>(std::move(aint)));
    }
    if (!loadedChunks.empty()) {
        std::vector<Chunk*> list;
        std::vector<TileEntity*> list1;
        auto iterator1 = loadedChunks.begin();
        while (iterator1 != loadedChunks.end() && list.size() < 10) {
            const ChunkCoordIntPair chunkcoordintpair = *iterator1;
            if (worldObj->isBlockLoaded(BlockPos(chunkcoordintpair.chunkXPos << 4, 0, chunkcoordintpair.chunkZPos << 4))) {
                Chunk* chunk = worldObj->getChunkFromChunkCoords(chunkcoordintpair.chunkXPos, chunkcoordintpair.chunkZPos);
                if (chunk->isPopulated()) {
                    list.push_back(chunk);
                    const std::vector<TileEntity*> tiles = getServerForPlayer()->getTileEntitiesIn(chunkcoordintpair.chunkXPos * 16, 0,
                        chunkcoordintpair.chunkZPos * 16, chunkcoordintpair.chunkXPos * 16 + 16, 256, chunkcoordintpair.chunkZPos * 16 + 16);
                    list1.insert(list1.end(), tiles.begin(), tiles.end());
                    iterator1 = loadedChunks.erase(iterator1);
                    continue;
                }
            }
            ++iterator1;
        }
        if (!list.empty()) {
            if (list.size() == 1)
                playerNetServerHandler->sendPacket(std::make_shared<S21PacketChunkData>(list[0], true, 65535));
            else
                playerNetServerHandler->sendPacket(std::make_shared<S26PacketMapChunkBulk>(list));
            for (TileEntity* tileentity : list1)
                sendTileEntityUpdate(tileentity);
            for (Chunk* chunk1 : list)
                getServerForPlayer()->getEntityTracker()->func_85172_a(this, chunk1);
        }
    }
    Entity* entity = getSpectatingEntity();
    if (entity != this) {
        if (!entity->isEntityAlive()) {
            setSpectatingEntity(this);
        } else {
            setPositionAndRotation(entity->posX, entity->posY, entity->posZ, entity->rotationYaw, entity->rotationPitch);
            mcServer->getConfigurationManager()->serverUpdateMountedMovingPlayer(this);
            if (isSneaking())
                setSpectatingEntity(this);
        }
    }
}

void EntityPlayerMP::onUpdateEntity()
{
    EntityPlayer::onUpdate();
    // Java: for each map in the inventory, ItemMapBase.createMapDataPacket -- maps come with their item hooks.
    if (getHealth() != lastHealth || lastFoodLevel != foodStats.getFoodLevel() || (foodStats.getSaturationLevel() == 0.0f) != wasHungry) {
        playerNetServerHandler->sendPacket(std::make_shared<S06PacketUpdateHealth>(getHealth(), foodStats.getFoodLevel(), foodStats.getSaturationLevel()));
        lastHealth = getHealth();
        lastFoodLevel = foodStats.getFoodLevel();
        wasHungry = foodStats.getSaturationLevel() == 0.0f;
    }
    if (getHealth() + getAbsorptionAmount() != combinedHealth) {
        combinedHealth = getHealth() + getAbsorptionAmount();
        // Java: updates the health objectives of the scoreboard.
    }
    if (experienceTotal != lastExperience) {
        lastExperience = experienceTotal;
        playerNetServerHandler->sendPacket(std::make_shared<S1FPacketSetExperience>(experience, experienceTotal, experienceLevel));
    }
    // Java: every 100 ticks updateBiomesExplored() for the exploreAllBiomes achievement -- statistics.
}

void EntityPlayerMP::onDeath(const DamageSource& cause)
{
    if (worldObj->getGameRules().getBoolean("showDeathMessages")) {
        // Java: a team's death message visibility may limit who sees it -- teams come with the scoreboard.
        mcServer->getConfigurationManager()->sendChatMsg(getCombatTracker().getDeathMessage());
    }
    if (!worldObj->getGameRules().getBoolean("keepInventory"))
        inventory.dropAllItems();
    // Java: the deathCount objectives -- the scoreboard.
    if (EntityLivingBase* entitylivingbase = getAttackingEntity()) {
        // Java: the egg's killed-by statistic -- statistics.
        entitylivingbase->addToPlayerScore(this, scoreValue);
    }
    // Java: triggerAchievement(deathsStat), func_175145_a(timeSinceDeathStat) -- statistics.
    (void)cause;
    getCombatTracker().reset();
}

bool EntityPlayerMP::attackEntityFrom(const DamageSource& source, float amount)
{
    if (isEntityInvulnerable(source))
        return false;
    const bool flag = mcServer->isDedicatedServer() && canPlayersAttack() && source.getDamageType() == "fall";
    if (!flag && respawnInvulnerabilityTicks > 0 && &source != &DamageSource::outOfWorld)
        return false;
    if (dynamic_cast<const EntityDamageSource*>(&source)) {
        Entity* entity = source.getEntity();
        if (auto* player = dynamic_cast<EntityPlayer*>(entity); player && !canAttackPlayer(player))
            return false;
        // Java: an arrow shot by a player that may not be attacked -- with EntityArrow.
    }
    return EntityPlayer::attackEntityFrom(source, amount);
}

bool EntityPlayerMP::canAttackPlayer(EntityPlayer* other)
{
    return !canPlayersAttack() ? false : EntityPlayer::canAttackPlayer(other);
}

bool EntityPlayerMP::canPlayersAttack() const
{
    return mcServer->isPVPEnabled();
}

void EntityPlayerMP::travelToDimension(int dimensionId)
{
    if (dimension == 1 && dimensionId == 1) {
        // Java: triggerAchievement(theEnd2) -- achievements.
        worldObj->removeEntity(this);
        playerConqueredTheEnd = true;
        playerNetServerHandler->sendPacket(std::make_shared<S2BPacketChangeGameState>(4, 0.0f));
    } else {
        if (dimension == 0 && dimensionId == 1) {
            // Java: triggerAchievement(theEnd) -- achievements.
            if (const std::optional<BlockPos> blockpos = mcServer->worldServerForDimension(dimensionId)->getSpawnCoordinate())
                playerNetServerHandler->setPlayerLocation(static_cast<double>(blockpos->getX()), static_cast<double>(blockpos->getY()),
                    static_cast<double>(blockpos->getZ()), 0.0f, 0.0f);
            dimensionId = 1;
        }
        // Java: else triggerAchievement(portal) -- achievements.
        mcServer->getConfigurationManager()->transferPlayerToDimension(this, dimensionId);
        lastExperience = -1;
        lastHealth = -1.0f;
        lastFoodLevel = -1;
    }
}

bool EntityPlayerMP::isSpectatedByPlayer(EntityPlayerMP* player)
{
    return player->isSpectator() ? getSpectatingEntity() == this : (isSpectator() ? false : EntityPlayer::isSpectatedByPlayer(player));
}

void EntityPlayerMP::sendTileEntityUpdate(TileEntity* p_147097_1_)
{
    if (p_147097_1_) {
        if (const std::shared_ptr<Packet> packet = p_147097_1_->getDescriptionPacket())
            playerNetServerHandler->sendPacket(packet);
    }
}

void EntityPlayerMP::onItemPickup(Entity* p_71001_1_, int p_71001_2_)
{
    EntityPlayer::onItemPickup(p_71001_1_, p_71001_2_);
    openContainer->detectAndSendChanges();
}

EntityPlayer::EnumStatus EntityPlayerMP::trySleep(const BlockPos& bedLocation)
{
    const EnumStatus status = EntityPlayer::trySleep(bedLocation);
    if (status == EnumStatus::OK) {
        const std::shared_ptr<Packet> packet = std::make_shared<S0APacketUseBed>(this, bedLocation);
        getServerForPlayer()->getEntityTracker()->sendToAllTrackingEntity(this, packet);
        playerNetServerHandler->setPlayerLocation(posX, posY, posZ, rotationYaw, rotationPitch);
        playerNetServerHandler->sendPacket(packet);
    }
    return status;
}

void EntityPlayerMP::wakeUpPlayer(bool immediately, bool updateWorldFlag, bool setSpawn)
{
    if (isPlayerSleeping())
        getServerForPlayer()->getEntityTracker()->func_151248_b(this, std::make_shared<S0BPacketAnimation>(this, 2));
    EntityPlayer::wakeUpPlayer(immediately, updateWorldFlag, setSpawn);
    if (playerNetServerHandler)
        playerNetServerHandler->setPlayerLocation(posX, posY, posZ, rotationYaw, rotationPitch);
}

void EntityPlayerMP::mountEntity(Entity* entityIn)
{
    const std::shared_ptr<Entity> entity = ridingEntity;
    EntityPlayer::mountEntity(entityIn);
    if (entityIn != entity.get()) {
        playerNetServerHandler->sendPacket(std::make_shared<S1BPacketEntityAttach>(0, this, ridingEntity.get()));
        playerNetServerHandler->setPlayerLocation(posX, posY, posZ, rotationYaw, rotationPitch);
    }
}

void EntityPlayerMP::handleFalling(double p_71122_1_, bool p_71122_3_)
{
    const int i = MathHelper::floor_double(posX);
    const int j = MathHelper::floor_double(posY - 0.20000000298023224);
    const int k = MathHelper::floor_double(posZ);
    BlockPos blockpos(i, j, k);
    Block* block = worldObj->getBlockState(blockpos)->getBlock();
    if (&block->getMaterial() == &Material::air) {
        Block* block1 = worldObj->getBlockState(blockpos.down())->getBlock();
        if (dynamic_cast<BlockFence*>(block1) || dynamic_cast<BlockWall*>(block1) || dynamic_cast<BlockFenceGate*>(block1)) {
            blockpos = blockpos.down();
            block = worldObj->getBlockState(blockpos)->getBlock();
        }
    }
    EntityPlayer::updateFallState(p_71122_1_, p_71122_3_, block, blockpos);
}

void EntityPlayerMP::openEditSign(TileEntitySign*)
{
    // Java: signTile.setPlayer(this) and S36PacketSignEditorOpen(signTile.getPos()) -- with TileEntitySign.
}

void EntityPlayerMP::displayGUIChest(IInventory* chestInventory)
{
    if (openContainer != inventoryContainer.get())
        closeScreen();
    if (auto* lockable = dynamic_cast<TileEntityLockable*>(chestInventory)) {
        if (lockable->isLocked() && !canOpen(lockable->getLockCode()) && !isSpectator()) {
            playerNetServerHandler->sendPacket(std::make_shared<S02PacketChat>(
                std::make_shared<ChatComponentTranslation>("container.isLocked", std::vector<ChatComponentTranslation::Argument> { inventoryDisplayName(*chestInventory) }),
                int8_t(2)));
            playerNetServerHandler->sendPacket(std::make_shared<S29PacketSoundEffect>("random.door_close", posX, posY, posZ, 1.0f, 1.0f));
            return;
        }
    }
    getNextWindowId();
    // Java: an IInteractionObject (every TileEntityLockable) opens its own GUI id and container; of those only
    // the chest's is ported.
    if (dynamic_cast<TileEntityChest*>(chestInventory)) {
        playerNetServerHandler->sendPacket(
            std::make_shared<S2DPacketOpenWindow>(currentWindowId, "minecraft:chest", inventoryDisplayName(*chestInventory), chestInventory->getSizeInventory()));
    } else {
        playerNetServerHandler->sendPacket(
            std::make_shared<S2DPacketOpenWindow>(currentWindowId, "minecraft:container", inventoryDisplayName(*chestInventory), chestInventory->getSizeInventory()));
    }
    setOpenContainer(std::make_shared<ContainerChest>(&inventory, chestInventory, this));
    openContainer->windowId = currentWindowId;
    openContainer->onCraftGuiOpened(this);
}

void EntityPlayerMP::displayGUIBook(const std::shared_ptr<ItemStack>& bookStack)
{
    if (bookStack->getItem() == Items::written_book)
        playerNetServerHandler->sendPacket(std::make_shared<S3FPacketCustomPayload>("MC|BOpen", std::vector<uint8_t>()));
}

void EntityPlayerMP::sendSlotContents(Container* containerToSend, int slotInd, const ItemStack* stack)
{
    if (!dynamic_cast<SlotCrafting*>(containerToSend->getSlot(slotInd))) {
        if (!isChangingQuantityOnly)
            playerNetServerHandler->sendPacket(std::make_shared<S2FPacketSetSlot>(containerToSend->windowId, slotInd, stack));
    }
}

void EntityPlayerMP::sendContainerToPlayer(Container* p_71120_1_)
{
    updateCraftingInventory(p_71120_1_, p_71120_1_->getInventory());
}

void EntityPlayerMP::updateCraftingInventory(Container* containerToSend, const std::vector<std::shared_ptr<ItemStack>>& itemsList)
{
    playerNetServerHandler->sendPacket(std::make_shared<S30PacketWindowItems>(containerToSend->windowId, itemsList));
    playerNetServerHandler->sendPacket(std::make_shared<S2FPacketSetSlot>(-1, -1, inventory.getItemStack().get()));
}

void EntityPlayerMP::sendProgressBarUpdate(Container* containerIn, int varToUpdate, int newValue)
{
    playerNetServerHandler->sendPacket(std::make_shared<S31PacketWindowProperty>(containerIn->windowId, varToUpdate, newValue));
}

void EntityPlayerMP::sendAllWindowProperties(Container* p_175173_1_, IInventory* p_175173_2_)
{
    for (int i = 0; i < p_175173_2_->getFieldCount(); ++i)
        playerNetServerHandler->sendPacket(std::make_shared<S31PacketWindowProperty>(p_175173_1_->windowId, i, p_175173_2_->getField(i)));
}

void EntityPlayerMP::closeScreen()
{
    playerNetServerHandler->sendPacket(std::make_shared<S2EPacketCloseWindow>(openContainer->windowId));
    closeContainer();
}

void EntityPlayerMP::updateHeldItem()
{
    if (!isChangingQuantityOnly)
        playerNetServerHandler->sendPacket(std::make_shared<S2FPacketSetSlot>(-1, -1, inventory.getItemStack().get()));
}

void EntityPlayerMP::closeContainer()
{
    openContainer->onContainerClosed(this);
    setOpenContainer(inventoryContainer);
}

void EntityPlayerMP::setEntityActionState(float p_110430_1_, float p_110430_2_, bool p_110430_3_, bool sneaking)
{
    if (ridingEntity) {
        if (p_110430_1_ >= -1.0f && p_110430_1_ <= 1.0f)
            moveStrafing = p_110430_1_;
        if (p_110430_2_ >= -1.0f && p_110430_2_ <= 1.0f)
            moveForward = p_110430_2_;
        isJumping = p_110430_3_;
        setSneaking(sneaking);
    }
}

void EntityPlayerMP::mountEntityAndWakeUp()
{
    if (riddenByEntity)
        riddenByEntity->mountEntity(this);
    if (sleeping)
        wakeUpPlayer(true, false, false);
}

void EntityPlayerMP::addChatComponentMessage(std::shared_ptr<IChatComponent> chatComponent)
{
    playerNetServerHandler->sendPacket(std::make_shared<S02PacketChat>(std::move(chatComponent)));
}

void EntityPlayerMP::onItemUseFinish()
{
    playerNetServerHandler->sendPacket(std::make_shared<S19PacketEntityStatus>(this, int8_t(9)));
    EntityPlayer::onItemUseFinish();
}

void EntityPlayerMP::setItemInUse(const std::shared_ptr<ItemStack>& stack, int duration)
{
    EntityPlayer::setItemInUse(stack, duration);
    if (stack && stack->getItem() && stack->getItem()->getItemUseAction(*stack) == EnumAction::EAT)
        getServerForPlayer()->getEntityTracker()->func_151248_b(this, std::make_shared<S0BPacketAnimation>(this, 3));
}

void EntityPlayerMP::clonePlayer(EntityPlayer* oldPlayer, bool respawnFromEnd)
{
    EntityPlayer::clonePlayer(oldPlayer, respawnFromEnd);
    lastExperience = -1;
    lastHealth = -1.0f;
    lastFoodLevel = -1;
    const auto* old = static_cast<EntityPlayerMP*>(oldPlayer);
    destroyedItemsNetCache.insert(destroyedItemsNetCache.end(), old->destroyedItemsNetCache.begin(), old->destroyedItemsNetCache.end());
}

void EntityPlayerMP::onNewPotionEffect(const PotionEffect& id)
{
    EntityPlayer::onNewPotionEffect(id);
    playerNetServerHandler->sendPacket(std::make_shared<S1DPacketEntityEffect>(getEntityId(), id));
}

void EntityPlayerMP::onChangedPotionEffect(const PotionEffect& id, bool p_70695_2_)
{
    EntityPlayer::onChangedPotionEffect(id, p_70695_2_);
    playerNetServerHandler->sendPacket(std::make_shared<S1DPacketEntityEffect>(getEntityId(), id));
}

void EntityPlayerMP::onFinishedPotionEffect(const PotionEffect& effect)
{
    EntityPlayer::onFinishedPotionEffect(effect);
    playerNetServerHandler->sendPacket(std::make_shared<S1EPacketRemoveEntityEffect>(getEntityId(), effect));
}

void EntityPlayerMP::setPositionAndUpdate(double x, double y, double z)
{
    playerNetServerHandler->setPlayerLocation(x, y, z, rotationYaw, rotationPitch);
}

void EntityPlayerMP::onCriticalHit(Entity* entityHit)
{
    getServerForPlayer()->getEntityTracker()->func_151248_b(this, std::make_shared<S0BPacketAnimation>(entityHit, 4));
}

void EntityPlayerMP::onEnchantmentCritical(Entity* entityHit)
{
    getServerForPlayer()->getEntityTracker()->func_151248_b(this, std::make_shared<S0BPacketAnimation>(entityHit, 5));
}

void EntityPlayerMP::sendPlayerAbilities()
{
    if (playerNetServerHandler) {
        playerNetServerHandler->sendPacket(std::make_shared<S39PacketPlayerAbilities>(capabilities));
        updatePotionMetadata();
    }
}

WorldServer* EntityPlayerMP::getServerForPlayer() const
{
    return static_cast<WorldServer*>(worldObj);
}

void EntityPlayerMP::setGameType(GameType gameType)
{
    theItemInWorldManager->setGameType(gameType);
    playerNetServerHandler->sendPacket(std::make_shared<S2BPacketChangeGameState>(3, static_cast<float>(getGameTypeID(gameType))));
    if (gameType == GameType::SPECTATOR)
        mountEntity(nullptr);
    else
        setSpectatingEntity(this);
    sendPlayerAbilities();
    markPotionsDirty();
}

bool EntityPlayerMP::isSpectator()
{
    return theItemInWorldManager->getGameType() == GameType::SPECTATOR;
}

void EntityPlayerMP::addChatMessage(std::shared_ptr<IChatComponent> component)
{
    playerNetServerHandler->sendPacket(std::make_shared<S02PacketChat>(std::move(component)));
}

bool EntityPlayerMP::canCommandSenderUseCommand(int permLevel, const std::string& commandName)
{
    if (commandName == "seed" && !mcServer->isDedicatedServer())
        return true;
    if (commandName != "tell" && commandName != "help" && commandName != "me" && commandName != "trigger") {
        if (mcServer->getConfigurationManager()->canSendCommands(getGameProfile())) {
            // Java: the op entry's permission level when opped -- the op list comes with commands.
            return mcServer->getOpPermissionLevel() >= permLevel;
        }
        return false;
    }
    return true;
}

void EntityPlayerMP::handleClientSettings(const C15PacketClientSettings& packetIn)
{
    translator = packetIn.getLang();
    chatVisibility = packetIn.getChatVisibility();
    chatColours = packetIn.isColorsEnabled();
    getDataWatcher().updateObject(10, static_cast<int8_t>(packetIn.getModelPartFlags()));
}

void EntityPlayerMP::loadResourcePack(const std::string& url, const std::string& hash)
{
    playerNetServerHandler->sendPacket(std::make_shared<S48PacketResourcePackSend>(url, hash));
}

void EntityPlayerMP::markPlayerActive()
{
    playerLastActiveTime = currentTimeMillis();
}

void EntityPlayerMP::removeEntity(Entity* p_152339_1_)
{
    if (dynamic_cast<EntityPlayer*>(p_152339_1_))
        playerNetServerHandler->sendPacket(std::make_shared<S13PacketDestroyEntities>(std::vector<int> { p_152339_1_->getEntityId() }));
    else
        destroyedItemsNetCache.push_back(p_152339_1_->getEntityId());
}

void EntityPlayerMP::updatePotionMetadata()
{
    if (isSpectator()) {
        resetPotionEffectMetadata();
        setInvisible(true);
    } else {
        EntityPlayer::updatePotionMetadata();
    }
    getServerForPlayer()->getEntityTracker()->func_180245_a(this);
}

void EntityPlayerMP::setSpectatingEntity(Entity* entityToSpectate)
{
    Entity* entity = getSpectatingEntity();
    spectatingEntity = entityToSpectate ? entityToSpectate : this;
    if (entity != spectatingEntity) {
        playerNetServerHandler->sendPacket(std::make_shared<S43PacketCamera>(spectatingEntity));
        setPositionAndUpdate(spectatingEntity->posX, spectatingEntity->posY, spectatingEntity->posZ);
    }
}

void EntityPlayerMP::attackTargetEntityWithCurrentItem(Entity* targetEntity)
{
    if (theItemInWorldManager->getGameType() == GameType::SPECTATOR)
        setSpectatingEntity(targetEntity);
    else
        EntityPlayer::attackTargetEntityWithCurrentItem(targetEntity);
}

} // namespace mc

#include "client/entity/EntityPlayerSP.h"

#include "block/Block.h"
#include "client/Minecraft.h"
#include "client/audio/Sound.h"
#include "client/audio/SoundHandler.h"
#include "client/gui/GuiIngame.h"
#include "client/gui/GuiScreen.h"
#include "client/multiplayer/PlayerControllerMP.h"
#include "client/network/NetHandlerPlayClient.h"
#include "core/Java.h"
#include "inventory/Container.h"
#include "item/ItemStack.h"
#include "network/play/ClientPlayPackets.h"
#include "util/DamageSource.h"
#include "world/World.h"

namespace mc {

EntityPlayerSP::EntityPlayerSP(Minecraft* mcIn, World* worldIn, NetHandlerPlayClient* netHandler)
    : AbstractClientPlayer(worldIn, netHandler->getGameProfile())
    , sendQueue(netHandler)
    , mc(mcIn)
{
}

void EntityPlayerSP::construct()
{
    AbstractClientPlayer::construct();
    dimension = 0;
}

void EntityPlayerSP::mountEntity(Entity* entityIn)
{
    AbstractClientPlayer::mountEntity(entityIn);
    // Java: riding an EntityMinecart starts MovingSoundMinecartRiding -- with minecarts.
}

void EntityPlayerSP::onUpdate()
{
    if (worldObj->isBlockLoaded(BlockPos(posX, 0.0, posZ))) {
        AbstractClientPlayer::onUpdate();
        if (isRiding()) {
            sendQueue->addToSendQueue(std::make_shared<C03PacketPlayer::C05PacketPlayerLook>(rotationYaw, rotationPitch, onGround));
            sendQueue->addToSendQueue(std::make_shared<C0CPacketInput>(moveStrafing, moveForward, movementInput->jump, movementInput->sneak));
        } else {
            onUpdateWalkingPlayer();
        }
    }
}

void EntityPlayerSP::onUpdateWalkingPlayer()
{
    const bool flag = isSprinting();
    if (flag != serverSprintState) {
        if (flag)
            sendQueue->addToSendQueue(std::make_shared<C0BPacketEntityAction>(this, C0BPacketEntityAction::Action::START_SPRINTING));
        else
            sendQueue->addToSendQueue(std::make_shared<C0BPacketEntityAction>(this, C0BPacketEntityAction::Action::STOP_SPRINTING));
        serverSprintState = flag;
    }
    const bool flag1 = isSneaking();
    if (flag1 != serverSneakState) {
        if (flag1)
            sendQueue->addToSendQueue(std::make_shared<C0BPacketEntityAction>(this, C0BPacketEntityAction::Action::START_SNEAKING));
        else
            sendQueue->addToSendQueue(std::make_shared<C0BPacketEntityAction>(this, C0BPacketEntityAction::Action::STOP_SNEAKING));
        serverSneakState = flag1;
    }
    if (isCurrentViewEntity()) {
        const double d0 = posX - lastReportedPosX;
        const double d1 = getEntityBoundingBox().minY - lastReportedPosY;
        const double d2 = posZ - lastReportedPosZ;
        const double d3 = static_cast<double>(rotationYaw - lastReportedYaw);
        const double d4 = static_cast<double>(rotationPitch - lastReportedPitch);
        bool flag2 = d0 * d0 + d1 * d1 + d2 * d2 > 9.0E-4 || positionUpdateTicks >= 20;
        const bool flag3 = d3 != 0.0 || d4 != 0.0;
        if (!ridingEntity) {
            if (flag2 && flag3) {
                sendQueue->addToSendQueue(
                    std::make_shared<C03PacketPlayer::C06PacketPlayerPosLook>(posX, getEntityBoundingBox().minY, posZ, rotationYaw, rotationPitch, onGround));
            } else if (flag2) {
                sendQueue->addToSendQueue(std::make_shared<C03PacketPlayer::C04PacketPlayerPosition>(posX, getEntityBoundingBox().minY, posZ, onGround));
            } else if (flag3) {
                sendQueue->addToSendQueue(std::make_shared<C03PacketPlayer::C05PacketPlayerLook>(rotationYaw, rotationPitch, onGround));
            } else {
                sendQueue->addToSendQueue(std::make_shared<C03PacketPlayer>(onGround));
            }
        } else {
            sendQueue->addToSendQueue(std::make_shared<C03PacketPlayer::C06PacketPlayerPosLook>(motionX, -999.0, motionZ, rotationYaw, rotationPitch, onGround));
            flag2 = false;
        }
        ++positionUpdateTicks;
        if (flag2) {
            lastReportedPosX = posX;
            lastReportedPosY = getEntityBoundingBox().minY;
            lastReportedPosZ = posZ;
            positionUpdateTicks = 0;
        }
        if (flag3) {
            lastReportedYaw = rotationYaw;
            lastReportedPitch = rotationPitch;
        }
    }
}

std::shared_ptr<EntityItem> EntityPlayerSP::dropOneItem(bool dropAll)
{
    const C07PacketPlayerDigging::Action action = dropAll ? C07PacketPlayerDigging::Action::DROP_ALL_ITEMS : C07PacketPlayerDigging::Action::DROP_ITEM;
    sendQueue->addToSendQueue(std::make_shared<C07PacketPlayerDigging>(action, BlockPos::ORIGIN, EnumFacing::DOWN));
    return nullptr;
}

void EntityPlayerSP::sendChatMessage(const std::string& message)
{
    sendQueue->addToSendQueue(std::make_shared<C01PacketChatMessage>(message));
}

void EntityPlayerSP::swingItem()
{
    AbstractClientPlayer::swingItem();
    sendQueue->addToSendQueue(std::make_shared<C0APacketAnimation>());
}

void EntityPlayerSP::respawnPlayer()
{
    sendQueue->addToSendQueue(std::make_shared<C16PacketClientStatus>(C16PacketClientStatus::EnumState::PERFORM_RESPAWN));
}

void EntityPlayerSP::damageEntity(const DamageSource& damageSrc, float damageAmount)
{
    if (!isEntityInvulnerable(damageSrc))
        setHealth(getHealth() - damageAmount);
}

void EntityPlayerSP::closeScreen()
{
    sendQueue->addToSendQueue(std::make_shared<C0DPacketCloseWindow>(openContainer->windowId));
    closeScreenAndDropStack();
}

void EntityPlayerSP::closeScreenAndDropStack()
{
    inventory.setItemStack(nullptr);
    AbstractClientPlayer::closeScreen();
    mc->displayGuiScreen(nullptr);
}

void EntityPlayerSP::setPlayerSPHealth(float health)
{
    if (hasValidHealth) {
        const float f = getHealth() - health;
        if (f <= 0.0f) {
            setHealth(health);
            if (f < 0.0f)
                hurtResistantTime = maxHurtResistantTime / 2;
        } else {
            lastDamage = f;
            setHealth(getHealth());
            hurtResistantTime = maxHurtResistantTime;
            damageEntity(DamageSource::generic, f);
            hurtTime = maxHurtTime = 10;
        }
    } else {
        setHealth(health);
        hasValidHealth = true;
    }
}

void EntityPlayerSP::addStat(const StatBase*, int)
{
    // Java: independent statistics go to the StatFileWriter -- statistics.
}

void EntityPlayerSP::sendPlayerAbilities()
{
    sendQueue->addToSendQueue(std::make_shared<C13PacketPlayerAbilities>(capabilities));
}

void EntityPlayerSP::sendHorseJump()
{
    sendQueue->addToSendQueue(
        std::make_shared<C0BPacketEntityAction>(this, C0BPacketEntityAction::Action::RIDING_JUMP, java::f2i(getHorseJumpPower() * 100.0f)));
}

void EntityPlayerSP::sendHorseInventory()
{
    sendQueue->addToSendQueue(std::make_shared<C0BPacketEntityAction>(this, C0BPacketEntityAction::Action::OPEN_INVENTORY));
}

void EntityPlayerSP::addChatComponentMessage(std::shared_ptr<IChatComponent> chatComponent)
{
    mc->ingameGUI->getChatGUI().printChatMessage(chatComponent);
}

bool EntityPlayerSP::pushOutOfBlocks(double x, double y, double z)
{
    if (noClip)
        return false;
    const BlockPos blockpos(x, y, z);
    const double d0 = x - static_cast<double>(blockpos.getX());
    const double d1 = z - static_cast<double>(blockpos.getZ());
    if (!isOpenBlockSpace(blockpos)) {
        int i = -1;
        double d2 = 9999.0;
        if (isOpenBlockSpace(blockpos.west()) && d0 < d2) {
            d2 = d0;
            i = 0;
        }
        if (isOpenBlockSpace(blockpos.east()) && 1.0 - d0 < d2) {
            d2 = 1.0 - d0;
            i = 1;
        }
        if (isOpenBlockSpace(blockpos.north()) && d1 < d2) {
            d2 = d1;
            i = 4;
        }
        if (isOpenBlockSpace(blockpos.south()) && 1.0 - d1 < d2) {
            d2 = 1.0 - d1;
            i = 5;
        }
        const float f = 0.1f;
        if (i == 0)
            motionX = static_cast<double>(-f);
        if (i == 1)
            motionX = static_cast<double>(f);
        if (i == 4)
            motionZ = static_cast<double>(-f);
        if (i == 5)
            motionZ = static_cast<double>(f);
    }
    return false;
}

bool EntityPlayerSP::isOpenBlockSpace(const BlockPos& pos)
{
    return !worldObj->getBlockState(pos)->getBlock()->isNormalCube() && !worldObj->getBlockState(pos.up())->getBlock()->isNormalCube();
}

void EntityPlayerSP::setSprinting(bool sprinting)
{
    AbstractClientPlayer::setSprinting(sprinting);
    sprintingTicksLeft = sprinting ? 600 : 0;
}

void EntityPlayerSP::setXPStats(float currentXP, int maxXP, int level)
{
    experience = currentXP;
    experienceTotal = maxXP;
    experienceLevel = level;
}

void EntityPlayerSP::addChatMessage(std::shared_ptr<IChatComponent> component)
{
    mc->ingameGUI->getChatGUI().printChatMessage(component);
}

void EntityPlayerSP::playSound(const std::string& name, float volume, float pitch)
{
    worldObj->playSound(posX, posY, posZ, name, volume, pitch, false);
}

bool EntityPlayerSP::isRidingHorse() const
{
    // Java: riding a saddled EntityHorse -- with horses.
    return false;
}

void EntityPlayerSP::openEditSign(TileEntitySign*)
{
    // Java: mc.displayGuiScreen(new GuiEditSign(signTile)) -- with signs.
}

void EntityPlayerSP::displayGUIBook(const std::shared_ptr<ItemStack>&)
{
    // Java: a writable book opens GuiScreenBook -- with books.
}

void EntityPlayerSP::displayGUIChest(IInventory*)
{
    // Java: GuiChest / GuiHopper / GuiFurnace / GuiBrewingStand / GuiBeacon / GuiDispenser by the GUI id --
    // the container screens come with their stage.
}

void EntityPlayerSP::onCriticalHit(Entity*)
{
    // Java: mc.effectRenderer.emitParticleAtEntity(entityHit, CRIT) -- with the effect renderer.
}

void EntityPlayerSP::onEnchantmentCritical(Entity*)
{
    // Java: mc.effectRenderer.emitParticleAtEntity(entityHit, CRIT_MAGIC) -- with the effect renderer.
}

bool EntityPlayerSP::isSneaking() const
{
    const bool flag = movementInput ? movementInput->sneak : false;
    return flag && !sleeping;
}

void EntityPlayerSP::updateEntityActionState()
{
    AbstractClientPlayer::updateEntityActionState();
    if (isCurrentViewEntity()) {
        moveStrafing = movementInput->moveStrafe;
        moveForward = movementInput->moveForward;
        isJumping = movementInput->jump;
        prevRenderArmYaw = renderArmYaw;
        prevRenderArmPitch = renderArmPitch;
        renderArmPitch = static_cast<float>(static_cast<double>(renderArmPitch) + static_cast<double>(rotationPitch - renderArmPitch) * 0.5);
        renderArmYaw = static_cast<float>(static_cast<double>(renderArmYaw) + static_cast<double>(rotationYaw - renderArmYaw) * 0.5);
    }
}

bool EntityPlayerSP::isCurrentViewEntity()
{
    return mc->getRenderViewEntity() == this;
}

void EntityPlayerSP::onLivingUpdate()
{
    if (sprintingTicksLeft > 0) {
        --sprintingTicksLeft;
        if (sprintingTicksLeft == 0)
            setSprinting(false);
    }
    if (sprintToggleTimer > 0)
        --sprintToggleTimer;
    prevTimeInPortal = timeInPortal;
    if (inPortal) {
        if (mc->currentScreen && !mc->currentScreen->doesGuiPauseGame())
            mc->displayGuiScreen(nullptr);
        if (timeInPortal == 0.0f)
            mc->getSoundHandler().playSound(PositionedSoundRecord::create(ResourceLocation("portal.trigger"), rand.nextFloat() * 0.4f + 0.8f));
        timeInPortal += 0.0125f;
        if (timeInPortal >= 1.0f)
            timeInPortal = 1.0f;
        inPortal = false;
    } else if (const PotionEffect* confusion = getActivePotionEffect(Potion::confusion); confusion && confusion->getDuration() > 60) {
        timeInPortal += 0.006666667f;
        if (timeInPortal > 1.0f)
            timeInPortal = 1.0f;
    } else {
        if (timeInPortal > 0.0f)
            timeInPortal -= 0.05f;
        if (timeInPortal < 0.0f)
            timeInPortal = 0.0f;
    }
    if (timeUntilPortal > 0)
        --timeUntilPortal;
    const bool flag = movementInput->jump;
    const bool flag1 = movementInput->sneak;
    const float f = 0.8f;
    const bool flag2 = movementInput->moveForward >= f;
    movementInput->updatePlayerMoveState();
    if (isUsingItem() && !isRiding()) {
        movementInput->moveStrafe *= 0.2f;
        movementInput->moveForward *= 0.2f;
        sprintToggleTimer = 0;
    }
    pushOutOfBlocks(posX - static_cast<double>(width) * 0.35, getEntityBoundingBox().minY + 0.5, posZ + static_cast<double>(width) * 0.35);
    pushOutOfBlocks(posX - static_cast<double>(width) * 0.35, getEntityBoundingBox().minY + 0.5, posZ - static_cast<double>(width) * 0.35);
    pushOutOfBlocks(posX + static_cast<double>(width) * 0.35, getEntityBoundingBox().minY + 0.5, posZ - static_cast<double>(width) * 0.35);
    pushOutOfBlocks(posX + static_cast<double>(width) * 0.35, getEntityBoundingBox().minY + 0.5, posZ + static_cast<double>(width) * 0.35);
    const bool flag3 = static_cast<float>(getFoodStats().getFoodLevel()) > 6.0f || capabilities.allowFlying;
    if (onGround && !flag1 && !flag2 && movementInput->moveForward >= f && !isSprinting() && flag3 && !isUsingItem() && !isPotionActive(Potion::blindness)) {
        if (sprintToggleTimer <= 0 && !mc->gameSettings->keyBindSprint.isKeyDown())
            sprintToggleTimer = 7;
        else
            setSprinting(true);
    }
    if (!isSprinting() && movementInput->moveForward >= f && flag3 && !isUsingItem() && !isPotionActive(Potion::blindness)
        && mc->gameSettings->keyBindSprint.isKeyDown())
        setSprinting(true);
    if (isSprinting() && (movementInput->moveForward < f || isCollidedHorizontally || !flag3))
        setSprinting(false);
    if (capabilities.allowFlying) {
        if (mc->playerController->isSpectatorMode()) {
            if (!capabilities.isFlying) {
                capabilities.isFlying = true;
                sendPlayerAbilities();
            }
        } else if (!flag && movementInput->jump) {
            if (flyToggleTimer == 0) {
                flyToggleTimer = 7;
            } else {
                capabilities.isFlying = !capabilities.isFlying;
                sendPlayerAbilities();
                flyToggleTimer = 0;
            }
        }
    }
    if (capabilities.isFlying && isCurrentViewEntity()) {
        if (movementInput->sneak)
            motionY -= static_cast<double>(capabilities.getFlySpeed() * 3.0f);
        if (movementInput->jump)
            motionY += static_cast<double>(capabilities.getFlySpeed() * 3.0f);
    }
    if (isRidingHorse()) {
        if (horseJumpPowerCounter < 0) {
            ++horseJumpPowerCounter;
            if (horseJumpPowerCounter == 0)
                horseJumpPower = 0.0f;
        }
        if (flag && !movementInput->jump) {
            horseJumpPowerCounter = -10;
            sendHorseJump();
        } else if (!flag && movementInput->jump) {
            horseJumpPowerCounter = 0;
            horseJumpPower = 0.0f;
        } else if (flag) {
            ++horseJumpPowerCounter;
            if (horseJumpPowerCounter < 10)
                horseJumpPower = static_cast<float>(horseJumpPowerCounter) * 0.1f;
            else
                horseJumpPower = 0.8f + 2.0f / static_cast<float>(horseJumpPowerCounter - 9) * 0.1f;
        }
    } else {
        horseJumpPower = 0.0f;
    }
    AbstractClientPlayer::onLivingUpdate();
    if (onGround && capabilities.isFlying && !mc->playerController->isSpectatorMode()) {
        capabilities.isFlying = false;
        sendPlayerAbilities();
    }
}

} // namespace mc

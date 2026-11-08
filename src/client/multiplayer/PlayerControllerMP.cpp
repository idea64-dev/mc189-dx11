#include "client/multiplayer/PlayerControllerMP.h"

#include "block/Block.h"
#include "block/material/Material.h"
#include "client/Minecraft.h"
#include "client/audio/Sound.h"
#include "client/audio/SoundHandler.h"
#include "client/multiplayer/WorldClient.h"
#include "client/network/NetHandlerPlayClient.h"
#include "core/Java.h"
#include "entity/Entity.h"
#include "entity/player/EntityPlayer.h"
#include "entity/player/EntityPlayerSP.h"
#include "entity/player/InventoryPlayer.h"
#include "inventory/Container.h"
#include "item/ItemBlock.h"
#include "item/ItemStack.h"
#include "item/ItemTool.h"
#include "network/NetworkManager.h"
#include "network/play/ClientPlayPackets.h"
#include "world/border/WorldBorder.h"

#include <cmath>

namespace mc {

PlayerControllerMP::PlayerControllerMP(Minecraft* mcIn, NetHandlerPlayClient* netHandler)
    : mc(mcIn)
    , netClientHandler(netHandler)
{
}

void PlayerControllerMP::clickBlockCreative(Minecraft* mcIn, PlayerControllerMP* playerController, const BlockPos& pos, EnumFacing facing)
{
    if (!mcIn->theWorld->extinguishFire(mcIn->thePlayer.get(), pos, facing))
        playerController->onPlayerDestroyBlock(pos, facing);
}

void PlayerControllerMP::setPlayerCapabilities(EntityPlayer* player)
{
    configurePlayerCapabilities(currentGameType, player->capabilities);
}

void PlayerControllerMP::setGameType(GameType type)
{
    currentGameType = type;
    configurePlayerCapabilities(currentGameType, mc->thePlayer->capabilities);
}

void PlayerControllerMP::flipPlayer(EntityPlayer* playerIn)
{
    playerIn->rotationYaw = -180.0f;
}

bool PlayerControllerMP::onPlayerDestroyBlock(const BlockPos& pos, EnumFacing)
{
    if (isAdventure(currentGameType)) {
        if (currentGameType == GameType::SPECTATOR)
            return false;

        if (!mc->thePlayer->isAllowEdit()) {
            Block* block = mc->theWorld->getBlockState(pos)->getBlock();
            const std::shared_ptr<ItemStack> itemstack = mc->thePlayer->inventory.getCurrentItem();

            if (itemstack == nullptr)
                return false;

            if (!itemstack->canDestroy(block))
                return false;
        }
    }

    const std::shared_ptr<ItemStack> held = mc->thePlayer->inventory.getCurrentItem();
    if (isCreative(currentGameType) && held != nullptr && dynamic_cast<ItemSword*>(held->getItem()) != nullptr)
        return false;

    World* world = mc->theWorld;
    const IBlockState* iblockstate = world->getBlockState(pos);
    Block* block1 = iblockstate->getBlock();

    if (&block1->getMaterial() == &Material::air)
        return false;

    world->playAuxSFX(2001, pos, Block::getStateId(iblockstate));
    const bool flag = world->setBlockToAir(pos);

    if (flag)
        block1->onBlockDestroyedByPlayer(world, pos, iblockstate);

    currentBlock = BlockPos(currentBlock.getX(), -1, currentBlock.getZ());

    if (!isCreative(currentGameType)) {
        const std::shared_ptr<ItemStack> itemstack1 = mc->thePlayer->inventory.getCurrentItem();

        if (itemstack1 != nullptr) {
            itemstack1->onBlockDestroyed(world, block1, pos, mc->thePlayer.get());

            if (itemstack1->stackSize == 0)
                mc->thePlayer->destroyCurrentEquippedItem();
        }
    }

    return flag;
}

bool PlayerControllerMP::clickBlock(const BlockPos& loc, EnumFacing face)
{
    if (isAdventure(currentGameType)) {
        if (currentGameType == GameType::SPECTATOR)
            return false;

        if (!mc->thePlayer->isAllowEdit()) {
            Block* block = mc->theWorld->getBlockState(loc)->getBlock();
            const std::shared_ptr<ItemStack> itemstack = mc->thePlayer->inventory.getCurrentItem();

            if (itemstack == nullptr)
                return false;

            if (!itemstack->canDestroy(block))
                return false;
        }
    }

    if (!mc->theWorld->getWorldBorder().contains(loc))
        return false;

    if (isCreative(currentGameType)) {
        netClientHandler->addToSendQueue(std::make_shared<C07PacketPlayerDigging>(C07PacketPlayerDigging::Action::START_DESTROY_BLOCK, loc, face));
        clickBlockCreative(mc, this, loc, face);
        blockHitDelay = 5;
    } else if (!isHittingBlock || !isHittingPosition(loc)) {
        if (isHittingBlock)
            netClientHandler->addToSendQueue(std::make_shared<C07PacketPlayerDigging>(C07PacketPlayerDigging::Action::ABORT_DESTROY_BLOCK, currentBlock, face));

        netClientHandler->addToSendQueue(std::make_shared<C07PacketPlayerDigging>(C07PacketPlayerDigging::Action::START_DESTROY_BLOCK, loc, face));
        Block* block1 = mc->theWorld->getBlockState(loc)->getBlock();
        const bool flag = &block1->getMaterial() != &Material::air;

        if (flag && curBlockDamageMP == 0.0f)
            block1->onBlockClicked(mc->theWorld, loc, mc->thePlayer.get());

        if (flag && block1->getPlayerRelativeBlockHardness(mc->thePlayer.get(), mc->thePlayer->worldObj, loc) >= 1.0f) {
            onPlayerDestroyBlock(loc, face);
        } else {
            isHittingBlock = true;
            currentBlock = loc;
            currentItemHittingBlock = mc->thePlayer->inventory.getCurrentItem();
            curBlockDamageMP = 0.0f;
            stepSoundTickCounter = 0.0f;
            mc->theWorld->sendBlockBreakProgress(mc->thePlayer->getEntityId(), currentBlock, java::f2i(curBlockDamageMP * 10.0f) - 1);
        }
    }

    return true;
}

void PlayerControllerMP::resetBlockRemoving()
{
    if (isHittingBlock) {
        netClientHandler->addToSendQueue(std::make_shared<C07PacketPlayerDigging>(C07PacketPlayerDigging::Action::ABORT_DESTROY_BLOCK, currentBlock, EnumFacing::DOWN));
        isHittingBlock = false;
        curBlockDamageMP = 0.0f;
        mc->theWorld->sendBlockBreakProgress(mc->thePlayer->getEntityId(), currentBlock, -1);
    }
}

bool PlayerControllerMP::onPlayerDamageBlock(const BlockPos& posBlock, EnumFacing directionFacing)
{
    syncCurrentPlayItem();

    if (blockHitDelay > 0) {
        --blockHitDelay;
        return true;
    }

    if (isCreative(currentGameType) && mc->theWorld->getWorldBorder().contains(posBlock)) {
        blockHitDelay = 5;
        netClientHandler->addToSendQueue(std::make_shared<C07PacketPlayerDigging>(C07PacketPlayerDigging::Action::START_DESTROY_BLOCK, posBlock, directionFacing));
        clickBlockCreative(mc, this, posBlock, directionFacing);
        return true;
    }

    if (isHittingPosition(posBlock)) {
        Block* block = mc->theWorld->getBlockState(posBlock)->getBlock();

        if (&block->getMaterial() == &Material::air) {
            isHittingBlock = false;
            return false;
        }

        curBlockDamageMP += block->getPlayerRelativeBlockHardness(mc->thePlayer.get(), mc->thePlayer->worldObj, posBlock);

        if (std::fmod(stepSoundTickCounter, 4.0f) == 0.0f) {
            mc->getSoundHandler().playSound(std::make_shared<PositionedSoundRecord>(ResourceLocation(block->stepSound->getStepSound()),
                (block->stepSound->getVolume() + 1.0f) / 8.0f, block->stepSound->getFrequency() * 0.5f, static_cast<float>(posBlock.getX()) + 0.5f,
                static_cast<float>(posBlock.getY()) + 0.5f, static_cast<float>(posBlock.getZ()) + 0.5f));
        }

        ++stepSoundTickCounter;

        if (curBlockDamageMP >= 1.0f) {
            isHittingBlock = false;
            netClientHandler->addToSendQueue(std::make_shared<C07PacketPlayerDigging>(C07PacketPlayerDigging::Action::STOP_DESTROY_BLOCK, posBlock, directionFacing));
            onPlayerDestroyBlock(posBlock, directionFacing);
            curBlockDamageMP = 0.0f;
            stepSoundTickCounter = 0.0f;
            blockHitDelay = 5;
        }

        mc->theWorld->sendBlockBreakProgress(mc->thePlayer->getEntityId(), currentBlock, java::f2i(curBlockDamageMP * 10.0f) - 1);
        return true;
    }

    return clickBlock(posBlock, directionFacing);
}

void PlayerControllerMP::updateController()
{
    syncCurrentPlayItem();

    if (netClientHandler->getNetworkManager()->isChannelOpen())
        netClientHandler->getNetworkManager()->processReceivedPackets();
    else
        netClientHandler->getNetworkManager()->checkDisconnected();
}

bool PlayerControllerMP::isHittingPosition(const BlockPos& pos) const
{
    const std::shared_ptr<ItemStack> itemstack = mc->thePlayer->inventory.getCurrentItem();
    bool flag = currentItemHittingBlock == nullptr && itemstack == nullptr;

    if (currentItemHittingBlock != nullptr && itemstack != nullptr) {
        flag = itemstack->getItem() == currentItemHittingBlock->getItem() && ItemStack::areItemStackTagsEqual(itemstack.get(), currentItemHittingBlock.get())
            && (itemstack->isItemStackDamageable() || itemstack->getMetadata() == currentItemHittingBlock->getMetadata());
    }

    return pos == currentBlock && flag;
}

void PlayerControllerMP::syncCurrentPlayItem()
{
    const int i = mc->thePlayer->inventory.currentItem;

    if (i != currentPlayerItem) {
        currentPlayerItem = i;
        netClientHandler->addToSendQueue(std::make_shared<C09PacketHeldItemChange>(currentPlayerItem));
    }
}

bool PlayerControllerMP::onPlayerRightClick(EntityPlayerSP* player, WorldClient* worldIn, const std::shared_ptr<ItemStack>& heldStack, const BlockPos& hitPos,
    EnumFacing side, const Vec3& hitVec)
{
    syncCurrentPlayItem();
    const float f = static_cast<float>(hitVec.xCoord - static_cast<double>(hitPos.getX()));
    const float f1 = static_cast<float>(hitVec.yCoord - static_cast<double>(hitPos.getY()));
    const float f2 = static_cast<float>(hitVec.zCoord - static_cast<double>(hitPos.getZ()));
    bool flag = false;

    if (!mc->theWorld->getWorldBorder().contains(hitPos))
        return false;

    if (currentGameType != GameType::SPECTATOR) {
        const IBlockState* iblockstate = worldIn->getBlockState(hitPos);

        if ((!player->isSneaking() || player->inventory.getCurrentItem() == nullptr)
            && iblockstate->getBlock()->onBlockActivated(worldIn, hitPos, iblockstate, player, side, f, f1, f2))
            flag = true;

        if (!flag && heldStack != nullptr) {
            if (auto* itemblock = dynamic_cast<ItemBlock*>(heldStack->getItem())) {
                if (!itemblock->canPlaceBlockOnSide(worldIn, hitPos, side, player, heldStack.get()))
                    return false;
            }
        }
    }

    netClientHandler->addToSendQueue(std::make_shared<C08PacketPlayerBlockPlacement>(hitPos, getIndex(side), player->inventory.getCurrentItem().get(), f, f1, f2));

    if (!flag && currentGameType != GameType::SPECTATOR) {
        if (heldStack == nullptr)
            return false;

        if (isCreative(currentGameType)) {
            const int i = heldStack->getMetadata();
            const int j = heldStack->stackSize;
            const bool flag1 = heldStack->onItemUse(player, worldIn, hitPos, side, f, f1, f2);
            heldStack->setItemDamage(i);
            heldStack->stackSize = j;
            return flag1;
        }

        return heldStack->onItemUse(player, worldIn, hitPos, side, f, f1, f2);
    }

    return true;
}

bool PlayerControllerMP::sendUseItem(EntityPlayer* playerIn, World* worldIn, const std::shared_ptr<ItemStack>& itemStackIn)
{
    if (currentGameType == GameType::SPECTATOR)
        return false;

    syncCurrentPlayItem();
    netClientHandler->addToSendQueue(std::make_shared<C08PacketPlayerBlockPlacement>(playerIn->inventory.getCurrentItem().get()));
    const int i = itemStackIn->stackSize;
    // Java: the same stack, or a new one the item hands back (e.g. a filled bucket).
    const std::shared_ptr<ItemStack> itemstack = ItemStack::useItemRightClick(itemStackIn, worldIn, playerIn);

    if (itemstack != itemStackIn || (itemstack != nullptr && itemstack->stackSize != i)) {
        auto& slot = playerIn->inventory.mainInventory[static_cast<size_t>(playerIn->inventory.currentItem)];
        slot = itemstack;

        if (itemstack->stackSize == 0)
            slot = nullptr;

        return true;
    }

    return false;
}

std::shared_ptr<EntityPlayerSP> PlayerControllerMP::func_178892_a(World* worldIn)
{
    return Entity::create<EntityPlayerSP>(mc, worldIn, netClientHandler);
}

void PlayerControllerMP::attackEntity(EntityPlayer* playerIn, Entity* targetEntity)
{
    syncCurrentPlayItem();
    netClientHandler->addToSendQueue(std::make_shared<C02PacketUseEntity>(targetEntity, C02PacketUseEntity::Action::ATTACK));

    if (currentGameType != GameType::SPECTATOR)
        playerIn->attackTargetEntityWithCurrentItem(targetEntity);
}

bool PlayerControllerMP::interactWithEntitySendPacket(EntityPlayer* playerIn, Entity* targetEntity)
{
    syncCurrentPlayItem();
    netClientHandler->addToSendQueue(std::make_shared<C02PacketUseEntity>(targetEntity, C02PacketUseEntity::Action::INTERACT));
    return currentGameType != GameType::SPECTATOR && playerIn->interactWith(targetEntity);
}

bool PlayerControllerMP::isPlayerRightClickingOnEntity(EntityPlayer* player, Entity* entityIn, const MovingObjectPosition& movingObject)
{
    syncCurrentPlayItem();
    const Vec3 vec3(movingObject.hitVec.xCoord - entityIn->posX, movingObject.hitVec.yCoord - entityIn->posY, movingObject.hitVec.zCoord - entityIn->posZ);
    netClientHandler->addToSendQueue(std::make_shared<C02PacketUseEntity>(entityIn, vec3));
    return currentGameType != GameType::SPECTATOR && entityIn->interactAt(player, vec3);
}

std::shared_ptr<ItemStack> PlayerControllerMP::windowClick(int windowId, int slotId, int mouseButtonClicked, int mode, EntityPlayer* playerIn)
{
    const int16_t short1 = playerIn->openContainer->getNextTransactionID(&playerIn->inventory);
    std::shared_ptr<ItemStack> itemstack = playerIn->openContainer->slotClick(slotId, mouseButtonClicked, mode, playerIn);
    netClientHandler->addToSendQueue(std::make_shared<C0EPacketClickWindow>(windowId, slotId, mouseButtonClicked, mode, itemstack.get(), short1));
    return itemstack;
}

void PlayerControllerMP::sendEnchantPacket(int windowID, int button)
{
    netClientHandler->addToSendQueue(std::make_shared<C11PacketEnchantItem>(windowID, button));
}

void PlayerControllerMP::sendSlotPacket(const ItemStack* itemStackIn, int slotId)
{
    if (isCreative(currentGameType))
        netClientHandler->addToSendQueue(std::make_shared<C10PacketCreativeInventoryAction>(slotId, itemStackIn));
}

void PlayerControllerMP::sendPacketDropItem(const ItemStack* itemStackIn)
{
    if (isCreative(currentGameType) && itemStackIn != nullptr)
        netClientHandler->addToSendQueue(std::make_shared<C10PacketCreativeInventoryAction>(-1, itemStackIn));
}

void PlayerControllerMP::onStoppedUsingItem(EntityPlayer* playerIn)
{
    syncCurrentPlayItem();
    netClientHandler->addToSendQueue(std::make_shared<C07PacketPlayerDigging>(C07PacketPlayerDigging::Action::RELEASE_USE_ITEM, BlockPos::ORIGIN, EnumFacing::DOWN));
    playerIn->stopUsingItem();
}

bool PlayerControllerMP::isRidingHorse() const
{
    // Java: ridingEntity instanceof EntityHorse -- the horses come with the mobs (stage 10).
    return mc->thePlayer->isRiding() && mc->thePlayer->ridingEntity->getClassName() == "EntityHorse";
}

} // namespace mc

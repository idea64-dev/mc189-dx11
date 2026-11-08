#include "server/management/ItemInWorldManager.h"

#include "block/AllBlocks.h"
#include "entity/player/EntityPlayerMP.h"
#include "entity/player/InventoryPlayer.h"
#include "item/ItemStack.h"
#include "item/ItemTool.h"
#include "network/NetHandlerPlayServer.h"
#include "network/play/ServerPlayPackets.h"
#include "server/MinecraftServer.h"
#include "server/management/ServerConfigurationManager.h"
#include "tileentity/TileEntity.h"
#include "world/World.h"
#include "world/WorldServer.h"

namespace mc {

void ItemInWorldManager::setGameType(GameType type)
{
    gameType = type;
    configurePlayerCapabilities(type, thisPlayerMP->capabilities);
    thisPlayerMP->sendPlayerAbilities();
    thisPlayerMP->mcServer->getConfigurationManager()->sendPacketToAllPlayers(
        std::make_shared<S38PacketPlayerListItem>(S38PacketPlayerListItem::Action::UPDATE_GAME_MODE, std::vector<EntityPlayerMP*> { thisPlayerMP }));
}

void ItemInWorldManager::initializeGameType(GameType type)
{
    if (gameType == GameType::NOT_SET)
        gameType = type;
    setGameType(gameType);
}

void ItemInWorldManager::updateBlockRemoving()
{
    ++curblockDamage;

    if (receivedFinishDiggingPacket) {
        const int i = curblockDamage - initialBlockDamage;
        Block* block = theWorld->getBlockState(field_180241_i)->getBlock();

        if (&block->getMaterial() == &Material::air) {
            receivedFinishDiggingPacket = false;
        } else {
            const float f = block->getPlayerRelativeBlockHardness(thisPlayerMP, thisPlayerMP->worldObj, field_180241_i) * static_cast<float>(i + 1);
            const int j = java::f2i(f * 10.0f);
            if (j != durabilityRemainingOnBlock) {
                theWorld->sendBlockBreakProgress(thisPlayerMP->getEntityId(), field_180241_i, j);
                durabilityRemainingOnBlock = j;
            }
            if (f >= 1.0f) {
                receivedFinishDiggingPacket = false;
                tryHarvestBlock(field_180241_i);
            }
        }
    } else if (isDestroyingBlock) {
        Block* block1 = theWorld->getBlockState(field_180240_f)->getBlock();

        if (&block1->getMaterial() == &Material::air) {
            theWorld->sendBlockBreakProgress(thisPlayerMP->getEntityId(), field_180240_f, -1);
            durabilityRemainingOnBlock = -1;
            isDestroyingBlock = false;
        } else {
            const int k = curblockDamage - initialDamage;
            // Java: the hardness is taken at field_180241_i (the last finished position), a vanilla quirk kept here.
            const float f1 = block1->getPlayerRelativeBlockHardness(thisPlayerMP, thisPlayerMP->worldObj, field_180241_i) * static_cast<float>(k + 1);
            const int l = java::f2i(f1 * 10.0f);
            if (l != durabilityRemainingOnBlock) {
                theWorld->sendBlockBreakProgress(thisPlayerMP->getEntityId(), field_180240_f, l);
                durabilityRemainingOnBlock = l;
            }
        }
    }
}

void ItemInWorldManager::onBlockClicked(const BlockPos& pos, EnumFacing side)
{
    if (isCreative()) {
        if (!theWorld->extinguishFire(nullptr, pos, side))
            tryHarvestBlock(pos);
        return;
    }

    Block* block = theWorld->getBlockState(pos)->getBlock();

    if (isAdventure(gameType)) {
        if (gameType == GameType::SPECTATOR)
            return;

        if (!thisPlayerMP->isAllowEdit()) {
            const std::shared_ptr<ItemStack> itemstack = thisPlayerMP->getCurrentEquippedItem();
            if (!itemstack)
                return;
            if (!itemstack->canDestroy(block))
                return;
        }
    }

    theWorld->extinguishFire(nullptr, pos, side);
    initialDamage = curblockDamage;
    float f = 1.0f;

    if (&block->getMaterial() != &Material::air) {
        block->onBlockClicked(theWorld, pos, thisPlayerMP);
        f = block->getPlayerRelativeBlockHardness(thisPlayerMP, thisPlayerMP->worldObj, pos);
    }

    if (&block->getMaterial() != &Material::air && f >= 1.0f) {
        tryHarvestBlock(pos);
    } else {
        isDestroyingBlock = true;
        field_180240_f = pos;
        const int i = java::f2i(f * 10.0f);
        theWorld->sendBlockBreakProgress(thisPlayerMP->getEntityId(), pos, i);
        durabilityRemainingOnBlock = i;
    }
}

void ItemInWorldManager::blockRemoving(const BlockPos& pos)
{
    if (pos != field_180240_f)
        return;

    const int i = curblockDamage - initialDamage;
    Block* block = theWorld->getBlockState(pos)->getBlock();

    if (&block->getMaterial() != &Material::air) {
        const float f = block->getPlayerRelativeBlockHardness(thisPlayerMP, thisPlayerMP->worldObj, pos) * static_cast<float>(i + 1);

        if (f >= 0.7f) {
            isDestroyingBlock = false;
            theWorld->sendBlockBreakProgress(thisPlayerMP->getEntityId(), pos, -1);
            tryHarvestBlock(pos);
        } else if (!receivedFinishDiggingPacket) {
            isDestroyingBlock = false;
            receivedFinishDiggingPacket = true;
            field_180241_i = pos;
            initialBlockDamage = initialDamage;
        }
    }
}

void ItemInWorldManager::cancelDestroyingBlock()
{
    isDestroyingBlock = false;
    theWorld->sendBlockBreakProgress(thisPlayerMP->getEntityId(), field_180240_f, -1);
}

bool ItemInWorldManager::removeBlock(const BlockPos& pos)
{
    const IBlockState* iblockstate = theWorld->getBlockState(pos);
    iblockstate->getBlock()->onBlockHarvested(theWorld, pos, iblockstate, thisPlayerMP);
    const bool flag = theWorld->setBlockToAir(pos);
    if (flag)
        iblockstate->getBlock()->onBlockDestroyedByPlayer(theWorld, pos, iblockstate);
    return flag;
}

bool ItemInWorldManager::tryHarvestBlock(const BlockPos& pos)
{
    if (mc::isCreative(gameType)) {
        const std::shared_ptr<ItemStack> held = thisPlayerMP->getHeldItem();
        if (held && dynamic_cast<ItemSword*>(held->getItem()))
            return false;
    }

    const IBlockState* iblockstate = theWorld->getBlockState(pos);
    TileEntity* tileentity = theWorld->getTileEntity(pos);
    // Keep the tile entity alive through the removal, as Java's reference does.
    const std::shared_ptr<TileEntity> tileEntityRef = tileentity ? theWorld->getTileEntityShared(pos) : nullptr;

    if (isAdventure(gameType)) {
        if (gameType == GameType::SPECTATOR)
            return false;

        if (!thisPlayerMP->isAllowEdit()) {
            const std::shared_ptr<ItemStack> itemstack = thisPlayerMP->getCurrentEquippedItem();
            if (!itemstack)
                return false;
            if (!itemstack->canDestroy(iblockstate->getBlock()))
                return false;
        }
    }

    theWorld->playAuxSFXAtEntity(thisPlayerMP, 2001, pos, Block::getStateId(iblockstate));
    const bool flag1 = removeBlock(pos);

    if (isCreative()) {
        thisPlayerMP->playerNetServerHandler->sendPacket(std::make_shared<S23PacketBlockChange>(theWorld, pos));
    } else {
        const std::shared_ptr<ItemStack> itemstack1 = thisPlayerMP->getCurrentEquippedItem();
        const bool flag = thisPlayerMP->canHarvestBlock(iblockstate->getBlock());

        if (itemstack1) {
            itemstack1->onBlockDestroyed(theWorld, iblockstate->getBlock(), pos, thisPlayerMP);
            if (itemstack1->stackSize == 0)
                thisPlayerMP->destroyCurrentEquippedItem();
        }

        if (flag1 && flag)
            iblockstate->getBlock()->harvestBlock(theWorld, thisPlayerMP, pos, iblockstate, tileEntityRef.get());
    }

    return flag1;
}

bool ItemInWorldManager::tryUseItem(EntityPlayer* player, World* worldIn, const std::shared_ptr<ItemStack>& stack)
{
    if (gameType == GameType::SPECTATOR)
        return false;

    const int i = stack->stackSize;
    const int j = stack->getMetadata();
    std::shared_ptr<ItemStack> itemstack = stack->useItemRightClick(worldIn, player);

    if (itemstack != stack || (itemstack && (itemstack->stackSize != i || itemstack->getMaxItemUseDuration() > 0 || itemstack->getMetadata() != j))) {
        player->inventory->mainInventory[static_cast<size_t>(player->inventory->currentItem)] = itemstack;

        if (isCreative()) {
            itemstack->stackSize = i;
            if (itemstack->isItemStackDamageable())
                itemstack->setItemDamage(j);
        }

        if (itemstack->stackSize == 0)
            player->inventory->mainInventory[static_cast<size_t>(player->inventory->currentItem)] = nullptr;

        if (!player->isUsingItem())
            static_cast<EntityPlayerMP*>(player)->sendContainerToPlayer(player->inventoryContainer.get());

        return true;
    }

    return false;
}

bool ItemInWorldManager::activateBlockOrUseItem(EntityPlayer* player, World* worldIn, const std::shared_ptr<ItemStack>& stack, const BlockPos& pos,
    EnumFacing side, float offsetX, float offsetY, float offsetZ)
{
    if (gameType == GameType::SPECTATOR) {
        // Java: a spectator opens the container it clicks (read-only) -- with the container GUIs (stage 11).
        return false;
    }

    if (!player->isSneaking() || player->getHeldItem() == nullptr) {
        const IBlockState* iblockstate = worldIn->getBlockState(pos);
        if (iblockstate->getBlock()->onBlockActivated(worldIn, pos, iblockstate, player, side, offsetX, offsetY, offsetZ))
            return true;
    }

    if (!stack)
        return false;

    if (isCreative()) {
        const int j = stack->getMetadata();
        const int i = stack->stackSize;
        const bool flag = stack->onItemUse(player, worldIn, pos, side, offsetX, offsetY, offsetZ);
        stack->setItemDamage(j);
        stack->stackSize = i;
        return flag;
    }

    return stack->onItemUse(player, worldIn, pos, side, offsetX, offsetY, offsetZ);
}

void ItemInWorldManager::setWorld(WorldServer* serverWorld)
{
    theWorld = serverWorld;
}

} // namespace mc

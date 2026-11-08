#include "tileentity/TileEntityHopper.h"

#include "block/BlockChest.h"
#include "block/BlockHopper.h"
#include "entity/Entity.h"
#include "init/Items.h"
#include "item/ItemStack.h"
#include "nbt/NBTTagCompound.h"
#include "tileentity/ISidedInventory.h"
#include "tileentity/TileEntities.h"
#include "util/MathHelper.h"
#include "world/World.h"

#include <algorithm>

namespace mc {

// --- SidedInventory ---

namespace SidedInventory {

bool isSided(IInventory* inventoryIn)
{
    return dynamic_cast<ISidedInventory*>(inventoryIn) != nullptr || dynamic_cast<TileEntityFurnace*>(inventoryIn) != nullptr;
}

std::vector<int> getSlotsForFace(IInventory* inventoryIn, EnumFacing side)
{
    if (auto* sided = dynamic_cast<ISidedInventory*>(inventoryIn))
        return sided->getSlotsForFace(side);
    // Java: TileEntityFurnace slotsBottom / slotsTop / slotsSides.
    return side == EnumFacing::DOWN ? std::vector<int> { 2, 1 } : (side == EnumFacing::UP ? std::vector<int> { 0 } : std::vector<int> { 1 });
}

bool canInsertItem(IInventory* inventoryIn, int index, const ItemStack& stack, std::optional<EnumFacing> direction)
{
    if (auto* sided = dynamic_cast<ISidedInventory*>(inventoryIn))
        return sided->canInsertItem(index, stack, direction);
    return inventoryIn->isItemValidForSlot(index, stack);
}

bool canExtractItem(IInventory* inventoryIn, int index, const ItemStack& stack, EnumFacing direction)
{
    if (auto* sided = dynamic_cast<ISidedInventory*>(inventoryIn))
        return sided->canExtractItem(index, stack, direction);
    if (direction == EnumFacing::DOWN && index == 1) {
        const Item* item = stack.getItem();
        if (item != Items::water_bucket && item != Items::bucket)
            return false;
    }
    return true;
}

} // namespace SidedInventory

// --- TileEntityHopper ---

void TileEntityHopper::readFromNBT(const NBTTagCompound& compound)
{
    TileEntityLockable::readFromNBT(compound);
    const auto nbttaglist = compound.getTagList("Items", 10);
    inventory.fill(nullptr);
    if (compound.hasKey("CustomName", 8))
        customName = compound.getString("CustomName");
    transferCooldown = compound.getInteger("TransferCooldown");
    for (int i = 0; i < nbttaglist->tagCount(); ++i) {
        const auto nbttagcompound = nbttaglist->getCompoundTagAt(i);
        const int j = nbttagcompound->getByte("Slot");
        if (j >= 0 && j < static_cast<int>(inventory.size())) {
            auto loaded = ItemStack::loadItemStackFromNBT(*nbttagcompound);
            inventory[static_cast<size_t>(j)] = loaded ? std::shared_ptr<ItemStack>(std::move(loaded)) : nullptr;
        }
    }
}

void TileEntityHopper::writeToNBT(NBTTagCompound& compound) const
{
    TileEntityLockable::writeToNBT(compound);
    auto nbttaglist = std::make_shared<NBTTagList>();
    for (size_t i = 0; i < inventory.size(); ++i) {
        if (inventory[i]) {
            auto nbttagcompound = std::make_shared<NBTTagCompound>();
            nbttagcompound->setByte("Slot", static_cast<int8_t>(i));
            inventory[i]->writeToNBT(*nbttagcompound);
            nbttaglist->appendTag(nbttagcompound);
        }
    }
    compound.setTag("Items", nbttaglist);
    compound.setInteger("TransferCooldown", transferCooldown);
    if (hasCustomName())
        compound.setString("CustomName", customName);
}

std::shared_ptr<ItemStack> TileEntityHopper::decrStackSize(int index, int count)
{
    std::shared_ptr<ItemStack>& slot = inventory.at(static_cast<size_t>(index));
    if (!slot)
        return nullptr;
    if (slot->stackSize <= count) {
        std::shared_ptr<ItemStack> itemstack1 = std::move(slot);
        slot = nullptr;
        return itemstack1;
    }
    auto itemstack = std::make_shared<ItemStack>(slot->splitStack(count));
    if (slot->stackSize == 0)
        slot = nullptr;
    return itemstack;
}

std::shared_ptr<ItemStack> TileEntityHopper::removeStackFromSlot(int index)
{
    std::shared_ptr<ItemStack>& slot = inventory.at(static_cast<size_t>(index));
    std::shared_ptr<ItemStack> itemstack = std::move(slot);
    slot = nullptr;
    return itemstack;
}

void TileEntityHopper::setInventorySlotContents(int index, std::shared_ptr<ItemStack> stack)
{
    if (stack && stack->stackSize > getInventoryStackLimit())
        stack->stackSize = getInventoryStackLimit();
    inventory.at(static_cast<size_t>(index)) = std::move(stack);
}

bool TileEntityHopper::isUseableByPlayer(EntityPlayer*)
{
    // Java: and the player within 8 blocks (stage 10).
    return worldObj->getTileEntity(pos) == this;
}

void TileEntityHopper::update()
{
    if (worldObj != nullptr && !worldObj->isRemote) {
        --transferCooldown;
        if (!isOnTransferCooldown()) {
            setTransferCooldown(0);
            updateHopper();
        }
    }
}

bool TileEntityHopper::updateHopper()
{
    if (worldObj != nullptr && !worldObj->isRemote) {
        if (!isOnTransferCooldown() && BlockHopper::isEnabled(getBlockMetadata())) {
            bool flag = false;
            if (!isEmpty())
                flag = transferItemsOut();
            if (!isFull())
                flag = captureDroppedItems(this) || flag;
            if (flag) {
                setTransferCooldown(8);
                markDirty();
                return true;
            }
        }
        return false;
    }
    return false;
}

bool TileEntityHopper::isEmpty() const
{
    return std::all_of(inventory.begin(), inventory.end(), [](const std::shared_ptr<ItemStack>& s) { return s == nullptr; });
}

bool TileEntityHopper::isFull() const
{
    for (const std::shared_ptr<ItemStack>& itemstack : inventory) {
        if (!itemstack || itemstack->stackSize != itemstack->getMaxStackSize())
            return false;
    }
    return true;
}

bool TileEntityHopper::transferItemsOut()
{
    const std::shared_ptr<IInventory> iinventory = getInventoryForHopperTransfer();
    if (!iinventory)
        return false;
    const EnumFacing enumfacing = getOpposite(BlockHopper::getFacing(getBlockMetadata()));
    if (isInventoryFull(iinventory.get(), enumfacing))
        return false;
    for (int i = 0; i < getSizeInventory(); ++i) {
        if (getStackInSlot(i)) {
            auto itemstack = std::make_shared<ItemStack>(getStackInSlot(i)->copy());
            const std::shared_ptr<ItemStack> itemstack1 = putStackInInventoryAllSlots(iinventory.get(), decrStackSize(i, 1), enumfacing);
            if (!itemstack1 || itemstack1->stackSize == 0) {
                iinventory->markDirty();
                return true;
            }
            setInventorySlotContents(i, itemstack);
        }
    }
    return false;
}

bool TileEntityHopper::isInventoryFull(IInventory* inventoryIn, EnumFacing side)
{
    if (SidedInventory::isSided(inventoryIn)) {
        for (const int k : SidedInventory::getSlotsForFace(inventoryIn, side)) {
            const std::shared_ptr<ItemStack> itemstack1 = inventoryIn->getStackInSlot(k);
            if (!itemstack1 || itemstack1->stackSize != itemstack1->getMaxStackSize())
                return false;
        }
    } else {
        const int i = inventoryIn->getSizeInventory();
        for (int j = 0; j < i; ++j) {
            const std::shared_ptr<ItemStack> itemstack = inventoryIn->getStackInSlot(j);
            if (!itemstack || itemstack->stackSize != itemstack->getMaxStackSize())
                return false;
        }
    }
    return true;
}

bool TileEntityHopper::isInventoryEmpty(IInventory* inventoryIn, EnumFacing side)
{
    if (SidedInventory::isSided(inventoryIn)) {
        for (const int i : SidedInventory::getSlotsForFace(inventoryIn, side)) {
            if (inventoryIn->getStackInSlot(i))
                return false;
        }
    } else {
        const int j = inventoryIn->getSizeInventory();
        for (int k = 0; k < j; ++k) {
            if (inventoryIn->getStackInSlot(k))
                return false;
        }
    }
    return true;
}

bool TileEntityHopper::captureDroppedItems(TileEntityHopper* hopper)
{
    const std::shared_ptr<IInventory> iinventory = getHopperInventory(hopper);
    if (iinventory) {
        const EnumFacing enumfacing = EnumFacing::DOWN;
        if (isInventoryEmpty(iinventory.get(), enumfacing))
            return false;
        if (SidedInventory::isSided(iinventory.get())) {
            for (const int i : SidedInventory::getSlotsForFace(iinventory.get(), enumfacing)) {
                if (pullItemFromSlot(hopper, iinventory.get(), i, enumfacing))
                    return true;
            }
        } else {
            const int j = iinventory->getSizeInventory();
            for (int k = 0; k < j; ++k) {
                if (pullItemFromSlot(hopper, iinventory.get(), k, enumfacing))
                    return true;
            }
        }
    } else {
        // Java: picks up the EntityItems in the block above (putDropInInventoryAllSlots) -- item entities come
        // with stage 10.
    }
    return false;
}

bool TileEntityHopper::pullItemFromSlot(TileEntityHopper* hopper, IInventory* inventoryIn, int index, EnumFacing direction)
{
    const std::shared_ptr<ItemStack> itemstack = inventoryIn->getStackInSlot(index);
    if (itemstack && canExtractItemFromSlot(inventoryIn, *itemstack, index, direction)) {
        auto itemstack1 = std::make_shared<ItemStack>(itemstack->copy());
        const std::shared_ptr<ItemStack> itemstack2 = putStackInInventoryAllSlots(hopper, inventoryIn->decrStackSize(index, 1), std::nullopt);
        if (!itemstack2 || itemstack2->stackSize == 0) {
            inventoryIn->markDirty();
            return true;
        }
        inventoryIn->setInventorySlotContents(index, itemstack1);
    }
    return false;
}

std::shared_ptr<ItemStack> TileEntityHopper::putStackInInventoryAllSlots(IInventory* inventoryIn, std::shared_ptr<ItemStack> stack, std::optional<EnumFacing> side)
{
    if (SidedInventory::isSided(inventoryIn) && side) {
        const std::vector<int> aint = SidedInventory::getSlotsForFace(inventoryIn, *side);
        for (size_t k = 0; k < aint.size() && stack && stack->stackSize > 0; ++k)
            stack = insertStack(inventoryIn, stack, aint[k], side);
    } else {
        const int i = inventoryIn->getSizeInventory();
        for (int j = 0; j < i && stack && stack->stackSize > 0; ++j)
            stack = insertStack(inventoryIn, stack, j, side);
    }
    if (stack && stack->stackSize == 0)
        stack = nullptr;
    return stack;
}

bool TileEntityHopper::canInsertItemInSlot(IInventory* inventoryIn, const ItemStack& stack, int index, std::optional<EnumFacing> side)
{
    return !inventoryIn->isItemValidForSlot(index, stack) ? false
                                                         : !SidedInventory::isSided(inventoryIn) || SidedInventory::canInsertItem(inventoryIn, index, stack, side);
}

bool TileEntityHopper::canExtractItemFromSlot(IInventory* inventoryIn, const ItemStack& stack, int index, EnumFacing side)
{
    return !SidedInventory::isSided(inventoryIn) || SidedInventory::canExtractItem(inventoryIn, index, stack, side);
}

std::shared_ptr<ItemStack> TileEntityHopper::insertStack(IInventory* inventoryIn, std::shared_ptr<ItemStack> stack, int index, std::optional<EnumFacing> side)
{
    const std::shared_ptr<ItemStack> itemstack = inventoryIn->getStackInSlot(index);
    if (canInsertItemInSlot(inventoryIn, *stack, index, side)) {
        bool flag = false;
        if (!itemstack) {
            inventoryIn->setInventorySlotContents(index, stack);
            stack = nullptr;
            flag = true;
        } else if (canCombine(*itemstack, *stack)) {
            const int i = stack->getMaxStackSize() - itemstack->stackSize;
            const int j = std::min(stack->stackSize, i);
            stack->stackSize -= j;
            itemstack->stackSize += j;
            flag = j > 0;
        }
        if (flag) {
            if (auto* tileentityhopper = dynamic_cast<TileEntityHopper*>(inventoryIn)) {
                if (tileentityhopper->mayTransfer())
                    tileentityhopper->setTransferCooldown(8);
                inventoryIn->markDirty();
            }
            inventoryIn->markDirty();
        }
    }
    return stack;
}

std::shared_ptr<IInventory> TileEntityHopper::getInventoryForHopperTransfer()
{
    const EnumFacing enumfacing = BlockHopper::getFacing(getBlockMetadata());
    return getInventoryAtPosition(getWorld(), pos.getX() + getFrontOffsetX(enumfacing), pos.getY() + getFrontOffsetY(enumfacing),
        pos.getZ() + getFrontOffsetZ(enumfacing));
}

std::shared_ptr<IInventory> TileEntityHopper::getHopperInventory(TileEntityHopper* hopper)
{
    return getInventoryAtPosition(hopper->getWorld(), hopper->getXPos(), hopper->getYPos() + 1.0, hopper->getZPos());
}

std::shared_ptr<IInventory> TileEntityHopper::getInventoryAtPosition(World* worldIn, double x, double y, double z)
{
    std::shared_ptr<IInventory> iinventory;
    const int i = MathHelper::floor_double(x);
    const int j = MathHelper::floor_double(y);
    const int k = MathHelper::floor_double(z);
    const BlockPos blockpos(i, j, k);
    Block* block = worldIn->getBlockState(blockpos)->getBlock();
    if (block->hasTileEntity()) {
        const std::shared_ptr<TileEntity> tileentity = worldIn->getTileEntityShared(blockpos);
        if (auto* inv = dynamic_cast<IInventory*>(tileentity.get())) {
            iinventory = std::shared_ptr<IInventory>(tileentity, inv);
            auto* blockchest = dynamic_cast<BlockChest*>(block);
            if (dynamic_cast<TileEntityChest*>(tileentity.get()) && blockchest)
                iinventory = blockchest->getLockableContainer(worldIn, blockpos);
        }
    }
    if (!iinventory) {
        // Java: EntitySelectors.selectInventories -- living entities that are inventories (minecarts, stage 10).
        const std::vector<Entity*> list = worldIn->getEntitiesInAABBexcluding(nullptr, AxisAlignedBB(x - 0.5, y - 0.5, z - 0.5, x + 0.5, y + 0.5, z + 0.5),
            [](Entity* e) { return dynamic_cast<IInventory*>(e) != nullptr && e->isEntityAlive(); });
        if (!list.empty()) {
            Entity* entity = list[static_cast<size_t>(worldIn->rand.nextInt(static_cast<int>(list.size())))];
            iinventory = std::shared_ptr<IInventory>(entity->shared_from_this(), dynamic_cast<IInventory*>(entity));
        }
    }
    return iinventory;
}

bool TileEntityHopper::canCombine(const ItemStack& stack1, const ItemStack& stack2)
{
    return stack1.getItem() != stack2.getItem()
        ? false
        : (stack1.getMetadata() != stack2.getMetadata() ? false : (stack1.stackSize > stack1.getMaxStackSize() ? false : ItemStack::areItemStackTagsEqual(&stack1, &stack2)));
}

} // namespace mc

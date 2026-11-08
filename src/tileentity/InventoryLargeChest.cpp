#include "tileentity/InventoryLargeChest.h"

#include "item/ItemStack.h"

namespace mc {

namespace {

// Java: ILockableContainer.isLocked / getLockCode / setLockCode on either kind of half.
bool isLockedHalf(const IInventory* inv)
{
    if (const auto* lockable = dynamic_cast<const TileEntityLockable*>(inv))
        return lockable->isLocked();
    if (const auto* large = dynamic_cast<const InventoryLargeChest*>(inv))
        return large->isLocked();
    return false;
}

std::string lockCodeOf(const IInventory* inv)
{
    if (const auto* lockable = dynamic_cast<const TileEntityLockable*>(inv))
        return lockable->getLockCode();
    if (const auto* large = dynamic_cast<const InventoryLargeChest*>(inv))
        return large->getLockCode();
    return {};
}

void setLockCodeOf(IInventory* inv, const std::string& code)
{
    if (auto* lockable = dynamic_cast<TileEntityLockable*>(inv))
        lockable->setLockCode(code);
    else if (auto* large = dynamic_cast<InventoryLargeChest*>(inv))
        large->setLockCode(code);
}

} // namespace

InventoryLargeChest::InventoryLargeChest(std::string nameIn, std::shared_ptr<IInventory> upperChestIn, std::shared_ptr<IInventory> lowerChestIn)
    : name(std::move(nameIn))
{
    if (!upperChestIn)
        upperChestIn = lowerChestIn;
    if (!lowerChestIn)
        lowerChestIn = upperChestIn;
    upperChest = std::move(upperChestIn);
    lowerChest = std::move(lowerChestIn);
    if (isLockedHalf(upperChest.get()))
        setLockCodeOf(lowerChest.get(), lockCodeOf(upperChest.get()));
    else if (isLockedHalf(lowerChest.get()))
        setLockCodeOf(upperChest.get(), lockCodeOf(lowerChest.get()));
}

int InventoryLargeChest::getSizeInventory() const
{
    return upperChest->getSizeInventory() + lowerChest->getSizeInventory();
}

std::string InventoryLargeChest::getName() const
{
    return upperChest->hasCustomName() ? upperChest->getName() : (lowerChest->hasCustomName() ? lowerChest->getName() : name);
}

bool InventoryLargeChest::hasCustomName() const
{
    return upperChest->hasCustomName() || lowerChest->hasCustomName();
}

std::shared_ptr<ItemStack> InventoryLargeChest::getStackInSlot(int index) const
{
    return index >= upperChest->getSizeInventory() ? lowerChest->getStackInSlot(index - upperChest->getSizeInventory()) : upperChest->getStackInSlot(index);
}

std::shared_ptr<ItemStack> InventoryLargeChest::decrStackSize(int index, int count)
{
    return index >= upperChest->getSizeInventory() ? lowerChest->decrStackSize(index - upperChest->getSizeInventory(), count)
                                                   : upperChest->decrStackSize(index, count);
}

std::shared_ptr<ItemStack> InventoryLargeChest::removeStackFromSlot(int index)
{
    return index >= upperChest->getSizeInventory() ? lowerChest->removeStackFromSlot(index - upperChest->getSizeInventory())
                                                   : upperChest->removeStackFromSlot(index);
}

void InventoryLargeChest::setInventorySlotContents(int index, std::shared_ptr<ItemStack> stack)
{
    if (index >= upperChest->getSizeInventory())
        lowerChest->setInventorySlotContents(index - upperChest->getSizeInventory(), std::move(stack));
    else
        upperChest->setInventorySlotContents(index, std::move(stack));
}

int InventoryLargeChest::getInventoryStackLimit() const
{
    return upperChest->getInventoryStackLimit();
}

void InventoryLargeChest::markDirty()
{
    upperChest->markDirty();
    lowerChest->markDirty();
}

bool InventoryLargeChest::isUseableByPlayer(EntityPlayer* player)
{
    return upperChest->isUseableByPlayer(player) && lowerChest->isUseableByPlayer(player);
}

void InventoryLargeChest::openInventory(EntityPlayer* player)
{
    upperChest->openInventory(player);
    lowerChest->openInventory(player);
}

void InventoryLargeChest::closeInventory(EntityPlayer* player)
{
    upperChest->closeInventory(player);
    lowerChest->closeInventory(player);
}

bool InventoryLargeChest::isLocked() const
{
    return isLockedHalf(upperChest.get()) || isLockedHalf(lowerChest.get());
}

void InventoryLargeChest::setLockCode(const std::string& code)
{
    setLockCodeOf(upperChest.get(), code);
    setLockCodeOf(lowerChest.get(), code);
}

std::string InventoryLargeChest::getLockCode() const
{
    return lockCodeOf(upperChest.get());
}

void InventoryLargeChest::clear()
{
    upperChest->clear();
    lowerChest->clear();
}

} // namespace mc

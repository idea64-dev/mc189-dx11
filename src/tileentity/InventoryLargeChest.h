#pragma once

// Original: net/minecraft/inventory/InventoryLargeChest.java -- the two halves of a double chest as one
// inventory (Java: an ILockableContainer). Kept with the tile entities until inventory/ has it.

#include "tileentity/TileEntity.h"

#include <memory>
#include <string>

namespace mc {

class InventoryLargeChest : public IInventory {
public:
    // Java: a null half is replaced by the other one.
    InventoryLargeChest(std::string nameIn, std::shared_ptr<IInventory> upperChestIn, std::shared_ptr<IInventory> lowerChestIn);

    int getSizeInventory() const override;
    bool isPartOfLargeChest(const IInventory* inventoryIn) const { return upperChest.get() == inventoryIn || lowerChest.get() == inventoryIn; }
    std::string getName() const override;
    bool hasCustomName() const override;
    std::shared_ptr<ItemStack> getStackInSlot(int index) const override;
    std::shared_ptr<ItemStack> decrStackSize(int index, int count) override;
    std::shared_ptr<ItemStack> removeStackFromSlot(int index) override;
    void setInventorySlotContents(int index, std::shared_ptr<ItemStack> stack) override;
    int getInventoryStackLimit() const override;
    void markDirty() override;
    bool isUseableByPlayer(EntityPlayer* player) override;
    void openInventory(EntityPlayer* player) override;
    void closeInventory(EntityPlayer* player) override;
    bool isItemValidForSlot(int, const ItemStack&) const override { return true; }
    int getField(int) const override { return 0; }
    void setField(int, int) override { }
    int getFieldCount() const override { return 0; }
    bool isLocked() const;
    void setLockCode(const std::string& code);
    std::string getLockCode() const;
    void clear() override;

private:
    std::string name;
    std::shared_ptr<IInventory> upperChest;
    std::shared_ptr<IInventory> lowerChest;
};

} // namespace mc

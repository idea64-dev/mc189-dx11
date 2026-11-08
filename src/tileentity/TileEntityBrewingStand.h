#pragma once

// Original: net/minecraft/tileentity/TileEntityBrewingStand.java -- three bottles and an ingredient, brewed in
// 400 ticks.

#include "tileentity/ISidedInventory.h"
#include "tileentity/TileEntity.h"

#include <array>
#include <memory>
#include <optional>
#include <string>

namespace mc {

class Item;

class TileEntityBrewingStand : public TileEntityLockable, public ISidedInventory {
public:
    std::string getName() const override { return hasCustomName() ? customName : "container.brewing"; }
    bool hasCustomName() const override { return !customName.empty(); }
    void setName(std::string name) { customName = std::move(name); }
    int getSizeInventory() const override { return static_cast<int>(brewingItemStacks.size()); }
    bool isTickable() const override { return true; }
    void update() override;
    void readFromNBT(const NBTTagCompound& compound) override;
    void writeToNBT(NBTTagCompound& compound) const override;
    std::shared_ptr<ItemStack> getStackInSlot(int index) const override;
    std::shared_ptr<ItemStack> decrStackSize(int index, int count) override;
    std::shared_ptr<ItemStack> removeStackFromSlot(int index) override;
    void setInventorySlotContents(int index, std::shared_ptr<ItemStack> stack) override;
    int getInventoryStackLimit() const override { return 64; }
    bool isUseableByPlayer(EntityPlayer* player) override;
    void openInventory(EntityPlayer*) override { }
    void closeInventory(EntityPlayer*) override { }
    bool isItemValidForSlot(int index, const ItemStack& stack) const override;
    std::array<bool, 3> func_174902_m() const;
    std::vector<int> getSlotsForFace(EnumFacing side) override;
    bool canInsertItem(int index, const ItemStack& itemStackIn, std::optional<EnumFacing> direction) override;
    bool canExtractItem(int, const ItemStack&, EnumFacing) override { return true; }
    int getField(int id) const override { return id == 0 ? brewTime : 0; }
    void setField(int id, int value) override
    {
        if (id == 0)
            brewTime = value;
    }
    int getFieldCount() const override { return 1; }
    void clear() override { brewingItemStacks.fill(nullptr); }
    std::string_view getMappingName() const override { return "Cauldron"; }

private:
    bool canBrew();
    void brewPotions();
    int getPotionResult(int meta, const ItemStack* stack);

    std::array<std::shared_ptr<ItemStack>, 4> brewingItemStacks {};
    int brewTime = 0;
    // Java: null until the first update.
    std::optional<std::array<bool, 3>> filledSlots;
    Item* ingredientID = nullptr;
    std::string customName;
};

} // namespace mc

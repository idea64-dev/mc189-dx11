#pragma once

// Original: net/minecraft/inventory/InventoryBasic.java, IInvBasic.java and InventoryEnderChest.java

#include "tileentity/TileEntity.h"

#include <memory>
#include <string>
#include <vector>

namespace mc {

class EntityPlayer;
class IChatComponent;
class InventoryBasic;
class ItemStack;
class NBTTagList;
class TileEntityEnderChest;

class IInvBasic {
public:
    virtual ~IInvBasic() = default;
    virtual void onInventoryChanged(InventoryBasic* p_76316_1_) = 0;
};

class InventoryBasic : public IInventory {
public:
    InventoryBasic(std::string title, bool customName, int slotCount);
    InventoryBasic(IChatComponent& title, int slotCount);

    void addInventoryChangeListener(IInvBasic* listener) { changeListeners.push_back(listener); }
    void removeInventoryChangeListener(IInvBasic* listener);
    std::shared_ptr<ItemStack> getStackInSlot(int index) const override;
    std::shared_ptr<ItemStack> decrStackSize(int index, int count) override;
    // Java: func_174894_a -- adds a copy of the stack where it fits; the rest, or null when all fit.
    std::shared_ptr<ItemStack> func_174894_a(const ItemStack& stack);
    std::shared_ptr<ItemStack> removeStackFromSlot(int index) override;
    void setInventorySlotContents(int index, std::shared_ptr<ItemStack> stack) override;
    int getSizeInventory() const override { return slotsCount; }
    std::string getName() const override { return inventoryTitle; }
    bool hasCustomName() const override { return customName; }
    void setCustomName(std::string inventoryTitleIn);
    std::shared_ptr<IChatComponent> getDisplayName() const;
    int getInventoryStackLimit() const override { return 64; }
    void markDirty() override;
    bool isUseableByPlayer(EntityPlayer*) override { return true; }
    void openInventory(EntityPlayer*) override { }
    void closeInventory(EntityPlayer*) override { }
    bool isItemValidForSlot(int, const ItemStack&) const override { return true; }
    int getField(int) const override { return 0; }
    void setField(int, int) override { }
    int getFieldCount() const override { return 0; }
    void clear() override;

private:
    std::string inventoryTitle;
    int slotsCount;
    std::vector<std::shared_ptr<ItemStack>> inventoryContents;
    std::vector<IInvBasic*> changeListeners;
    bool customName;
};

class InventoryEnderChest : public InventoryBasic {
public:
    InventoryEnderChest() : InventoryBasic("container.enderchest", false, 27) { }

    void setChestTileEntity(TileEntityEnderChest* chestTileEntity) { associatedChest = chestTileEntity; }
    void loadInventoryFromNBT(const NBTTagList& p_70486_1_);
    std::shared_ptr<NBTTagList> saveInventoryToNBT() const;
    bool isUseableByPlayer(EntityPlayer* player) override;
    void openInventory(EntityPlayer* player) override;
    void closeInventory(EntityPlayer* player) override;

private:
    TileEntityEnderChest* associatedChest = nullptr;
};

} // namespace mc

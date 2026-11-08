#pragma once

// Original: net/minecraft/inventory/InventoryCrafting.java, InventoryCraftResult.java and SlotCrafting.java
// Recipes (CraftingManager) are not ported yet: the matrix never has a result and taking one leaves the
// matrix as it is (see SlotCrafting::onPickupFromSlot).

#include "inventory/Slot.h"
#include "tileentity/TileEntity.h"

#include <array>
#include <memory>
#include <string>
#include <vector>

namespace mc {

class Container;
class EntityPlayer;
class IChatComponent;
class ItemStack;

class InventoryCrafting : public IInventory {
public:
    InventoryCrafting(Container* eventHandlerIn, int width, int height);

    int getSizeInventory() const override { return static_cast<int>(stackList.size()); }
    std::shared_ptr<ItemStack> getStackInSlot(int index) const override;
    std::shared_ptr<ItemStack> getStackInRowAndColumn(int row, int column) const;
    std::string getName() const override { return "container.crafting"; }
    bool hasCustomName() const override { return false; }
    std::shared_ptr<IChatComponent> getDisplayName() const;
    std::shared_ptr<ItemStack> removeStackFromSlot(int index) override;
    std::shared_ptr<ItemStack> decrStackSize(int index, int count) override;
    void setInventorySlotContents(int index, std::shared_ptr<ItemStack> stack) override;
    int getInventoryStackLimit() const override { return 64; }
    void markDirty() override { }
    bool isUseableByPlayer(EntityPlayer*) override { return true; }
    void openInventory(EntityPlayer*) override { }
    void closeInventory(EntityPlayer*) override { }
    bool isItemValidForSlot(int, const ItemStack&) const override { return true; }
    int getField(int) const override { return 0; }
    void setField(int, int) override { }
    int getFieldCount() const override { return 0; }
    void clear() override;
    int getHeight() const { return inventoryHeight; }
    int getWidth() const { return inventoryWidth; }

private:
    std::vector<std::shared_ptr<ItemStack>> stackList;
    const int inventoryWidth;
    const int inventoryHeight;
    Container* const eventHandler;
};

class InventoryCraftResult : public IInventory {
public:
    int getSizeInventory() const override { return 1; }
    std::shared_ptr<ItemStack> getStackInSlot(int) const override { return stackResult[0]; }
    std::string getName() const override { return "Result"; }
    bool hasCustomName() const override { return false; }
    std::shared_ptr<IChatComponent> getDisplayName() const;
    std::shared_ptr<ItemStack> decrStackSize(int, int) override;
    std::shared_ptr<ItemStack> removeStackFromSlot(int) override;
    void setInventorySlotContents(int, std::shared_ptr<ItemStack> stack) override { stackResult[0] = std::move(stack); }
    int getInventoryStackLimit() const override { return 64; }
    void markDirty() override { }
    bool isUseableByPlayer(EntityPlayer*) override { return true; }
    void openInventory(EntityPlayer*) override { }
    void closeInventory(EntityPlayer*) override { }
    bool isItemValidForSlot(int, const ItemStack&) const override { return true; }
    int getField(int) const override { return 0; }
    void setField(int, int) override { }
    int getFieldCount() const override { return 0; }
    void clear() override { stackResult = {}; }

private:
    std::array<std::shared_ptr<ItemStack>, 1> stackResult {};
};

class SlotCrafting : public Slot {
public:
    SlotCrafting(EntityPlayer* player, InventoryCrafting* craftingInventory, IInventory* p_i45790_3_, int slotIndex, int xPosition, int yPosition)
        : Slot(p_i45790_3_, slotIndex, xPosition, yPosition)
        , craftMatrix(craftingInventory)
        , thePlayer(player)
    {
    }

    bool isItemValid(const ItemStack*) override { return false; }
    std::shared_ptr<ItemStack> decrStackSize(int amount) override;
    void onPickupFromSlot(EntityPlayer* playerIn, const std::shared_ptr<ItemStack>& stack) override;

protected:
    void onCrafting(ItemStack& stack, int amount) override;
    void onCrafting(ItemStack& stack) override;

private:
    InventoryCrafting* const craftMatrix;
    EntityPlayer* const thePlayer;
    int amountCrafted = 0;
};

} // namespace mc

#pragma once

// Original: net/minecraft/inventory/Container.java and ICrafting.java
// Containers own their slots. Java's dragSlots is a HashSet of slots by identity (an unspecified order); here
// it keeps the order the slots were dragged over.

#include "inventory/Slot.h"

#include <cstdint>
#include <memory>
#include <vector>

namespace mc {

class EntityPlayer;
class IInventory;
class InventoryPlayer;
class ItemStack;
class TileEntity;

class Container;

class ICrafting {
public:
    virtual ~ICrafting() = default;
    virtual void updateCraftingInventory(Container* containerToSend, const std::vector<std::shared_ptr<ItemStack>>& itemsList) = 0;
    virtual void sendSlotContents(Container* containerToSend, int slotInd, const ItemStack* stack) = 0;
    virtual void sendProgressBarUpdate(Container* containerIn, int varToUpdate, int newValue) = 0;
    virtual void sendAllWindowProperties(Container* p_175173_1_, IInventory* p_175173_2_) = 0;
};

class Container {
public:
    virtual ~Container() = default;
    Container() = default;
    Container(const Container&) = delete;
    Container& operator=(const Container&) = delete;

    // Throws std::invalid_argument when the listener is already listening.
    void onCraftGuiOpened(ICrafting* listener);
    void removeCraftingFromCrafters(ICrafting* listeners);
    std::vector<std::shared_ptr<ItemStack>> getInventory() const;
    virtual void detectAndSendChanges();
    virtual bool enchantItem(EntityPlayer*, int) { return false; }
    Slot* getSlotFromInventory(const IInventory* inv, int slotIn) const;
    Slot* getSlot(int slotId) const { return inventorySlots.at(static_cast<size_t>(slotId)).get(); }
    virtual std::shared_ptr<ItemStack> transferStackInSlot(EntityPlayer* playerIn, int index);
    std::shared_ptr<ItemStack> slotClick(int slotId, int clickedButton, int mode, EntityPlayer* playerIn);
    virtual bool canMergeSlot(const ItemStack*, Slot*) { return true; }
    virtual void onContainerClosed(EntityPlayer* playerIn);
    virtual void onCraftMatrixChanged(IInventory* inventoryIn) { (void)inventoryIn; detectAndSendChanges(); }
    void putStackInSlot(int slotID, std::shared_ptr<ItemStack> stack) { getSlot(slotID)->putStack(std::move(stack)); }
    void putStacksInSlots(const std::vector<std::shared_ptr<ItemStack>>& p_75131_1_);
    virtual void updateProgressBar(int, int) { }
    int16_t getNextTransactionID(InventoryPlayer*) { return ++transactionID; }
    bool getCanCraft(EntityPlayer* p_75129_1_) const;
    void setCanCraft(EntityPlayer* p_75128_1_, bool p_75128_2_);
    virtual bool canInteractWith(EntityPlayer* playerIn) = 0;
    virtual bool canDragIntoSlot(Slot*) { return true; }

    static int extractDragMode(int p_94529_0_) { return p_94529_0_ >> 2 & 3; }
    static int getDragEvent(int p_94532_0_) { return p_94532_0_ & 3; }
    static int func_94534_d(int p_94534_0_, int p_94534_1_) { return p_94534_0_ & 3 | (p_94534_1_ & 3) << 2; }
    static bool isValidDragMode(int dragModeIn, EntityPlayer* player);
    static bool canAddItemToSlot(Slot* slotIn, const ItemStack* stack, bool stackSizeMatters);
    static void computeStackSize(size_t dragSlotCount, int p_94525_1_, ItemStack& p_94525_2_, int p_94525_3_);
    static int calcRedstone(TileEntity* te);
    static int calcRedstoneFromInventory(IInventory* inv);

    std::vector<std::shared_ptr<ItemStack>> inventoryItemStacks;
    std::vector<std::unique_ptr<Slot>> inventorySlots;
    int windowId = 0;

protected:
    Slot* addSlotToContainer(std::unique_ptr<Slot> slotIn);
    virtual void retrySlotClick(int slotId, int clickedButton, bool mode, EntityPlayer* playerIn);
    bool mergeItemStack(ItemStack& stack, int startIndex, int endIndex, bool reverseDirection);
    void resetDrag();

    std::vector<ICrafting*> crafters;

private:
    int16_t transactionID = 0;
    int dragMode = -1;
    int dragEvent = 0;
    std::vector<Slot*> dragSlots;
    std::vector<EntityPlayer*> playerList;
};

} // namespace mc

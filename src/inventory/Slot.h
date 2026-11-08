#pragma once

// Original: net/minecraft/inventory/Slot.java
// Java passes null stacks to isItemValid / getItemStackLimit; they take pointers here.

#include <memory>
#include <optional>
#include <string>

namespace mc {

class EntityPlayer;
class IInventory;
class ItemStack;

class Slot {
public:
    Slot(IInventory* inventoryIn, int index, int xPosition, int yPosition)
        : inventory(inventoryIn)
        , xDisplayPosition(xPosition)
        , yDisplayPosition(yPosition)
        , slotIndex(index)
    {
    }
    virtual ~Slot() = default;

    void onSlotChange(ItemStack* p_75220_1_, const ItemStack* p_75220_2_);
    virtual void onPickupFromSlot(EntityPlayer* playerIn, const std::shared_ptr<ItemStack>& stack);
    virtual bool isItemValid(const ItemStack*) { return true; }
    std::shared_ptr<ItemStack> getStack() const;
    bool getHasStack() const { return getStack() != nullptr; }
    void putStack(std::shared_ptr<ItemStack> stack);
    virtual void onSlotChanged();
    virtual int getSlotStackLimit();
    virtual int getItemStackLimit(const ItemStack*) { return getSlotStackLimit(); }
    // Java: null for no background icon.
    virtual std::optional<std::string> getSlotTexture() { return std::nullopt; }
    virtual std::shared_ptr<ItemStack> decrStackSize(int amount);
    bool isHere(const IInventory* inv, int slotIn) const { return inv == inventory && slotIn == slotIndex; }
    virtual bool canTakeStack(EntityPlayer*) { return true; }
    virtual bool canBeHovered() { return true; }
    int getSlotIndex() const { return slotIndex; }

    IInventory* const inventory;
    int slotNumber = 0;
    int xDisplayPosition;
    int yDisplayPosition;

protected:
    virtual void onCrafting(ItemStack&, int) { }
    virtual void onCrafting(ItemStack&) { }

private:
    const int slotIndex;
};

} // namespace mc

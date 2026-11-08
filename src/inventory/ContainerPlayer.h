#pragma once

// Original: net/minecraft/inventory/ContainerPlayer.java

#include "inventory/Container.h"
#include "inventory/InventoryCrafting.h"

namespace mc {

class EntityPlayer;
class InventoryPlayer;

class ContainerPlayer : public Container {
public:
    ContainerPlayer(InventoryPlayer& playerInventory, bool localWorld, EntityPlayer* player);

    void onCraftMatrixChanged(IInventory* inventoryIn) override;
    void onContainerClosed(EntityPlayer* playerIn) override;
    bool canInteractWith(EntityPlayer*) override { return true; }
    std::shared_ptr<ItemStack> transferStackInSlot(EntityPlayer* playerIn, int index) override;
    bool canMergeSlot(const ItemStack* stack, Slot* slotIn) override;

    InventoryCrafting craftMatrix { this, 2, 2 };
    InventoryCraftResult craftResult;
    bool isLocalWorld;

private:
    EntityPlayer* const thePlayer;
};

} // namespace mc

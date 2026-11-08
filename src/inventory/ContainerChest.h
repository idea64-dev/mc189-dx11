#pragma once

// Original: net/minecraft/inventory/ContainerChest.java

#include "inventory/Container.h"

namespace mc {

class ContainerChest : public Container {
public:
    ContainerChest(IInventory* playerInventory, IInventory* chestInventory, EntityPlayer* player);

    bool canInteractWith(EntityPlayer* playerIn) override;
    std::shared_ptr<ItemStack> transferStackInSlot(EntityPlayer* playerIn, int index) override;
    void onContainerClosed(EntityPlayer* playerIn) override;
    IInventory* getLowerChestInventory() const { return lowerChestInventory; }

private:
    IInventory* const lowerChestInventory;
    const int numRows;
};

} // namespace mc

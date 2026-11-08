#include "inventory/ContainerChest.h"

#include "inventory/CopyItemStack.h"
#include "item/ItemStack.h"
#include "tileentity/TileEntity.h"

namespace mc {

ContainerChest::ContainerChest(IInventory* playerInventory, IInventory* chestInventory, EntityPlayer* player)
    : lowerChestInventory(chestInventory)
    , numRows(chestInventory->getSizeInventory() / 9)
{
    chestInventory->openInventory(player);
    const int i = (numRows - 4) * 18;
    for (int j = 0; j < numRows; ++j) {
        for (int k = 0; k < 9; ++k)
            addSlotToContainer(std::make_unique<Slot>(chestInventory, k + j * 9, 8 + k * 18, 18 + j * 18));
    }
    for (int l = 0; l < 3; ++l) {
        for (int j1 = 0; j1 < 9; ++j1)
            addSlotToContainer(std::make_unique<Slot>(playerInventory, j1 + l * 9 + 9, 8 + j1 * 18, 103 + l * 18 + i));
    }
    for (int i1 = 0; i1 < 9; ++i1)
        addSlotToContainer(std::make_unique<Slot>(playerInventory, i1, 8 + i1 * 18, 161 + i));
}

bool ContainerChest::canInteractWith(EntityPlayer* playerIn)
{
    return lowerChestInventory->isUseableByPlayer(playerIn);
}

std::shared_ptr<ItemStack> ContainerChest::transferStackInSlot(EntityPlayer*, int index)
{
    std::shared_ptr<ItemStack> itemstack;
    Slot* slot = inventorySlots.at(static_cast<size_t>(index)).get();
    if (slot && slot->getHasStack()) {
        const std::shared_ptr<ItemStack> itemstack1 = slot->getStack();
        itemstack = copyItemStack(itemstack1);
        if (index < numRows * 9) {
            if (!mergeItemStack(*itemstack1, numRows * 9, static_cast<int>(inventorySlots.size()), true))
                return nullptr;
        } else if (!mergeItemStack(*itemstack1, 0, numRows * 9, false)) {
            return nullptr;
        }
        if (itemstack1->stackSize == 0)
            slot->putStack(nullptr);
        else
            slot->onSlotChanged();
    }
    return itemstack;
}

void ContainerChest::onContainerClosed(EntityPlayer* playerIn)
{
    Container::onContainerClosed(playerIn);
    lowerChestInventory->closeInventory(playerIn);
}

} // namespace mc

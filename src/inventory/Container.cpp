#include "inventory/Container.h"

#include "entity/player/EntityPlayer.h"
#include "entity/player/InventoryPlayer.h"
#include "inventory/CopyItemStack.h"
#include "item/ItemStack.h"
#include "tileentity/TileEntity.h"
#include "util/MathHelper.h"

#include <algorithm>
#include <stdexcept>

namespace mc {

// --- Slot ---

void Slot::onSlotChange(ItemStack* p_75220_1_, const ItemStack* p_75220_2_)
{
    if (p_75220_1_ && p_75220_2_ && p_75220_1_->getItem() == p_75220_2_->getItem()) {
        const int i = p_75220_2_->stackSize - p_75220_1_->stackSize;
        if (i > 0)
            onCrafting(*p_75220_1_, i);
    }
}

void Slot::onPickupFromSlot(EntityPlayer*, const std::shared_ptr<ItemStack>&)
{
    onSlotChanged();
}

std::shared_ptr<ItemStack> Slot::getStack() const
{
    return inventory->getStackInSlot(slotIndex);
}

void Slot::putStack(std::shared_ptr<ItemStack> stack)
{
    inventory->setInventorySlotContents(slotIndex, std::move(stack));
    onSlotChanged();
}

void Slot::onSlotChanged()
{
    inventory->markDirty();
}

int Slot::getSlotStackLimit()
{
    return inventory->getInventoryStackLimit();
}

std::shared_ptr<ItemStack> Slot::decrStackSize(int amount)
{
    return inventory->decrStackSize(slotIndex, amount);
}

// --- Container ---

Slot* Container::addSlotToContainer(std::unique_ptr<Slot> slotIn)
{
    slotIn->slotNumber = static_cast<int>(inventorySlots.size());
    inventorySlots.push_back(std::move(slotIn));
    inventoryItemStacks.push_back(nullptr);
    return inventorySlots.back().get();
}

void Container::onCraftGuiOpened(ICrafting* listener)
{
    if (std::find(crafters.begin(), crafters.end(), listener) != crafters.end())
        throw std::invalid_argument("Listener already listening");
    crafters.push_back(listener);
    listener->updateCraftingInventory(this, getInventory());
    detectAndSendChanges();
}

void Container::removeCraftingFromCrafters(ICrafting* listeners)
{
    const auto it = std::find(crafters.begin(), crafters.end(), listeners);
    if (it != crafters.end())
        crafters.erase(it);
}

std::vector<std::shared_ptr<ItemStack>> Container::getInventory() const
{
    std::vector<std::shared_ptr<ItemStack>> list;
    list.reserve(inventorySlots.size());
    for (const auto& slot : inventorySlots)
        list.push_back(slot->getStack());
    return list;
}

void Container::detectAndSendChanges()
{
    for (size_t i = 0; i < inventorySlots.size(); ++i) {
        const std::shared_ptr<ItemStack> itemstack = inventorySlots[i]->getStack();
        std::shared_ptr<ItemStack> itemstack1 = inventoryItemStacks[i];
        if (!ItemStack::areItemStacksEqual(itemstack1.get(), itemstack.get())) {
            itemstack1 = copyItemStack(itemstack);
            inventoryItemStacks[i] = itemstack1;
            for (ICrafting* crafter : crafters)
                crafter->sendSlotContents(this, static_cast<int>(i), itemstack1.get());
        }
    }
}

Slot* Container::getSlotFromInventory(const IInventory* inv, int slotIn) const
{
    for (const auto& slot : inventorySlots) {
        if (slot->isHere(inv, slotIn))
            return slot.get();
    }
    return nullptr;
}

std::shared_ptr<ItemStack> Container::transferStackInSlot(EntityPlayer*, int index)
{
    Slot* slot = inventorySlots.at(static_cast<size_t>(index)).get();
    return slot ? slot->getStack() : nullptr;
}

std::shared_ptr<ItemStack> Container::slotClick(int slotId, int clickedButton, int mode, EntityPlayer* playerIn)
{
    std::shared_ptr<ItemStack> itemstack;
    InventoryPlayer& inventoryplayer = playerIn->inventory;
    if (mode == 5) {
        const int i = dragEvent;
        dragEvent = getDragEvent(clickedButton);
        if ((i != 1 || dragEvent != 2) && i != dragEvent) {
            resetDrag();
        } else if (!inventoryplayer.getItemStack()) {
            resetDrag();
        } else if (dragEvent == 0) {
            dragMode = extractDragMode(clickedButton);
            if (isValidDragMode(dragMode, playerIn)) {
                dragEvent = 1;
                dragSlots.clear();
            } else {
                resetDrag();
            }
        } else if (dragEvent == 1) {
            Slot* slot = inventorySlots.at(static_cast<size_t>(slotId)).get();
            const ItemStack* held = inventoryplayer.getItemStack().get();
            if (slot && canAddItemToSlot(slot, held, true) && slot->isItemValid(held) && held->stackSize > static_cast<int>(dragSlots.size())
                && canDragIntoSlot(slot)) {
                if (std::find(dragSlots.begin(), dragSlots.end(), slot) == dragSlots.end())
                    dragSlots.push_back(slot);
            }
        } else if (dragEvent == 2) {
            if (!dragSlots.empty()) {
                std::shared_ptr<ItemStack> itemstack3 = copyItemStack(inventoryplayer.getItemStack());
                int j = inventoryplayer.getItemStack()->stackSize;
                for (Slot* slot1 : dragSlots) {
                    const ItemStack* held = inventoryplayer.getItemStack().get();
                    if (slot1 && canAddItemToSlot(slot1, held, true) && slot1->isItemValid(held) && held->stackSize >= static_cast<int>(dragSlots.size())
                        && canDragIntoSlot(slot1)) {
                        std::shared_ptr<ItemStack> itemstack1 = copyItemStack(itemstack3);
                        const int k = slot1->getHasStack() ? slot1->getStack()->stackSize : 0;
                        computeStackSize(dragSlots.size(), dragMode, *itemstack1, k);
                        if (itemstack1->stackSize > itemstack1->getMaxStackSize())
                            itemstack1->stackSize = itemstack1->getMaxStackSize();
                        if (itemstack1->stackSize > slot1->getItemStackLimit(itemstack1.get()))
                            itemstack1->stackSize = slot1->getItemStackLimit(itemstack1.get());
                        j -= itemstack1->stackSize - k;
                        slot1->putStack(itemstack1);
                    }
                }
                itemstack3->stackSize = j;
                if (itemstack3->stackSize <= 0)
                    itemstack3 = nullptr;
                inventoryplayer.setItemStack(itemstack3);
            }
            resetDrag();
        } else {
            resetDrag();
        }
    } else if (dragEvent != 0) {
        resetDrag();
    } else if ((mode == 0 || mode == 1) && (clickedButton == 0 || clickedButton == 1)) {
        if (slotId == -999) {
            if (inventoryplayer.getItemStack()) {
                if (clickedButton == 0) {
                    playerIn->dropPlayerItemWithRandomChoice(inventoryplayer.getItemStack(), true);
                    inventoryplayer.setItemStack(nullptr);
                }
                if (clickedButton == 1) {
                    playerIn->dropPlayerItemWithRandomChoice(std::make_shared<ItemStack>(inventoryplayer.getItemStack()->splitStack(1)), true);
                    if (inventoryplayer.getItemStack()->stackSize == 0)
                        inventoryplayer.setItemStack(nullptr);
                }
            }
        } else if (mode == 1) {
            if (slotId < 0)
                return nullptr;
            Slot* slot6 = inventorySlots.at(static_cast<size_t>(slotId)).get();
            if (slot6 && slot6->canTakeStack(playerIn)) {
                const std::shared_ptr<ItemStack> itemstack8 = transferStackInSlot(playerIn, slotId);
                if (itemstack8) {
                    const Item* item = itemstack8->getItem();
                    itemstack = copyItemStack(itemstack8);
                    if (slot6->getStack() && slot6->getStack()->getItem() == item)
                        retrySlotClick(slotId, clickedButton, true, playerIn);
                }
            }
        } else {
            if (slotId < 0)
                return nullptr;
            Slot* slot7 = inventorySlots.at(static_cast<size_t>(slotId)).get();
            if (slot7) {
                std::shared_ptr<ItemStack> itemstack9 = slot7->getStack();
                const std::shared_ptr<ItemStack> itemstack10 = inventoryplayer.getItemStack();
                if (itemstack9)
                    itemstack = copyItemStack(itemstack9);
                if (!itemstack9) {
                    if (itemstack10 && slot7->isItemValid(itemstack10.get())) {
                        int k2 = clickedButton == 0 ? itemstack10->stackSize : 1;
                        if (k2 > slot7->getItemStackLimit(itemstack10.get()))
                            k2 = slot7->getItemStackLimit(itemstack10.get());
                        if (itemstack10->stackSize >= k2)
                            slot7->putStack(std::make_shared<ItemStack>(itemstack10->splitStack(k2)));
                        if (itemstack10->stackSize == 0)
                            inventoryplayer.setItemStack(nullptr);
                    }
                } else if (slot7->canTakeStack(playerIn)) {
                    if (!itemstack10) {
                        const int j2 = clickedButton == 0 ? itemstack9->stackSize : (itemstack9->stackSize + 1) / 2;
                        const std::shared_ptr<ItemStack> itemstack12 = slot7->decrStackSize(j2);
                        inventoryplayer.setItemStack(itemstack12);
                        if (itemstack9->stackSize == 0)
                            slot7->putStack(nullptr);
                        slot7->onPickupFromSlot(playerIn, inventoryplayer.getItemStack());
                    } else if (slot7->isItemValid(itemstack10.get())) {
                        if (itemstack9->getItem() == itemstack10->getItem() && itemstack9->getMetadata() == itemstack10->getMetadata()
                            && ItemStack::areItemStackTagsEqual(itemstack9.get(), itemstack10.get())) {
                            int i2 = clickedButton == 0 ? itemstack10->stackSize : 1;
                            if (i2 > slot7->getItemStackLimit(itemstack10.get()) - itemstack9->stackSize)
                                i2 = slot7->getItemStackLimit(itemstack10.get()) - itemstack9->stackSize;
                            if (i2 > itemstack10->getMaxStackSize() - itemstack9->stackSize)
                                i2 = itemstack10->getMaxStackSize() - itemstack9->stackSize;
                            itemstack10->splitStack(i2);
                            if (itemstack10->stackSize == 0)
                                inventoryplayer.setItemStack(nullptr);
                            itemstack9->stackSize += i2;
                        } else if (itemstack10->stackSize <= slot7->getItemStackLimit(itemstack10.get())) {
                            slot7->putStack(itemstack10);
                            inventoryplayer.setItemStack(itemstack9);
                        }
                    } else if (itemstack9->getItem() == itemstack10->getItem() && itemstack10->getMaxStackSize() > 1
                        && (!itemstack9->getHasSubtypes() || itemstack9->getMetadata() == itemstack10->getMetadata())
                        && ItemStack::areItemStackTagsEqual(itemstack9.get(), itemstack10.get())) {
                        const int l1 = itemstack9->stackSize;
                        if (l1 > 0 && l1 + itemstack10->stackSize <= itemstack10->getMaxStackSize()) {
                            itemstack10->stackSize += l1;
                            itemstack9 = slot7->decrStackSize(l1);
                            if (itemstack9->stackSize == 0)
                                slot7->putStack(nullptr);
                            slot7->onPickupFromSlot(playerIn, inventoryplayer.getItemStack());
                        }
                    }
                }
                slot7->onSlotChanged();
            }
        }
    } else if (mode == 2 && clickedButton >= 0 && clickedButton < 9) {
        Slot* slot5 = inventorySlots.at(static_cast<size_t>(slotId)).get();
        if (slot5->canTakeStack(playerIn)) {
            const std::shared_ptr<ItemStack> itemstack7 = inventoryplayer.getStackInSlot(clickedButton);
            bool flag = !itemstack7 || (slot5->inventory == &inventoryplayer && slot5->isItemValid(itemstack7.get()));
            int k1 = -1;
            if (!flag) {
                k1 = inventoryplayer.getFirstEmptyStack();
                flag |= k1 > -1;
            }
            if (slot5->getHasStack() && flag) {
                const std::shared_ptr<ItemStack> itemstack11 = slot5->getStack();
                inventoryplayer.setInventorySlotContents(clickedButton, copyItemStack(itemstack11));
                if ((slot5->inventory != &inventoryplayer || !slot5->isItemValid(itemstack7.get())) && itemstack7) {
                    if (k1 > -1) {
                        inventoryplayer.addItemStackToInventory(itemstack7);
                        slot5->decrStackSize(itemstack11->stackSize);
                        slot5->putStack(nullptr);
                        slot5->onPickupFromSlot(playerIn, itemstack11);
                    }
                } else {
                    slot5->decrStackSize(itemstack11->stackSize);
                    slot5->putStack(itemstack7);
                    slot5->onPickupFromSlot(playerIn, itemstack11);
                }
            } else if (!slot5->getHasStack() && itemstack7 && slot5->isItemValid(itemstack7.get())) {
                inventoryplayer.setInventorySlotContents(clickedButton, nullptr);
                slot5->putStack(itemstack7);
            }
        }
    } else if (mode == 3 && playerIn->capabilities.isCreativeMode && !inventoryplayer.getItemStack() && slotId >= 0) {
        Slot* slot4 = inventorySlots.at(static_cast<size_t>(slotId)).get();
        if (slot4 && slot4->getHasStack()) {
            std::shared_ptr<ItemStack> itemstack6 = copyItemStack(slot4->getStack());
            itemstack6->stackSize = itemstack6->getMaxStackSize();
            inventoryplayer.setItemStack(itemstack6);
        }
    } else if (mode == 4 && !inventoryplayer.getItemStack() && slotId >= 0) {
        Slot* slot3 = inventorySlots.at(static_cast<size_t>(slotId)).get();
        if (slot3 && slot3->getHasStack() && slot3->canTakeStack(playerIn)) {
            const std::shared_ptr<ItemStack> itemstack5 = slot3->decrStackSize(clickedButton == 0 ? 1 : slot3->getStack()->stackSize);
            slot3->onPickupFromSlot(playerIn, itemstack5);
            playerIn->dropPlayerItemWithRandomChoice(itemstack5, true);
        }
    } else if (mode == 6 && slotId >= 0) {
        Slot* slot2 = inventorySlots.at(static_cast<size_t>(slotId)).get();
        const std::shared_ptr<ItemStack> itemstack4 = inventoryplayer.getItemStack();
        if (itemstack4 && (!slot2 || !slot2->getHasStack() || !slot2->canTakeStack(playerIn))) {
            const int i1 = clickedButton == 0 ? 0 : static_cast<int>(inventorySlots.size()) - 1;
            const int j1 = clickedButton == 0 ? 1 : -1;
            for (int l2 = 0; l2 < 2; ++l2) {
                for (int i3 = i1; i3 >= 0 && i3 < static_cast<int>(inventorySlots.size()) && itemstack4->stackSize < itemstack4->getMaxStackSize(); i3 += j1) {
                    Slot* slot8 = inventorySlots[static_cast<size_t>(i3)].get();
                    if (slot8->getHasStack() && canAddItemToSlot(slot8, itemstack4.get(), true) && slot8->canTakeStack(playerIn)
                        && canMergeSlot(itemstack4.get(), slot8) && (l2 != 0 || slot8->getStack()->stackSize != slot8->getStack()->getMaxStackSize())) {
                        const int l = std::min(itemstack4->getMaxStackSize() - itemstack4->stackSize, slot8->getStack()->stackSize);
                        const std::shared_ptr<ItemStack> itemstack2 = slot8->decrStackSize(l);
                        itemstack4->stackSize += l;
                        if (itemstack2->stackSize <= 0)
                            slot8->putStack(nullptr);
                        slot8->onPickupFromSlot(playerIn, itemstack2);
                    }
                }
            }
        }
        detectAndSendChanges();
    }
    return itemstack;
}

void Container::retrySlotClick(int slotId, int clickedButton, bool, EntityPlayer* playerIn)
{
    slotClick(slotId, clickedButton, 1, playerIn);
}

void Container::onContainerClosed(EntityPlayer* playerIn)
{
    InventoryPlayer& inventoryplayer = playerIn->inventory;
    if (inventoryplayer.getItemStack()) {
        playerIn->dropPlayerItemWithRandomChoice(inventoryplayer.getItemStack(), false);
        inventoryplayer.setItemStack(nullptr);
    }
}

void Container::putStacksInSlots(const std::vector<std::shared_ptr<ItemStack>>& p_75131_1_)
{
    for (size_t i = 0; i < p_75131_1_.size(); ++i)
        getSlot(static_cast<int>(i))->putStack(p_75131_1_[i]);
}

bool Container::getCanCraft(EntityPlayer* p_75129_1_) const
{
    return std::find(playerList.begin(), playerList.end(), p_75129_1_) == playerList.end();
}

void Container::setCanCraft(EntityPlayer* p_75128_1_, bool p_75128_2_)
{
    const auto it = std::find(playerList.begin(), playerList.end(), p_75128_1_);
    if (p_75128_2_) {
        if (it != playerList.end())
            playerList.erase(it);
    } else if (it == playerList.end()) {
        playerList.push_back(p_75128_1_);
    }
}

bool Container::mergeItemStack(ItemStack& stack, int startIndex, int endIndex, bool reverseDirection)
{
    bool flag = false;
    int i = startIndex;
    if (reverseDirection)
        i = endIndex - 1;
    if (stack.isStackable()) {
        while (stack.stackSize > 0 && ((!reverseDirection && i < endIndex) || (reverseDirection && i >= startIndex))) {
            Slot* slot = inventorySlots.at(static_cast<size_t>(i)).get();
            const std::shared_ptr<ItemStack> itemstack = slot->getStack();
            if (itemstack && itemstack->getItem() == stack.getItem() && (!stack.getHasSubtypes() || stack.getMetadata() == itemstack->getMetadata())
                && ItemStack::areItemStackTagsEqual(&stack, itemstack.get())) {
                const int j = itemstack->stackSize + stack.stackSize;
                if (j <= stack.getMaxStackSize()) {
                    stack.stackSize = 0;
                    itemstack->stackSize = j;
                    slot->onSlotChanged();
                    flag = true;
                } else if (itemstack->stackSize < stack.getMaxStackSize()) {
                    stack.stackSize -= stack.getMaxStackSize() - itemstack->stackSize;
                    itemstack->stackSize = stack.getMaxStackSize();
                    slot->onSlotChanged();
                    flag = true;
                }
            }
            if (reverseDirection)
                --i;
            else
                ++i;
        }
    }
    if (stack.stackSize > 0) {
        if (reverseDirection)
            i = endIndex - 1;
        else
            i = startIndex;
        while ((!reverseDirection && i < endIndex) || (reverseDirection && i >= startIndex)) {
            Slot* slot1 = inventorySlots.at(static_cast<size_t>(i)).get();
            if (!slot1->getStack()) {
                slot1->putStack(std::make_shared<ItemStack>(stack.copy()));
                slot1->onSlotChanged();
                stack.stackSize = 0;
                flag = true;
                break;
            }
            if (reverseDirection)
                --i;
            else
                ++i;
        }
    }
    return flag;
}

bool Container::isValidDragMode(int dragModeIn, EntityPlayer* player)
{
    return dragModeIn == 0 ? true : (dragModeIn == 1 ? true : dragModeIn == 2 && player->capabilities.isCreativeMode);
}

void Container::resetDrag()
{
    dragEvent = 0;
    dragSlots.clear();
}

bool Container::canAddItemToSlot(Slot* slotIn, const ItemStack* stack, bool stackSizeMatters)
{
    bool flag = !slotIn || !slotIn->getHasStack();
    if (slotIn && slotIn->getHasStack() && stack && stack->isItemEqual(slotIn->getStack().get())
        && ItemStack::areItemStackTagsEqual(slotIn->getStack().get(), stack))
        flag |= slotIn->getStack()->stackSize + (stackSizeMatters ? 0 : stack->stackSize) <= stack->getMaxStackSize();
    return flag;
}

void Container::computeStackSize(size_t dragSlotCount, int p_94525_1_, ItemStack& p_94525_2_, int p_94525_3_)
{
    switch (p_94525_1_) {
    case 0:
        p_94525_2_.stackSize = MathHelper::floor_float(static_cast<float>(p_94525_2_.stackSize) / static_cast<float>(dragSlotCount));
        break;
    case 1:
        p_94525_2_.stackSize = 1;
        break;
    case 2:
        p_94525_2_.stackSize = p_94525_2_.getItem()->getItemStackLimit();
        break;
    default:
        break;
    }
    p_94525_2_.stackSize += p_94525_3_;
}

int Container::calcRedstone(TileEntity* te)
{
    return calcRedstoneFromInventory(dynamic_cast<IInventory*>(te));
}

int Container::calcRedstoneFromInventory(IInventory* inv)
{
    if (!inv)
        return 0;
    int i = 0;
    float f = 0.0f;
    for (int j = 0; j < inv->getSizeInventory(); ++j) {
        const std::shared_ptr<ItemStack> itemstack = inv->getStackInSlot(j);
        if (itemstack) {
            f += static_cast<float>(itemstack->stackSize) / static_cast<float>(std::min(inv->getInventoryStackLimit(), itemstack->getMaxStackSize()));
            ++i;
        }
    }
    f = f / static_cast<float>(inv->getSizeInventory());
    return MathHelper::floor_float(f * 14.0f) + (i > 0 ? 1 : 0);
}

} // namespace mc

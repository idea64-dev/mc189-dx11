#include "inventory/InventoryBasic.h"

#include "inventory/CopyItemStack.h"
#include "item/ItemStack.h"
#include "nbt/NBTTagCompound.h"
#include "tileentity/TileEntities.h"
#include "util/ChatComponent.h"

#include <algorithm>

namespace mc {

InventoryBasic::InventoryBasic(std::string title, bool customNameIn, int slotCount)
    : inventoryTitle(std::move(title))
    , slotsCount(slotCount)
    , inventoryContents(static_cast<size_t>(slotCount))
    , customName(customNameIn)
{
}

InventoryBasic::InventoryBasic(IChatComponent& title, int slotCount)
    : InventoryBasic(title.getUnformattedText(), true, slotCount)
{
}

void InventoryBasic::removeInventoryChangeListener(IInvBasic* listener)
{
    const auto it = std::find(changeListeners.begin(), changeListeners.end(), listener);
    if (it != changeListeners.end())
        changeListeners.erase(it);
}

std::shared_ptr<ItemStack> InventoryBasic::getStackInSlot(int index) const
{
    return index >= 0 && index < static_cast<int>(inventoryContents.size()) ? inventoryContents[static_cast<size_t>(index)] : nullptr;
}

std::shared_ptr<ItemStack> InventoryBasic::decrStackSize(int index, int count)
{
    std::shared_ptr<ItemStack>& slot = inventoryContents.at(static_cast<size_t>(index));
    if (!slot)
        return nullptr;
    if (slot->stackSize <= count) {
        std::shared_ptr<ItemStack> itemstack1 = std::move(slot);
        slot = nullptr;
        markDirty();
        return itemstack1;
    }
    auto itemstack = std::make_shared<ItemStack>(slot->splitStack(count));
    if (slot->stackSize == 0)
        slot = nullptr;
    markDirty();
    return itemstack;
}

std::shared_ptr<ItemStack> InventoryBasic::func_174894_a(const ItemStack& stack)
{
    std::shared_ptr<ItemStack> itemstack = copyItemStack(&stack);
    for (int i = 0; i < slotsCount; ++i) {
        const std::shared_ptr<ItemStack> itemstack1 = getStackInSlot(i);
        if (!itemstack1) {
            setInventorySlotContents(i, itemstack);
            markDirty();
            return nullptr;
        }
        if (ItemStack::areItemsEqual(itemstack1.get(), itemstack.get())) {
            const int j = std::min(getInventoryStackLimit(), itemstack1->getMaxStackSize());
            const int k = std::min(itemstack->stackSize, j - itemstack1->stackSize);
            if (k > 0) {
                itemstack1->stackSize += k;
                itemstack->stackSize -= k;
                if (itemstack->stackSize <= 0) {
                    markDirty();
                    return nullptr;
                }
            }
        }
    }
    if (itemstack->stackSize != stack.stackSize)
        markDirty();
    return itemstack;
}

std::shared_ptr<ItemStack> InventoryBasic::removeStackFromSlot(int index)
{
    std::shared_ptr<ItemStack>& slot = inventoryContents.at(static_cast<size_t>(index));
    std::shared_ptr<ItemStack> itemstack = std::move(slot);
    slot = nullptr;
    return itemstack;
}

void InventoryBasic::setInventorySlotContents(int index, std::shared_ptr<ItemStack> stack)
{
    if (stack && stack->stackSize > getInventoryStackLimit())
        stack->stackSize = getInventoryStackLimit();
    inventoryContents.at(static_cast<size_t>(index)) = std::move(stack);
    markDirty();
}

void InventoryBasic::setCustomName(std::string inventoryTitleIn)
{
    customName = true;
    inventoryTitle = std::move(inventoryTitleIn);
}

std::shared_ptr<IChatComponent> InventoryBasic::getDisplayName() const
{
    if (hasCustomName())
        return std::make_shared<ChatComponentText>(getName());
    return std::make_shared<ChatComponentTranslation>(getName());
}

void InventoryBasic::markDirty()
{
    for (size_t i = 0; i < changeListeners.size(); ++i)
        changeListeners[i]->onInventoryChanged(this);
}

void InventoryBasic::clear()
{
    std::fill(inventoryContents.begin(), inventoryContents.end(), nullptr);
}

void InventoryEnderChest::loadInventoryFromNBT(const NBTTagList& p_70486_1_)
{
    for (int i = 0; i < getSizeInventory(); ++i)
        setInventorySlotContents(i, nullptr);
    for (int k = 0; k < p_70486_1_.tagCount(); ++k) {
        const std::shared_ptr<NBTTagCompound> nbttagcompound = p_70486_1_.getCompoundTagAt(k);
        const int j = nbttagcompound->getByte("Slot") & 255;
        if (j >= 0 && j < getSizeInventory())
            setInventorySlotContents(j, std::shared_ptr<ItemStack>(ItemStack::loadItemStackFromNBT(*nbttagcompound)));
    }
}

std::shared_ptr<NBTTagList> InventoryEnderChest::saveInventoryToNBT() const
{
    auto nbttaglist = std::make_shared<NBTTagList>();
    for (int i = 0; i < getSizeInventory(); ++i) {
        if (const std::shared_ptr<ItemStack> itemstack = getStackInSlot(i)) {
            auto nbttagcompound = std::make_shared<NBTTagCompound>();
            nbttagcompound->setByte("Slot", static_cast<int8_t>(i));
            itemstack->writeToNBT(*nbttagcompound);
            nbttaglist->appendTag(nbttagcompound);
        }
    }
    return nbttaglist;
}

bool InventoryEnderChest::isUseableByPlayer(EntityPlayer* player)
{
    // Java: false when the associated chest's canBeUsed(player) fails -- TileEntityEnderChest's viewer logic
    // comes with the GUI stages.
    return InventoryBasic::isUseableByPlayer(player);
}

void InventoryEnderChest::openInventory(EntityPlayer* player)
{
    // Java: associatedChest.openChest() -- with TileEntityEnderChest's viewer logic.
    InventoryBasic::openInventory(player);
}

void InventoryEnderChest::closeInventory(EntityPlayer* player)
{
    // Java: associatedChest.closeChest() -- with TileEntityEnderChest's viewer logic.
    InventoryBasic::closeInventory(player);
    associatedChest = nullptr;
}

} // namespace mc

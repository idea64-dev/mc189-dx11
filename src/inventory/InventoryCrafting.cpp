#include "inventory/InventoryCrafting.h"

#include "entity/player/EntityPlayer.h"
#include "inventory/Container.h"
#include "item/ItemStack.h"
#include "util/ChatComponent.h"

#include <algorithm>

namespace mc {

// --- InventoryCrafting ---

InventoryCrafting::InventoryCrafting(Container* eventHandlerIn, int width, int height)
    : stackList(static_cast<size_t>(width * height))
    , inventoryWidth(width)
    , inventoryHeight(height)
    , eventHandler(eventHandlerIn)
{
}

std::shared_ptr<ItemStack> InventoryCrafting::getStackInSlot(int index) const
{
    return index >= getSizeInventory() ? nullptr : stackList.at(static_cast<size_t>(index));
}

std::shared_ptr<ItemStack> InventoryCrafting::getStackInRowAndColumn(int row, int column) const
{
    return row >= 0 && row < inventoryWidth && column >= 0 && column <= inventoryHeight ? getStackInSlot(row + column * inventoryWidth) : nullptr;
}

std::shared_ptr<IChatComponent> InventoryCrafting::getDisplayName() const
{
    if (hasCustomName())
        return std::make_shared<ChatComponentText>(getName());
    return std::make_shared<ChatComponentTranslation>(getName());
}

std::shared_ptr<ItemStack> InventoryCrafting::removeStackFromSlot(int index)
{
    std::shared_ptr<ItemStack>& slot = stackList.at(static_cast<size_t>(index));
    std::shared_ptr<ItemStack> itemstack = std::move(slot);
    slot = nullptr;
    return itemstack;
}

std::shared_ptr<ItemStack> InventoryCrafting::decrStackSize(int index, int count)
{
    std::shared_ptr<ItemStack>& slot = stackList.at(static_cast<size_t>(index));
    if (!slot)
        return nullptr;
    if (slot->stackSize <= count) {
        std::shared_ptr<ItemStack> itemstack1 = std::move(slot);
        slot = nullptr;
        eventHandler->onCraftMatrixChanged(this);
        return itemstack1;
    }
    auto itemstack = std::make_shared<ItemStack>(slot->splitStack(count));
    if (slot->stackSize == 0)
        slot = nullptr;
    eventHandler->onCraftMatrixChanged(this);
    return itemstack;
}

void InventoryCrafting::setInventorySlotContents(int index, std::shared_ptr<ItemStack> stack)
{
    stackList.at(static_cast<size_t>(index)) = std::move(stack);
    eventHandler->onCraftMatrixChanged(this);
}

void InventoryCrafting::clear()
{
    std::fill(stackList.begin(), stackList.end(), nullptr);
}

// --- InventoryCraftResult ---

std::shared_ptr<IChatComponent> InventoryCraftResult::getDisplayName() const
{
    if (hasCustomName())
        return std::make_shared<ChatComponentText>(getName());
    return std::make_shared<ChatComponentTranslation>(getName());
}

std::shared_ptr<ItemStack> InventoryCraftResult::decrStackSize(int, int)
{
    std::shared_ptr<ItemStack> itemstack = std::move(stackResult[0]);
    stackResult[0] = nullptr;
    return itemstack;
}

std::shared_ptr<ItemStack> InventoryCraftResult::removeStackFromSlot(int)
{
    std::shared_ptr<ItemStack> itemstack = std::move(stackResult[0]);
    stackResult[0] = nullptr;
    return itemstack;
}

// --- SlotCrafting ---

std::shared_ptr<ItemStack> SlotCrafting::decrStackSize(int amount)
{
    if (getHasStack())
        amountCrafted += std::min(amount, getStack()->stackSize);
    return Slot::decrStackSize(amount);
}

void SlotCrafting::onCrafting(ItemStack& stack, int amount)
{
    amountCrafted += amount;
    onCrafting(stack);
}

void SlotCrafting::onCrafting(ItemStack& stack)
{
    // Java: stack.onCrafting(world, player, amountCrafted) when amountCrafted > 0 (Item.onCreated), then the
    // crafting achievements -- they come with the item hooks and the statistics.
    (void)stack;
    amountCrafted = 0;
}

void SlotCrafting::onPickupFromSlot(EntityPlayer* playerIn, const std::shared_ptr<ItemStack>& stack)
{
    onCrafting(*stack);
    // Java: CraftingManager.func_180303_b(craftMatrix, world) -- the items left in the grid (buckets, bottles),
    // with recipes. Without them nothing is left: each used slot loses one item.
    (void)playerIn;
    for (int i = 0; i < craftMatrix->getSizeInventory(); ++i) {
        if (craftMatrix->getStackInSlot(i))
            craftMatrix->decrStackSize(i, 1);
    }
    (void)thePlayer; // Java: gives the left items to this player's inventory
}

} // namespace mc

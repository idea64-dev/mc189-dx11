#include "inventory/ContainerPlayer.h"

#include "entity/player/EntityPlayer.h"
#include "entity/player/InventoryPlayer.h"
#include "init/Blocks.h"
#include "init/Items.h"
#include "inventory/CopyItemStack.h"
#include "item/ItemArmor.h"
#include "item/ItemStack.h"

namespace mc {

namespace {

// Java: the anonymous armor Slot of ContainerPlayer.
class SlotArmor : public Slot {
public:
    SlotArmor(IInventory* inventoryIn, int index, int xPosition, int yPosition, int armorTypeIn)
        : Slot(inventoryIn, index, xPosition, yPosition)
        , k_f(armorTypeIn)
    {
    }

    int getSlotStackLimit() override { return 1; }

    bool isItemValid(const ItemStack* stack) override
    {
        if (!stack)
            return false;
        if (const auto* armor = dynamic_cast<const ItemArmor*>(stack->getItem()))
            return armor->armorType == k_f;
        if (stack->getItem() != Item::getItemFromBlock(Blocks::pumpkin) && stack->getItem() != Items::skull)
            return false;
        return k_f == 0;
    }

    std::optional<std::string> getSlotTexture() override { return std::string(ItemArmor::EMPTY_SLOT_NAMES[k_f]); }

private:
    const int k_f;
};

} // namespace

ContainerPlayer::ContainerPlayer(InventoryPlayer& playerInventory, bool localWorld, EntityPlayer* player)
    : isLocalWorld(localWorld)
    , thePlayer(player)
{
    addSlotToContainer(std::make_unique<SlotCrafting>(playerInventory.player, &craftMatrix, &craftResult, 0, 144, 36));
    for (int i = 0; i < 2; ++i) {
        for (int j = 0; j < 2; ++j)
            addSlotToContainer(std::make_unique<Slot>(&craftMatrix, j + i * 2, 88 + j * 18, 26 + i * 18));
    }
    for (int k = 0; k < 4; ++k)
        addSlotToContainer(std::make_unique<SlotArmor>(&playerInventory, playerInventory.getSizeInventory() - 1 - k, 8, 8 + k * 18, k));
    for (int l = 0; l < 3; ++l) {
        for (int j1 = 0; j1 < 9; ++j1)
            addSlotToContainer(std::make_unique<Slot>(&playerInventory, j1 + (l + 1) * 9, 8 + j1 * 18, 84 + l * 18));
    }
    for (int i1 = 0; i1 < 9; ++i1)
        addSlotToContainer(std::make_unique<Slot>(&playerInventory, i1, 8 + i1 * 18, 142));
    onCraftMatrixChanged(&craftMatrix);
}

void ContainerPlayer::onCraftMatrixChanged(IInventory*)
{
    // Java: CraftingManager.getInstance().findMatchingRecipe(craftMatrix, thePlayer.worldObj) -- recipes are
    // not ported yet, so nothing matches.
    (void)thePlayer;
    craftResult.setInventorySlotContents(0, nullptr);
}

void ContainerPlayer::onContainerClosed(EntityPlayer* playerIn)
{
    Container::onContainerClosed(playerIn);
    for (int i = 0; i < 4; ++i) {
        if (const std::shared_ptr<ItemStack> itemstack = craftMatrix.removeStackFromSlot(i))
            playerIn->dropPlayerItemWithRandomChoice(itemstack, false);
    }
    craftResult.setInventorySlotContents(0, nullptr);
}

std::shared_ptr<ItemStack> ContainerPlayer::transferStackInSlot(EntityPlayer* playerIn, int index)
{
    std::shared_ptr<ItemStack> itemstack;
    Slot* slot = inventorySlots.at(static_cast<size_t>(index)).get();
    if (slot && slot->getHasStack()) {
        const std::shared_ptr<ItemStack> itemstack1 = slot->getStack();
        itemstack = copyItemStack(itemstack1);
        const auto* armor = dynamic_cast<const ItemArmor*>(itemstack->getItem());
        if (index == 0) {
            if (!mergeItemStack(*itemstack1, 9, 45, true))
                return nullptr;
            slot->onSlotChange(itemstack1.get(), itemstack.get());
        } else if (index >= 1 && index < 5) {
            if (!mergeItemStack(*itemstack1, 9, 45, false))
                return nullptr;
        } else if (index >= 5 && index < 9) {
            if (!mergeItemStack(*itemstack1, 9, 45, false))
                return nullptr;
        } else if (armor && !inventorySlots.at(static_cast<size_t>(5 + armor->armorType))->getHasStack()) {
            const int i = 5 + armor->armorType;
            if (!mergeItemStack(*itemstack1, i, i + 1, false))
                return nullptr;
        } else if (index >= 9 && index < 36) {
            if (!mergeItemStack(*itemstack1, 36, 45, false))
                return nullptr;
        } else if (index >= 36 && index < 45) {
            if (!mergeItemStack(*itemstack1, 9, 36, false))
                return nullptr;
        } else if (!mergeItemStack(*itemstack1, 9, 45, false)) {
            return nullptr;
        }
        if (itemstack1->stackSize == 0)
            slot->putStack(nullptr);
        else
            slot->onSlotChanged();
        if (itemstack1->stackSize == itemstack->stackSize)
            return nullptr;
        slot->onPickupFromSlot(playerIn, itemstack1);
    }
    return itemstack;
}

bool ContainerPlayer::canMergeSlot(const ItemStack* stack, Slot* slotIn)
{
    return slotIn->inventory != &craftResult && Container::canMergeSlot(stack, slotIn);
}

} // namespace mc

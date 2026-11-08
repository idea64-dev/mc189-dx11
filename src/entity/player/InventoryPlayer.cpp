#include "entity/player/InventoryPlayer.h"

#include "block/Block.h"
#include "block/material/Material.h"
#include "entity/EntityItemHooks.h"
#include "entity/player/EntityPlayer.h"
#include "inventory/CopyItemStack.h"
#include "item/ItemArmor.h"
#include "item/ItemStack.h"
#include "nbt/NBTTagCompound.h"
#include "util/ChatComponent.h"

#include <algorithm>
#include <stdexcept>

namespace mc {

namespace {

// Java: NBTUtil.func_181123_a -- whether every tag of `a` is in `b` (lists: any order when `listsAnyOrder`).
bool nbtContains(const NBTBase* a, const NBTBase* b, bool listsAnyOrder)
{
    if (a == b || !a)
        return true;
    if (!b || a->getId() != b->getId())
        return false;
    if (const auto* compound = dynamic_cast<const NBTTagCompound*>(a)) {
        const auto* other = static_cast<const NBTTagCompound*>(b);
        for (const std::string& s : compound->getKeySet()) {
            if (!nbtContains(compound->getTag(s).get(), other->getTag(s).get(), listsAnyOrder))
                return false;
        }
        return true;
    }
    if (const auto* list = dynamic_cast<const NBTTagList*>(a); list && listsAnyOrder) {
        const auto* other = static_cast<const NBTTagList*>(b);
        if (list->tagCount() == 0)
            return other->tagCount() == 0;
        for (int i = 0; i < list->tagCount(); ++i) {
            const std::shared_ptr<NBTBase> nbtbase = list->get(i);
            bool flag = false;
            for (int j = 0; j < other->tagCount(); ++j) {
                if (nbtContains(nbtbase.get(), other->get(j).get(), listsAnyOrder)) {
                    flag = true;
                    break;
                }
            }
            if (!flag)
                return false;
        }
        return true;
    }
    return a->equals(*b);
}

} // namespace

std::shared_ptr<ItemStack> InventoryPlayer::getCurrentItem() const
{
    return currentItem < 9 && currentItem >= 0 ? mainInventory[static_cast<size_t>(currentItem)] : nullptr;
}

int InventoryPlayer::getInventorySlotContainItem(const Item* itemIn) const
{
    for (size_t i = 0; i < mainInventory.size(); ++i) {
        if (mainInventory[i] && mainInventory[i]->getItem() == itemIn)
            return static_cast<int>(i);
    }
    return -1;
}

int InventoryPlayer::getInventorySlotContainItemAndDamage(const Item* itemIn, int metadataIn) const
{
    for (size_t i = 0; i < mainInventory.size(); ++i) {
        if (mainInventory[i] && mainInventory[i]->getItem() == itemIn && mainInventory[i]->getMetadata() == metadataIn)
            return static_cast<int>(i);
    }
    return -1;
}

int InventoryPlayer::storeItemStack(const ItemStack& itemStackIn) const
{
    for (size_t i = 0; i < mainInventory.size(); ++i) {
        const ItemStack* stack = mainInventory[i].get();
        if (stack && stack->getItem() == itemStackIn.getItem() && stack->isStackable() && stack->stackSize < stack->getMaxStackSize()
            && stack->stackSize < getInventoryStackLimit() && (!stack->getHasSubtypes() || stack->getMetadata() == itemStackIn.getMetadata())
            && ItemStack::areItemStackTagsEqual(stack, &itemStackIn))
            return static_cast<int>(i);
    }
    return -1;
}

int InventoryPlayer::getFirstEmptyStack() const
{
    for (size_t i = 0; i < mainInventory.size(); ++i) {
        if (!mainInventory[i])
            return static_cast<int>(i);
    }
    return -1;
}

void InventoryPlayer::setCurrentItem(Item* itemIn, int metadataIn, bool isMetaSpecific, bool p_146030_4_)
{
    const std::shared_ptr<ItemStack> itemstack = getCurrentItem();
    const int i = isMetaSpecific ? getInventorySlotContainItemAndDamage(itemIn, metadataIn) : getInventorySlotContainItem(itemIn);
    if (i >= 0 && i < 9) {
        currentItem = i;
    } else if (p_146030_4_ && itemIn) {
        const int j = getFirstEmptyStack();
        if (j >= 0 && j < 9)
            currentItem = j;
        if (!itemstack || !itemstack->isItemEnchantable()
            || getInventorySlotContainItemAndDamage(itemstack->getItem(), itemstack->getItemDamage()) != currentItem) {
            const int k = getInventorySlotContainItemAndDamage(itemIn, metadataIn);
            int l;
            if (k >= 0) {
                l = mainInventory[static_cast<size_t>(k)]->stackSize;
                mainInventory[static_cast<size_t>(k)] = mainInventory[static_cast<size_t>(currentItem)];
            } else {
                l = 1;
            }
            mainInventory[static_cast<size_t>(currentItem)] = std::make_shared<ItemStack>(itemIn, l, metadataIn);
        }
    }
}

void InventoryPlayer::changeCurrentItem(int direction)
{
    if (direction > 0)
        direction = 1;
    if (direction < 0)
        direction = -1;
    for (currentItem -= direction; currentItem < 0; currentItem += 9) {
    }
    while (currentItem >= 9)
        currentItem -= 9;
}

int InventoryPlayer::clearMatchingItems(Item* itemIn, int metadataIn, int removeCount, const NBTTagCompound* itemNBT)
{
    int i = 0;
    for (auto& slot : mainInventory) {
        const std::shared_ptr<ItemStack> itemstack = slot;
        if (itemstack && (!itemIn || itemstack->getItem() == itemIn) && (metadataIn <= -1 || itemstack->getMetadata() == metadataIn)
            && (!itemNBT || nbtContains(itemNBT, itemstack->getTagCompound().get(), true))) {
            const int k = removeCount <= 0 ? itemstack->stackSize : std::min(removeCount - i, itemstack->stackSize);
            i += k;
            if (removeCount != 0) {
                slot->stackSize -= k;
                if (slot->stackSize == 0)
                    slot = nullptr;
                if (removeCount > 0 && i >= removeCount)
                    return i;
            }
        }
    }
    for (auto& slot : armorInventory) {
        const std::shared_ptr<ItemStack> itemstack1 = slot;
        if (itemstack1 && (!itemIn || itemstack1->getItem() == itemIn) && (metadataIn <= -1 || itemstack1->getMetadata() == metadataIn)
            && (!itemNBT || nbtContains(itemNBT, itemstack1->getTagCompound().get(), false))) {
            const int j1 = removeCount <= 0 ? itemstack1->stackSize : std::min(removeCount - i, itemstack1->stackSize);
            i += j1;
            if (removeCount != 0) {
                slot->stackSize -= j1;
                if (slot->stackSize == 0)
                    slot = nullptr;
                if (removeCount > 0 && i >= removeCount)
                    return i;
            }
        }
    }
    if (itemStack) {
        if (itemIn && itemStack->getItem() != itemIn)
            return i;
        if (metadataIn > -1 && itemStack->getMetadata() != metadataIn)
            return i;
        if (itemNBT && !nbtContains(itemNBT, itemStack->getTagCompound().get(), false))
            return i;
        const int i1 = removeCount <= 0 ? itemStack->stackSize : std::min(removeCount - i, itemStack->stackSize);
        i += i1;
        if (removeCount != 0) {
            itemStack->stackSize -= i1;
            if (itemStack->stackSize == 0)
                itemStack = nullptr;
            if (removeCount > 0 && i >= removeCount)
                return i;
        }
    }
    return i;
}

int InventoryPlayer::storePartialItemStack(const ItemStack& itemStackIn)
{
    Item* item = itemStackIn.getItem();
    int i = itemStackIn.stackSize;
    int j = storeItemStack(itemStackIn);
    if (j < 0)
        j = getFirstEmptyStack();
    if (j < 0)
        return i;
    std::shared_ptr<ItemStack>& slot = mainInventory[static_cast<size_t>(j)];
    if (!slot) {
        slot = std::make_shared<ItemStack>(item, 0, itemStackIn.getMetadata());
        if (itemStackIn.hasTagCompound())
            slot->setTagCompound(std::static_pointer_cast<NBTTagCompound>(itemStackIn.getTagCompound()->copy()));
    }
    int k = i;
    if (i > slot->getMaxStackSize() - slot->stackSize)
        k = slot->getMaxStackSize() - slot->stackSize;
    if (k > getInventoryStackLimit() - slot->stackSize)
        k = getInventoryStackLimit() - slot->stackSize;
    if (k == 0)
        return i;
    i = i - k;
    slot->stackSize += k;
    slot->animationsToGo = 5;
    return i;
}

void InventoryPlayer::decrementAnimations()
{
    for (auto& slot : mainInventory) {
        if (slot) {
            // Java: ItemStack.updateAnimation -- the decrement, then Item.onUpdate (maps; with the item hooks).
            if (slot->animationsToGo > 0)
                --slot->animationsToGo;
        }
    }
}

bool InventoryPlayer::consumeInventoryItem(Item* itemIn)
{
    const int i = getInventorySlotContainItem(itemIn);
    if (i < 0)
        return false;
    std::shared_ptr<ItemStack>& slot = mainInventory[static_cast<size_t>(i)];
    if (--slot->stackSize <= 0)
        slot = nullptr;
    return true;
}

bool InventoryPlayer::hasItem(Item* itemIn) const
{
    return getInventorySlotContainItem(itemIn) >= 0;
}

bool InventoryPlayer::addItemStackToInventory(const std::shared_ptr<ItemStack>& itemStackIn)
{
    if (!itemStackIn || itemStackIn->stackSize == 0 || !itemStackIn->getItem())
        return false;
    if (itemStackIn->isItemDamaged()) {
        const int j = getFirstEmptyStack();
        if (j >= 0) {
            mainInventory[static_cast<size_t>(j)] = copyItemStack(itemStackIn);
            mainInventory[static_cast<size_t>(j)]->animationsToGo = 5;
            itemStackIn->stackSize = 0;
            return true;
        }
        if (player->capabilities.isCreativeMode) {
            itemStackIn->stackSize = 0;
            return true;
        }
        return false;
    }
    int i;
    while (true) {
        i = itemStackIn->stackSize;
        itemStackIn->stackSize = storePartialItemStack(*itemStackIn);
        if (itemStackIn->stackSize <= 0 || itemStackIn->stackSize >= i)
            break;
    }
    if (itemStackIn->stackSize == i && player->capabilities.isCreativeMode) {
        itemStackIn->stackSize = 0;
        return true;
    }
    return itemStackIn->stackSize < i;
}

std::shared_ptr<ItemStack>& InventoryPlayer::slotRef(int index)
{
    if (index >= static_cast<int>(mainInventory.size()))
        return armorInventory.at(static_cast<size_t>(index - static_cast<int>(mainInventory.size())));
    if (index < 0)
        throw std::out_of_range("inventory slot");
    return mainInventory[static_cast<size_t>(index)];
}

std::shared_ptr<ItemStack> InventoryPlayer::decrStackSize(int index, int count)
{
    std::shared_ptr<ItemStack>& slot = slotRef(index);
    if (!slot)
        return nullptr;
    if (slot->stackSize <= count) {
        std::shared_ptr<ItemStack> itemstack1 = std::move(slot);
        slot = nullptr;
        return itemstack1;
    }
    auto itemstack = std::make_shared<ItemStack>(slot->splitStack(count));
    if (slot->stackSize == 0)
        slot = nullptr;
    return itemstack;
}

std::shared_ptr<ItemStack> InventoryPlayer::removeStackFromSlot(int index)
{
    std::shared_ptr<ItemStack>& slot = slotRef(index);
    std::shared_ptr<ItemStack> itemstack = std::move(slot);
    slot = nullptr;
    return itemstack;
}

void InventoryPlayer::setInventorySlotContents(int index, std::shared_ptr<ItemStack> stack)
{
    slotRef(index) = std::move(stack);
}

float InventoryPlayer::getStrVsBlock(const Block* blockIn) const
{
    float f = 1.0f;
    if (const auto& stack = mainInventory[static_cast<size_t>(currentItem)])
        f *= stack->getStrVsBlock(blockIn);
    return f;
}

NBTTagList& InventoryPlayer::writeToNBT(NBTTagList& nbtTagListIn) const
{
    for (size_t i = 0; i < mainInventory.size(); ++i) {
        if (mainInventory[i]) {
            auto nbttagcompound = std::make_shared<NBTTagCompound>();
            nbttagcompound->setByte("Slot", static_cast<int8_t>(i));
            mainInventory[i]->writeToNBT(*nbttagcompound);
            nbtTagListIn.appendTag(nbttagcompound);
        }
    }
    for (size_t j = 0; j < armorInventory.size(); ++j) {
        if (armorInventory[j]) {
            auto nbttagcompound1 = std::make_shared<NBTTagCompound>();
            nbttagcompound1->setByte("Slot", static_cast<int8_t>(j + 100));
            armorInventory[j]->writeToNBT(*nbttagcompound1);
            nbtTagListIn.appendTag(nbttagcompound1);
        }
    }
    return nbtTagListIn;
}

void InventoryPlayer::readFromNBT(const NBTTagList& nbtTagListIn)
{
    mainInventory = {};
    armorInventory = {};
    for (int i = 0; i < nbtTagListIn.tagCount(); ++i) {
        const std::shared_ptr<NBTTagCompound> nbttagcompound = nbtTagListIn.getCompoundTagAt(i);
        const int j = nbttagcompound->getByte("Slot") & 255;
        std::shared_ptr<ItemStack> itemstack = ItemStack::loadItemStackFromNBT(*nbttagcompound);
        if (itemstack) {
            if (j >= 0 && j < static_cast<int>(mainInventory.size()))
                mainInventory[static_cast<size_t>(j)] = itemstack;
            if (j >= 100 && j < static_cast<int>(armorInventory.size()) + 100)
                armorInventory[static_cast<size_t>(j - 100)] = itemstack;
        }
    }
}

std::shared_ptr<ItemStack> InventoryPlayer::getStackInSlot(int index) const
{
    return const_cast<InventoryPlayer*>(this)->slotRef(index);
}

std::shared_ptr<IChatComponent> InventoryPlayer::getDisplayName() const
{
    if (hasCustomName())
        return std::make_shared<ChatComponentText>(getName());
    return std::make_shared<ChatComponentTranslation>(getName());
}

bool InventoryPlayer::canHeldItemHarvest(const Block* blockIn) const
{
    if (blockIn->getMaterial().isToolNotRequired())
        return true;
    const std::shared_ptr<ItemStack> itemstack = getStackInSlot(currentItem);
    return itemstack ? itemstack->canHarvestBlock(blockIn) : false;
}

int InventoryPlayer::getTotalArmorValue() const
{
    int i = 0;
    for (const auto& stack : armorInventory) {
        if (stack) {
            if (const auto* armor = dynamic_cast<const ItemArmor*>(stack->getItem()))
                i += armor->damageReduceAmount;
        }
    }
    return i;
}

void InventoryPlayer::damageArmor(float damage)
{
    damage = damage / 4.0f;
    if (damage < 1.0f)
        damage = 1.0f;
    for (auto& stack : armorInventory) {
        if (stack && dynamic_cast<const ItemArmor*>(stack->getItem())) {
            damageItem(*stack, static_cast<int>(damage), player);
            if (stack->stackSize == 0)
                stack = nullptr;
        }
    }
}

void InventoryPlayer::dropAllItems()
{
    for (auto& stack : mainInventory) {
        if (stack) {
            player->dropItem(stack, true, false);
            stack = nullptr;
        }
    }
    for (auto& stack : armorInventory) {
        if (stack) {
            player->dropItem(stack, true, false);
            stack = nullptr;
        }
    }
}

bool InventoryPlayer::isUseableByPlayer(EntityPlayer* playerIn)
{
    return player->isDead ? false : playerIn->getDistanceSqToEntity(player) <= 64.0;
}

bool InventoryPlayer::hasItemStack(const ItemStack* itemStackIn) const
{
    for (const auto& stack : armorInventory) {
        if (stack && stack->isItemEqual(itemStackIn))
            return true;
    }
    for (const auto& stack : mainInventory) {
        if (stack && stack->isItemEqual(itemStackIn))
            return true;
    }
    return false;
}

void InventoryPlayer::copyInventory(const InventoryPlayer& playerInventory)
{
    for (size_t i = 0; i < mainInventory.size(); ++i)
        mainInventory[i] = copyItemStack(playerInventory.mainInventory[i]);
    for (size_t j = 0; j < armorInventory.size(); ++j)
        armorInventory[j] = copyItemStack(playerInventory.armorInventory[j]);
    currentItem = playerInventory.currentItem;
}

void InventoryPlayer::clear()
{
    mainInventory = {};
    armorInventory = {};
}

} // namespace mc

#include "tileentity/TileEntityBrewingStand.h"

#include "block/BlockBrewingStand.h"
#include "init/Items.h"
#include "item/ItemPotion.h"
#include "item/ItemStack.h"
#include "nbt/NBTTagCompound.h"
#include "potion/PotionHelper.h"
#include "world/World.h"

namespace mc {

namespace {

// Java compares the cached effect lists of ItemPotion.getEffects by reference; a list without effects is null
// there and empty here, so two empty lists count as the same null.
bool sameList(const std::vector<PotionEffect>& a, const std::vector<PotionEffect>& b)
{
    return (a.empty() && b.empty()) || &a == &b;
}

// Java: list.equals(list1) for a non-null list -- false against null.
bool equalLists(const std::vector<PotionEffect>& a, const std::vector<PotionEffect>& b)
{
    return !b.empty() && a == b;
}

} // namespace

void TileEntityBrewingStand::update()
{
    if (brewTime > 0) {
        --brewTime;
        if (brewTime == 0) {
            brewPotions();
            markDirty();
        } else if (!canBrew()) {
            brewTime = 0;
            markDirty();
        } else if (ingredientID != brewingItemStacks[3]->getItem()) {
            brewTime = 0;
            markDirty();
        }
    } else if (canBrew()) {
        brewTime = 400;
        ingredientID = brewingItemStacks[3]->getItem();
    }
    if (!worldObj->isRemote) {
        const std::array<bool, 3> aboolean = func_174902_m();
        if (!filledSlots || aboolean != *filledSlots) {
            filledSlots = aboolean;
            const IBlockState* iblockstate = worldObj->getBlockState(getPos());
            if (!dynamic_cast<const BlockBrewingStand*>(iblockstate->getBlock()))
                return;
            for (size_t i = 0; i < BlockBrewingStand::HAS_BOTTLE.size(); ++i)
                iblockstate = iblockstate->withProperty(BlockBrewingStand::HAS_BOTTLE[i], aboolean[i]);
            worldObj->setBlockState(pos, iblockstate, 2);
        }
    }
}

bool TileEntityBrewingStand::canBrew()
{
    if (brewingItemStacks[3] && brewingItemStacks[3]->stackSize > 0) {
        const ItemStack& itemstack = *brewingItemStacks[3];
        if (!itemstack.getItem()->isPotionIngredient(itemstack))
            return false;
        bool flag = false;
        for (size_t i = 0; i < 3; ++i) {
            if (brewingItemStacks[i] && brewingItemStacks[i]->getItem() == Items::potionitem) {
                const int j = brewingItemStacks[i]->getMetadata();
                const int k = getPotionResult(j, &itemstack);
                if (!ItemPotion::isSplash(j) && ItemPotion::isSplash(k)) {
                    flag = true;
                    break;
                }
                const std::vector<PotionEffect>& list = Items::potionitem->getEffects(j);
                const std::vector<PotionEffect>& list1 = Items::potionitem->getEffects(k);
                if ((j <= 0 || !sameList(list, list1)) && (list.empty() || (!equalLists(list, list1) && !list1.empty())) && j != k) {
                    flag = true;
                    break;
                }
            }
        }
        return flag;
    }
    return false;
}

void TileEntityBrewingStand::brewPotions()
{
    if (canBrew()) {
        const ItemStack& itemstack = *brewingItemStacks[3];
        for (size_t i = 0; i < 3; ++i) {
            if (brewingItemStacks[i] && brewingItemStacks[i]->getItem() == Items::potionitem) {
                const int j = brewingItemStacks[i]->getMetadata();
                const int k = getPotionResult(j, &itemstack);
                const std::vector<PotionEffect>& list = Items::potionitem->getEffects(j);
                const std::vector<PotionEffect>& list1 = Items::potionitem->getEffects(k);
                if ((j > 0 && sameList(list, list1)) || (!list.empty() && (equalLists(list, list1) || list1.empty()))) {
                    if (!ItemPotion::isSplash(j) && ItemPotion::isSplash(k))
                        brewingItemStacks[i]->setItemDamage(k);
                } else if (j != k) {
                    brewingItemStacks[i]->setItemDamage(k);
                }
            }
        }
        if (itemstack.getItem()->hasContainerItem()) {
            brewingItemStacks[3] = std::make_shared<ItemStack>(itemstack.getItem()->getContainerItem());
        } else {
            --brewingItemStacks[3]->stackSize;
            if (brewingItemStacks[3]->stackSize <= 0)
                brewingItemStacks[3] = nullptr;
        }
    }
}

int TileEntityBrewingStand::getPotionResult(int meta, const ItemStack* stack)
{
    return stack == nullptr ? meta
                            : (stack->getItem()->isPotionIngredient(*stack) ? PotionHelper::applyIngredient(meta, *stack->getItem()->getPotionEffect(*stack)) : meta);
}

void TileEntityBrewingStand::readFromNBT(const NBTTagCompound& compound)
{
    TileEntityLockable::readFromNBT(compound);
    const auto nbttaglist = compound.getTagList("Items", 10);
    brewingItemStacks.fill(nullptr);
    for (int i = 0; i < nbttaglist->tagCount(); ++i) {
        const auto nbttagcompound = nbttaglist->getCompoundTagAt(i);
        const int j = nbttagcompound->getByte("Slot");
        if (j >= 0 && j < static_cast<int>(brewingItemStacks.size())) {
            auto loaded = ItemStack::loadItemStackFromNBT(*nbttagcompound);
            brewingItemStacks[static_cast<size_t>(j)] = loaded ? std::shared_ptr<ItemStack>(std::move(loaded)) : nullptr;
        }
    }
    brewTime = compound.getShort("BrewTime");
    if (compound.hasKey("CustomName", 8))
        customName = compound.getString("CustomName");
}

void TileEntityBrewingStand::writeToNBT(NBTTagCompound& compound) const
{
    TileEntityLockable::writeToNBT(compound);
    compound.setShort("BrewTime", static_cast<int16_t>(brewTime));
    auto nbttaglist = std::make_shared<NBTTagList>();
    for (size_t i = 0; i < brewingItemStacks.size(); ++i) {
        if (brewingItemStacks[i]) {
            auto nbttagcompound = std::make_shared<NBTTagCompound>();
            nbttagcompound->setByte("Slot", static_cast<int8_t>(i));
            brewingItemStacks[i]->writeToNBT(*nbttagcompound);
            nbttaglist->appendTag(nbttagcompound);
        }
    }
    compound.setTag("Items", nbttaglist);
    if (hasCustomName())
        compound.setString("CustomName", customName);
}

std::shared_ptr<ItemStack> TileEntityBrewingStand::getStackInSlot(int index) const
{
    return index >= 0 && index < static_cast<int>(brewingItemStacks.size()) ? brewingItemStacks[static_cast<size_t>(index)] : nullptr;
}

std::shared_ptr<ItemStack> TileEntityBrewingStand::decrStackSize(int index, int)
{
    if (index >= 0 && index < static_cast<int>(brewingItemStacks.size())) {
        std::shared_ptr<ItemStack> itemstack = std::move(brewingItemStacks[static_cast<size_t>(index)]);
        brewingItemStacks[static_cast<size_t>(index)] = nullptr;
        return itemstack;
    }
    return nullptr;
}

std::shared_ptr<ItemStack> TileEntityBrewingStand::removeStackFromSlot(int index)
{
    return decrStackSize(index, 0);
}

void TileEntityBrewingStand::setInventorySlotContents(int index, std::shared_ptr<ItemStack> stack)
{
    if (index >= 0 && index < static_cast<int>(brewingItemStacks.size()))
        brewingItemStacks[static_cast<size_t>(index)] = std::move(stack);
}

bool TileEntityBrewingStand::isUseableByPlayer(EntityPlayer*)
{
    // Java: and the player within 8 blocks (stage 10).
    return worldObj->getTileEntity(pos) == this;
}

bool TileEntityBrewingStand::isItemValidForSlot(int index, const ItemStack& stack) const
{
    return index == 3 ? stack.getItem()->isPotionIngredient(stack) : stack.getItem() == Items::potionitem || stack.getItem() == Items::glass_bottle;
}

std::array<bool, 3> TileEntityBrewingStand::func_174902_m() const
{
    std::array<bool, 3> aboolean {};
    for (size_t i = 0; i < 3; ++i) {
        if (brewingItemStacks[i])
            aboolean[i] = true;
    }
    return aboolean;
}

std::vector<int> TileEntityBrewingStand::getSlotsForFace(EnumFacing side)
{
    return side == EnumFacing::UP ? std::vector<int> { 3 } : std::vector<int> { 0, 1, 2 };
}

bool TileEntityBrewingStand::canInsertItem(int index, const ItemStack& itemStackIn, std::optional<EnumFacing>)
{
    return isItemValidForSlot(index, itemStackIn);
}

} // namespace mc

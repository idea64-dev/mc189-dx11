#include "item/ItemStack.h"

#include "block/Block.h"
#include "entity/player/EntityPlayer.h"
#include "nbt/NBTTagCompound.h"

namespace mc {

ItemStack::ItemStack(const Block* block, int amount, int meta)
    : ItemStack(Item::getItemFromBlock(block), amount, meta)
{
}

ItemStack::ItemStack(Item* itemIn, int amount, int meta)
    : stackSize(amount)
    , item(itemIn)
    , itemDamage(meta < 0 ? 0 : meta)
{
}

std::unique_ptr<ItemStack> ItemStack::loadItemStackFromNBT(const NBTTagCompound& nbt)
{
    std::unique_ptr<ItemStack> itemstack(new ItemStack());
    itemstack->readFromNBT(nbt);
    return itemstack->getItem() ? std::move(itemstack) : nullptr;
}

ItemStack ItemStack::splitStack(int amount)
{
    ItemStack itemstack(item, amount, itemDamage);
    if (stackTagCompound)
        itemstack.stackTagCompound = std::static_pointer_cast<NBTTagCompound>(stackTagCompound->copy());
    stackSize -= amount;
    return itemstack;
}

NBTTagCompound& ItemStack::writeToNBT(NBTTagCompound& nbt) const
{
    const auto location = Item::itemRegistry.getNameForObject(item);
    nbt.setString("id", location ? location->toString() : "minecraft:air");
    nbt.setByte("Count", static_cast<int8_t>(stackSize));
    nbt.setShort("Damage", static_cast<int16_t>(itemDamage));
    if (stackTagCompound)
        nbt.setTag("tag", stackTagCompound);
    return nbt;
}

void ItemStack::readFromNBT(const NBTTagCompound& nbt)
{
    if (nbt.hasKey("id", 8))
        item = Item::getByNameOrId(nbt.getString("id"));
    else
        item = Item::getItemById(nbt.getShort("id"));
    stackSize = nbt.getByte("Count");
    itemDamage = nbt.getShort("Damage");
    if (itemDamage < 0)
        itemDamage = 0;
    if (nbt.hasKey("tag", 10)) {
        stackTagCompound = nbt.getCompoundTag("tag");
        if (item)
            item->updateItemStackNBT(*stackTagCompound);
    }
}

bool ItemStack::isItemStackDamageable() const
{
    if (!item || item->getMaxDamage() <= 0)
        return false;
    return !hasTagCompound() || !stackTagCompound->getBoolean("Unbreakable");
}

ItemStack ItemStack::copy() const
{
    ItemStack itemstack(item, stackSize, itemDamage);
    if (stackTagCompound)
        itemstack.stackTagCompound = std::static_pointer_cast<NBTTagCompound>(stackTagCompound->copy());
    return itemstack;
}

namespace {

// stackA's tag null and stackB's not: unequal; stackA's null (both null): equal; else equals().
bool tagsEqual(const std::shared_ptr<NBTTagCompound>& a, const std::shared_ptr<NBTTagCompound>& b)
{
    if (!a && b)
        return false;
    return !a || (b && a->equals(*b));
}

} // namespace

bool ItemStack::areItemStackTagsEqual(const ItemStack* stackA, const ItemStack* stackB)
{
    if (!stackA && !stackB)
        return true;
    return stackA && stackB && tagsEqual(stackA->stackTagCompound, stackB->stackTagCompound);
}

bool ItemStack::areItemStacksEqual(const ItemStack* stackA, const ItemStack* stackB)
{
    if (!stackA && !stackB)
        return true;
    return stackA && stackB && stackA->isItemStackEqual(*stackB);
}

bool ItemStack::isItemStackEqual(const ItemStack& other) const
{
    return stackSize == other.stackSize && item == other.item && itemDamage == other.itemDamage && tagsEqual(stackTagCompound, other.stackTagCompound);
}

bool ItemStack::areItemsEqual(const ItemStack* stackA, const ItemStack* stackB)
{
    if (!stackA && !stackB)
        return true;
    return stackA && stackB && stackA->isItemEqual(stackB);
}

std::string ItemStack::toString() const
{
    return std::to_string(stackSize) + "x" + item->getUnlocalizedName() + "@" + std::to_string(itemDamage);
}

std::shared_ptr<NBTTagCompound> ItemStack::getSubCompound(const std::string& key, bool create)
{
    if (stackTagCompound && stackTagCompound->hasKey(key, 10))
        return stackTagCompound->getCompoundTag(key);
    if (create) {
        auto nbttagcompound = std::make_shared<NBTTagCompound>();
        setTagInfo(key, nbttagcompound);
        return nbttagcompound;
    }
    return nullptr;
}

std::shared_ptr<NBTTagList> ItemStack::getEnchantmentTagList() const
{
    return stackTagCompound ? stackTagCompound->getTagList("ench", 10) : nullptr;
}

std::string ItemStack::getDisplayName() const
{
    std::string s = item->getItemStackDisplayName(*this);
    if (stackTagCompound && stackTagCompound->hasKey("display", 10)) {
        const auto nbttagcompound = stackTagCompound->getCompoundTag("display");
        if (nbttagcompound->hasKey("Name", 8))
            s = nbttagcompound->getString("Name");
    }
    return s;
}

ItemStack& ItemStack::setStackDisplayName(const std::string& displayName)
{
    if (!stackTagCompound)
        stackTagCompound = std::make_shared<NBTTagCompound>();
    if (!stackTagCompound->hasKey("display", 10))
        stackTagCompound->setTag("display", std::make_shared<NBTTagCompound>());
    stackTagCompound->getCompoundTag("display")->setString("Name", displayName);
    return *this;
}

void ItemStack::clearCustomName()
{
    if (stackTagCompound && stackTagCompound->hasKey("display", 10)) {
        const auto nbttagcompound = stackTagCompound->getCompoundTag("display");
        nbttagcompound->removeTag("Name");
        if (nbttagcompound->hasNoTags()) {
            stackTagCompound->removeTag("display");
            if (stackTagCompound->hasNoTags())
                setTagCompound(nullptr);
        }
    }
}

bool ItemStack::hasDisplayName() const
{
    return stackTagCompound && stackTagCompound->hasKey("display", 10) && stackTagCompound->getCompoundTag("display")->hasKey("Name", 8);
}

void ItemStack::addEnchantment(int effectId, int level)
{
    if (!stackTagCompound)
        setTagCompound(std::make_shared<NBTTagCompound>());
    if (!stackTagCompound->hasKey("ench", 9))
        stackTagCompound->setTag("ench", std::make_shared<NBTTagList>());
    const auto nbttaglist = stackTagCompound->getTagList("ench", 10);
    auto nbttagcompound = std::make_shared<NBTTagCompound>();
    nbttagcompound->setShort("id", static_cast<int16_t>(effectId));
    nbttagcompound->setShort("lvl", static_cast<int16_t>(static_cast<int8_t>(level)));
    nbttaglist->appendTag(nbttagcompound);
}

bool ItemStack::isItemEnchanted() const
{
    return stackTagCompound && stackTagCompound->hasKey("ench", 9);
}

void ItemStack::setTagInfo(const std::string& key, std::shared_ptr<NBTBase> value)
{
    if (!stackTagCompound)
        setTagCompound(std::make_shared<NBTTagCompound>());
    stackTagCompound->setTag(key, std::move(value));
}

int ItemStack::getRepairCost() const
{
    return hasTagCompound() && stackTagCompound->hasKey("RepairCost", 3) ? stackTagCompound->getInteger("RepairCost") : 0;
}

void ItemStack::setRepairCost(int cost)
{
    if (!hasTagCompound())
        stackTagCompound = std::make_shared<NBTTagCompound>();
    stackTagCompound->setInteger("RepairCost", cost);
}

bool ItemStack::canDestroy(const Block* block)
{
    if (block == canDestroyCacheBlock)
        return canDestroyCacheResult;
    canDestroyCacheBlock = block;
    if (hasTagCompound() && stackTagCompound->hasKey("CanDestroy", 9)) {
        const auto nbttaglist = stackTagCompound->getTagList("CanDestroy", 8);
        for (int i = 0; i < nbttaglist->tagCount(); ++i) {
            if (Block::getBlockFromName(nbttaglist->getStringTagAt(i)) == block) {
                canDestroyCacheResult = true;
                return true;
            }
        }
    }
    canDestroyCacheResult = false;
    return false;
}

bool ItemStack::canPlaceOn(const Block* block)
{
    if (block == canPlaceOnCacheBlock)
        return canPlaceOnCacheResult;
    canPlaceOnCacheBlock = block;
    if (hasTagCompound() && stackTagCompound->hasKey("CanPlaceOn", 9)) {
        const auto nbttaglist = stackTagCompound->getTagList("CanPlaceOn", 8);
        for (int i = 0; i < nbttaglist->tagCount(); ++i) {
            if (Block::getBlockFromName(nbttaglist->getStringTagAt(i)) == block) {
                canPlaceOnCacheResult = true;
                return true;
            }
        }
    }
    canPlaceOnCacheResult = false;
    return false;
}

std::shared_ptr<ItemStack> ItemStack::copyItemStack(const ItemStack* stack)
{
    return stack ? std::make_shared<ItemStack>(stack->copy()) : nullptr;
}

bool ItemStack::onItemUse(EntityPlayer* playerIn, World* worldIn, const BlockPos& pos, EnumFacing side, float hitX, float hitY, float hitZ)
{
    // Java: a success adds to StatList.objectUseStats -- the statistics, stage 12.
    return getItem()->onItemUse(*this, playerIn, worldIn, pos, side, hitX, hitY, hitZ);
}

std::shared_ptr<ItemStack> ItemStack::useItemRightClick(World* worldIn, EntityPlayer* playerIn)
{
    return useItemRightClick(shared_from_this(), worldIn, playerIn);
}

std::shared_ptr<ItemStack> ItemStack::useItemRightClick(const std::shared_ptr<ItemStack>& stack, World* worldIn, EntityPlayer* playerIn)
{
    return stack->getItem()->onItemRightClick(stack, worldIn, playerIn);
}

std::shared_ptr<ItemStack> ItemStack::onItemUseFinish(World* worldIn, EntityPlayer* playerIn)
{
    return onItemUseFinish(shared_from_this(), worldIn, playerIn);
}

std::shared_ptr<ItemStack> ItemStack::onItemUseFinish(const std::shared_ptr<ItemStack>& stack, World* worldIn, EntityPlayer* playerIn)
{
    return stack->getItem()->onItemUseFinish(stack, worldIn, playerIn);
}

void ItemStack::hitEntity(EntityLivingBase* entityIn, EntityPlayer* playerIn)
{
    // Java: a hit adds to StatList.objectUseStats -- stage 12.
    item->hitEntity(*this, entityIn, playerIn);
}

void ItemStack::onBlockDestroyed(World* worldIn, Block* blockIn, const BlockPos& pos, EntityPlayer* playerIn)
{
    // Java: a success adds to StatList.objectUseStats -- stage 12.
    item->onBlockDestroyed(*this, worldIn, blockIn, pos, playerIn);
}

bool ItemStack::interactWithEntity(EntityPlayer* playerIn, EntityLivingBase* entityIn)
{
    return item->itemInteractionForEntity(*this, playerIn, entityIn);
}

void ItemStack::updateAnimation(World* worldIn, Entity* entityIn, int inventorySlot, bool isCurrentItem)
{
    if (animationsToGo > 0)
        --animationsToGo;
    item->onUpdate(*this, worldIn, entityIn, inventorySlot, isCurrentItem);
}

void ItemStack::onCrafting(World* worldIn, EntityPlayer* playerIn, int)
{
    // Java: addStat(StatList.objectCraftStats[...], amount) -- stage 12.
    item->onCreated(*this, worldIn, playerIn);
}

void ItemStack::onPlayerStoppedUsing(World* worldIn, EntityPlayer* playerIn, int timeLeft)
{
    getItem()->onPlayerStoppedUsing(*this, worldIn, playerIn, timeLeft);
}

} // namespace mc

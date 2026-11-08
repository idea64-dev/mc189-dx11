#pragma once

// Original: net/minecraft/item/ItemStack.java
// Java passes stacks by reference and uses null for "no stack". Here an ItemStack is a movable, non-copyable
// object (copy() makes the deep copy Java's copy() does); holders that need null or sharing use
// std::shared_ptr<ItemStack> / nullptr. The tag compound is shared like Java's reference: getTagCompound()
// hands out the stack's own compound.
// Stacks held through a shared pointer can return themselves from useItemRightClick / onItemUseFinish
// (enable_shared_from_this), so callers compare the result with the stack they passed as Java does.
// damageItem, attemptDamageItem and the attribute modifiers are in entity/EntityItemHooks.h.
// Not ported yet: getTooltip, item frames and getChatComponent. The use statistics (StatList.objectUseStats)
// come with stage 12.

#include "item/Item.h"

#include <memory>
#include <string>

namespace mc {

class Entity;
class EntityLivingBase;
class EntityPlayer;
class World;
struct BlockPos;
enum class EnumFacing;
class NBTTagCompound;
class NBTTagList;
class NBTBase;

class ItemStack : public std::enable_shared_from_this<ItemStack> {
public:
    explicit ItemStack(const Block* block, int amount = 1, int meta = 0);
    explicit ItemStack(Item* item, int amount = 1, int meta = 0);
    ItemStack(ItemStack&&) = default;
    ItemStack& operator=(ItemStack&&) = default;
    ItemStack(const ItemStack&) = delete;
    ItemStack& operator=(const ItemStack&) = delete;

    // Java returns null when the id names no item.
    static std::unique_ptr<ItemStack> loadItemStackFromNBT(const NBTTagCompound& nbt);

    ItemStack splitStack(int amount);
    Item* getItem() const { return item; }
    float getStrVsBlock(const Block* block) const { return item->getStrVsBlock(*this, block); }
    NBTTagCompound& writeToNBT(NBTTagCompound& nbt) const;
    void readFromNBT(const NBTTagCompound& nbt);
    int getMaxStackSize() const { return item->getItemStackLimit(); }
    bool isStackable() const { return getMaxStackSize() > 1 && (!isItemStackDamageable() || !isItemDamaged()); }
    bool isItemStackDamageable() const;
    bool getHasSubtypes() const { return item->getHasSubtypes(); }
    bool isItemDamaged() const { return isItemStackDamageable() && itemDamage > 0; }
    int getItemDamage() const { return itemDamage; }
    int getMetadata() const { return itemDamage; }
    void setItemDamage(int meta) { itemDamage = meta < 0 ? 0 : meta; }
    int getMaxDamage() const { return item->getMaxDamage(); }
    bool canHarvestBlock(const Block* block) const { return item->canHarvestBlock(block); }
    ItemStack copy() const;
    // Null-tolerant comparisons, as the Java statics.
    static bool areItemStackTagsEqual(const ItemStack* stackA, const ItemStack* stackB);
    static bool areItemStacksEqual(const ItemStack* stackA, const ItemStack* stackB);
    static bool areItemsEqual(const ItemStack* stackA, const ItemStack* stackB);
    bool isItemEqual(const ItemStack* other) const { return other && item == other->item && itemDamage == other->itemDamage; }
    std::string getUnlocalizedName() const { return item->getUnlocalizedName(*this); }
    std::string toString() const;
    bool getIsItemStackEqual(const ItemStack& other) const { return isItemStackEqual(other); }
    int getMaxItemUseDuration() const { return item->getMaxItemUseDuration(*this); }
    EnumAction getItemUseAction() const { return item->getItemUseAction(*this); }
    bool hasTagCompound() const { return stackTagCompound != nullptr; }
    const std::shared_ptr<NBTTagCompound>& getTagCompound() const { return stackTagCompound; }
    // Null when absent and create is false.
    std::shared_ptr<NBTTagCompound> getSubCompound(const std::string& key, bool create);
    std::shared_ptr<NBTTagList> getEnchantmentTagList() const;
    void setTagCompound(std::shared_ptr<NBTTagCompound> nbt) { stackTagCompound = std::move(nbt); }
    std::string getDisplayName() const;
    ItemStack& setStackDisplayName(const std::string& displayName);
    void clearCustomName();
    bool hasDisplayName() const;
    bool hasEffect() const { return item->hasEffect(*this); }
    EnumRarity getRarity() const { return item->getRarity(*this); }
    bool isItemEnchantable() const { return item->isItemTool(*this) && !isItemEnchanted(); }
    // Java: addEnchantment(Enchantment, level) -- with enchantments (stage 12).
    void addEnchantment(int effectId, int level);
    bool isItemEnchanted() const;
    void setTagInfo(const std::string& key, std::shared_ptr<NBTBase> value);
    bool canEditBlocks() const { return item->canItemEditBlocks(); }
    int getRepairCost() const;
    void setRepairCost(int cost);
    void setItem(Item* newItem) { item = newItem; }
    bool canDestroy(const Block* block);
    bool canPlaceOn(const Block* block);

    // Java: copyItemStack -- null for null.
    static std::shared_ptr<ItemStack> copyItemStack(const ItemStack* stack);
    bool onItemUse(EntityPlayer* playerIn, World* worldIn, const BlockPos& pos, EnumFacing side, float hitX, float hitY, float hitZ);
    // The stack must be held by a shared pointer (the result may be this stack).
    std::shared_ptr<ItemStack> useItemRightClick(World* worldIn, EntityPlayer* playerIn);
    static std::shared_ptr<ItemStack> useItemRightClick(const std::shared_ptr<ItemStack>& stack, World* worldIn, EntityPlayer* playerIn);
    std::shared_ptr<ItemStack> onItemUseFinish(World* worldIn, EntityPlayer* playerIn);
    static std::shared_ptr<ItemStack> onItemUseFinish(const std::shared_ptr<ItemStack>& stack, World* worldIn, EntityPlayer* playerIn);
    void hitEntity(EntityLivingBase* entityIn, EntityPlayer* playerIn);
    void onBlockDestroyed(World* worldIn, Block* blockIn, const BlockPos& pos, EntityPlayer* playerIn);
    bool interactWithEntity(EntityPlayer* playerIn, EntityLivingBase* entityIn);
    void updateAnimation(World* worldIn, Entity* entityIn, int inventorySlot, bool isCurrentItem);
    void onCrafting(World* worldIn, EntityPlayer* playerIn, int amount);
    void onPlayerStoppedUsing(World* worldIn, EntityPlayer* playerIn, int timeLeft);

    int stackSize = 0;
    int animationsToGo = 0;

private:
    ItemStack() = default;
    bool isItemStackEqual(const ItemStack& other) const;

    Item* item = nullptr;
    std::shared_ptr<NBTTagCompound> stackTagCompound;
    int itemDamage = 0;
    const Block* canDestroyCacheBlock = nullptr;
    bool canDestroyCacheResult = false;
    const Block* canPlaceOnCacheBlock = nullptr;
    bool canPlaceOnCacheResult = false;
};

} // namespace mc

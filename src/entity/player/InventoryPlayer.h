#pragma once

// Original: net/minecraft/entity/player/InventoryPlayer.java
// Stacks are shared like Java references (slots, containers and the cursor stack may hold the same stack).

#include "tileentity/TileEntity.h"

#include <array>
#include <memory>
#include <string>

namespace mc {

class Block;
class EntityPlayer;
class IChatComponent;
class Item;
class ItemStack;
class NBTTagCompound;
class NBTTagList;

class InventoryPlayer : public IInventory {
public:
    explicit InventoryPlayer(EntityPlayer* playerIn) : player(playerIn) { }

    // Java code reaches the inventory through a reference; callers here use both player->inventory.x and
    // player->inventory->x (and *player->inventory), so the value member also reads like a pointer.
    InventoryPlayer* operator->() { return this; }
    const InventoryPlayer* operator->() const { return this; }
    InventoryPlayer& operator*() { return *this; }
    const InventoryPlayer& operator*() const { return *this; }

    std::shared_ptr<ItemStack> getCurrentItem() const;
    static int getHotbarSize() { return 9; }
    int getFirstEmptyStack() const;
    void setCurrentItem(Item* itemIn, int metadataIn, bool isMetaSpecific, bool p_146030_4_);
    void changeCurrentItem(int direction);
    int clearMatchingItems(Item* itemIn, int metadataIn, int removeCount, const NBTTagCompound* itemNBT);
    void decrementAnimations();
    bool consumeInventoryItem(Item* itemIn);
    bool hasItem(Item* itemIn) const;
    bool addItemStackToInventory(const std::shared_ptr<ItemStack>& itemStackIn);
    float getStrVsBlock(const Block* blockIn) const;
    NBTTagList& writeToNBT(NBTTagList& nbtTagListIn) const;
    void readFromNBT(const NBTTagList& nbtTagListIn);
    bool canHeldItemHarvest(const Block* blockIn) const;
    std::shared_ptr<ItemStack> armorItemInSlot(int slotIn) const { return armorInventory[static_cast<size_t>(slotIn)]; }
    int getTotalArmorValue() const;
    void damageArmor(float damage);
    void dropAllItems();
    void setItemStack(std::shared_ptr<ItemStack> itemStackIn) { itemStack = std::move(itemStackIn); }
    const std::shared_ptr<ItemStack>& getItemStack() const { return itemStack; }
    bool hasItemStack(const ItemStack* itemStackIn) const;
    void copyInventory(const InventoryPlayer& playerInventory);
    std::shared_ptr<IChatComponent> getDisplayName() const;

    // IInventory
    int getSizeInventory() const override { return static_cast<int>(mainInventory.size()) + 4; }
    std::shared_ptr<ItemStack> getStackInSlot(int index) const override;
    std::shared_ptr<ItemStack> decrStackSize(int index, int count) override;
    std::shared_ptr<ItemStack> removeStackFromSlot(int index) override;
    void setInventorySlotContents(int index, std::shared_ptr<ItemStack> stack) override;
    int getInventoryStackLimit() const override { return 64; }
    void markDirty() override { inventoryChanged = true; }
    bool isUseableByPlayer(EntityPlayer* playerIn) override;
    void openInventory(EntityPlayer*) override { }
    void closeInventory(EntityPlayer*) override { }
    bool isItemValidForSlot(int, const ItemStack&) const override { return true; }
    int getField(int) const override { return 0; }
    void setField(int, int) override { }
    int getFieldCount() const override { return 0; }
    void clear() override;
    std::string getName() const override { return "container.inventory"; }
    bool hasCustomName() const override { return false; }

    std::array<std::shared_ptr<ItemStack>, 36> mainInventory {};
    std::array<std::shared_ptr<ItemStack>, 4> armorInventory {};
    int currentItem = 0;
    EntityPlayer* player;
    bool inventoryChanged = false;

private:
    int getInventorySlotContainItem(const Item* itemIn) const;
    int getInventorySlotContainItemAndDamage(const Item* itemIn, int metadataIn) const;
    int storeItemStack(const ItemStack& itemStackIn) const;
    int storePartialItemStack(const ItemStack& itemStackIn);
    // Java: the array (mainInventory or armorInventory) an index falls in, and the index within it.
    std::shared_ptr<ItemStack>& slotRef(int index);

    std::shared_ptr<ItemStack> itemStack;
};

} // namespace mc

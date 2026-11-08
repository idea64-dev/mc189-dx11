#pragma once

// Original: net/minecraft/tileentity/TileEntityHopper.java (with IHopper) -- moves items between inventories.

#include "tileentity/TileEntity.h"
#include "util/EnumFacing.h"

#include <array>
#include <memory>
#include <optional>
#include <string>

namespace mc {

class World;

class TileEntityHopper : public TileEntityLockable {
public:
    void readFromNBT(const NBTTagCompound& compound) override;
    void writeToNBT(NBTTagCompound& compound) const override;
    void markDirty() override { TileEntityLockable::markDirty(); }
    int getSizeInventory() const override { return static_cast<int>(inventory.size()); }
    std::shared_ptr<ItemStack> getStackInSlot(int index) const override { return inventory.at(static_cast<size_t>(index)); }
    std::shared_ptr<ItemStack> decrStackSize(int index, int count) override;
    std::shared_ptr<ItemStack> removeStackFromSlot(int index) override;
    void setInventorySlotContents(int index, std::shared_ptr<ItemStack> stack) override;
    std::string getName() const override { return hasCustomName() ? customName : "container.hopper"; }
    bool hasCustomName() const override { return !customName.empty(); }
    void setCustomName(std::string customNameIn) { customName = std::move(customNameIn); }
    int getInventoryStackLimit() const override { return 64; }
    bool isUseableByPlayer(EntityPlayer* player) override;
    void openInventory(EntityPlayer*) override { }
    void closeInventory(EntityPlayer*) override { }
    bool isItemValidForSlot(int, const ItemStack&) const override { return true; }
    bool isTickable() const override { return true; }
    void update() override;
    bool updateHopper();
    // Java: captureDroppedItems(IHopper) -- pulls from the inventory above (item entities come with stage 10).
    static bool captureDroppedItems(TileEntityHopper* hopper);
    // The remainder of the stack, null when everything went in.
    static std::shared_ptr<ItemStack> putStackInInventoryAllSlots(IInventory* inventoryIn, std::shared_ptr<ItemStack> stack, std::optional<EnumFacing> side);
    static std::shared_ptr<IInventory> getHopperInventory(TileEntityHopper* hopper);
    // Java returns the TileEntity, the double chest or an entity inventory at the position; null if none.
    static std::shared_ptr<IInventory> getInventoryAtPosition(World* worldIn, double x, double y, double z);
    World* getWorld() const { return worldObj; }
    double getXPos() const { return pos.getX() + 0.5; }
    double getYPos() const { return pos.getY() + 0.5; }
    double getZPos() const { return pos.getZ() + 0.5; }
    void setTransferCooldown(int ticks) { transferCooldown = ticks; }
    bool isOnTransferCooldown() const { return transferCooldown > 0; }
    bool mayTransfer() const { return transferCooldown <= 1; }
    int getField(int) const override { return 0; }
    void setField(int, int) override { }
    int getFieldCount() const override { return 0; }
    void clear() override { inventory.fill(nullptr); }
    std::string_view getMappingName() const override { return "Hopper"; }

private:
    bool isEmpty() const;
    bool isFull() const;
    bool transferItemsOut();
    bool isInventoryFull(IInventory* inventoryIn, EnumFacing side);
    static bool isInventoryEmpty(IInventory* inventoryIn, EnumFacing side);
    static bool pullItemFromSlot(TileEntityHopper* hopper, IInventory* inventoryIn, int index, EnumFacing direction);
    static bool canInsertItemInSlot(IInventory* inventoryIn, const ItemStack& stack, int index, std::optional<EnumFacing> side);
    static bool canExtractItemFromSlot(IInventory* inventoryIn, const ItemStack& stack, int index, EnumFacing side);
    static std::shared_ptr<ItemStack> insertStack(IInventory* inventoryIn, std::shared_ptr<ItemStack> stack, int index, std::optional<EnumFacing> side);
    std::shared_ptr<IInventory> getInventoryForHopperTransfer();
    static bool canCombine(const ItemStack& stack1, const ItemStack& stack2);

    std::array<std::shared_ptr<ItemStack>, 5> inventory {};
    std::string customName;
    int transferCooldown = -1;
};

// The sided view of an inventory: ISidedInventory, or the Java rules of TileEntityFurnace (which implements
// ISidedInventory in Java but not yet here). Shared with the dropper.
namespace SidedInventory {

bool isSided(IInventory* inventoryIn);
std::vector<int> getSlotsForFace(IInventory* inventoryIn, EnumFacing side);
bool canInsertItem(IInventory* inventoryIn, int index, const ItemStack& stack, std::optional<EnumFacing> direction);
bool canExtractItem(IInventory* inventoryIn, int index, const ItemStack& stack, EnumFacing direction);

} // namespace SidedInventory

} // namespace mc

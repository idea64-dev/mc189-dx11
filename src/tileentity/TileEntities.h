#pragma once

// Original: net/minecraft/tileentity/TileEntityChest.java, TileEntityDispenser.java, TileEntityDropper.java,
// TileEntityFurnace.java, TileEntityMobSpawner.java, MobSpawnerBaseLogic.java and TileEntityFlowerPot.java.
// Ported: the data, NBT and inventory parts. Behaviour that needs entities, players and containers (the
// chest lid and its viewers, smelting, the spawner's spawning, dispensing, GUIs) comes with those stages.

#include "tileentity/TileEntity.h"

#include <array>
#include <optional>
#include <vector>

namespace mc {

class Item;
class NBTTagList;
class TileEntityChest;
enum class EnumFacing;

// The ItemStack[] of an inventory with Java's slot handling.
template <size_t N>
class InventorySlots {
public:
    std::shared_ptr<ItemStack> get(int index) const { return stacks.at(static_cast<size_t>(index)); }
    std::shared_ptr<ItemStack>& at(int index) { return stacks.at(static_cast<size_t>(index)); }
    void clear() { stacks.fill(nullptr); }
    // Java: decrStackSize; markDirty is called by the owner when something was taken.
    std::shared_ptr<ItemStack> decrStackSize(int index, int count, bool& changed);
    std::shared_ptr<ItemStack> removeStackFromSlot(int index);
    // The "Items" list; slot bytes masked with 255 when maskSlot (the furnace reads them signed).
    void readItems(const NBTTagCompound& compound, bool maskSlot);
    void writeItems(NBTTagCompound& compound) const;
    static constexpr int size() { return static_cast<int>(N); }

private:
    std::array<std::shared_ptr<ItemStack>, N> stacks {};
};

class TileEntityChest : public TileEntityLockable {
public:
    TileEntityChest() = default;
    explicit TileEntityChest(int chestType)
        : cachedChestType(chestType)
    {
    }

    int getSizeInventory() const override { return 27; }
    std::shared_ptr<ItemStack> getStackInSlot(int index) const override { return chestContents.get(index); }
    std::shared_ptr<ItemStack> decrStackSize(int index, int count) override;
    std::shared_ptr<ItemStack> removeStackFromSlot(int index) override { return chestContents.removeStackFromSlot(index); }
    void setInventorySlotContents(int index, std::shared_ptr<ItemStack> stack) override;
    std::string getName() const override { return hasCustomName() ? customName : "container.chest"; }
    bool hasCustomName() const override { return !customName.empty(); }
    void setCustomName(std::string name) { customName = std::move(name); }
    void readFromNBT(const NBTTagCompound& compound) override;
    void writeToNBT(NBTTagCompound& compound) const override;
    int getInventoryStackLimit() const override { return 64; }
    bool isUseableByPlayer(EntityPlayer* player) override;
    void updateContainingBlockInfo() override;
    void checkForAdjacentChests();
    // Java: ITickable.update -- the adjacent chest check and the lid; the viewer count needs players.
    bool isTickable() const override { return true; }
    void update() override;
    bool receiveClientEvent(int id, int type) override;
    void openInventory(EntityPlayer* player) override;
    void closeInventory(EntityPlayer* player) override;
    bool isItemValidForSlot(int, const ItemStack&) const override { return true; }
    void invalidate() override;
    int getChestType();
    int getField(int) const override { return 0; }
    void setField(int, int) override { }
    int getFieldCount() const override { return 0; }
    void clear() override { chestContents.clear(); }
    std::string_view getMappingName() const override { return "Chest"; }

    bool adjacentChestChecked = false;
    TileEntityChest* adjacentChestZNeg = nullptr;
    TileEntityChest* adjacentChestXPos = nullptr;
    TileEntityChest* adjacentChestXNeg = nullptr;
    TileEntityChest* adjacentChestZPos = nullptr;
    float lidAngle = 0.0f;
    float prevLidAngle = 0.0f;
    int numPlayersUsing = 0;

protected:
    TileEntityChest* getAdjacentChest(EnumFacing side);

private:
    void func_174910_a(TileEntityChest* chestTe, EnumFacing side);
    bool isChestAt(const BlockPos& posIn);

    InventorySlots<27> chestContents;
    int ticksSinceSync = 0;
    int cachedChestType = -1;
    std::string customName;
};

class TileEntityDispenser : public TileEntityLockable {
public:
    int getSizeInventory() const override { return 9; }
    std::shared_ptr<ItemStack> getStackInSlot(int index) const override { return stacks.get(index); }
    std::shared_ptr<ItemStack> decrStackSize(int index, int count) override;
    std::shared_ptr<ItemStack> removeStackFromSlot(int index) override { return stacks.removeStackFromSlot(index); }
    int getDispenseSlot();
    void setInventorySlotContents(int index, std::shared_ptr<ItemStack> stack) override;
    int addItemStack(std::shared_ptr<ItemStack> stack);
    std::string getName() const override { return hasCustomName() ? *customName : "container.dispenser"; }
    void setCustomName(std::string name) { customName = std::move(name); }
    bool hasCustomName() const override { return customName.has_value(); }
    void readFromNBT(const NBTTagCompound& compound) override;
    void writeToNBT(NBTTagCompound& compound) const override;
    int getInventoryStackLimit() const override { return 64; }
    bool isUseableByPlayer(EntityPlayer* player) override;
    void openInventory(EntityPlayer*) override { }
    void closeInventory(EntityPlayer*) override { }
    bool isItemValidForSlot(int, const ItemStack&) const override { return true; }
    int getField(int) const override { return 0; }
    void setField(int, int) override { }
    int getFieldCount() const override { return 0; }
    void clear() override { stacks.clear(); }
    std::string_view getMappingName() const override { return "Trap"; }

protected:
    std::optional<std::string> customName;

private:
    InventorySlots<9> stacks;
};

class TileEntityDropper : public TileEntityDispenser {
public:
    std::string getName() const override { return hasCustomName() ? *customName : "container.dropper"; }
    std::string_view getMappingName() const override { return "Dropper"; }
};

class TileEntityFurnace : public TileEntityLockable {
public:
    int getSizeInventory() const override { return 3; }
    std::shared_ptr<ItemStack> getStackInSlot(int index) const override { return furnaceItemStacks.get(index); }
    std::shared_ptr<ItemStack> decrStackSize(int index, int count) override;
    std::shared_ptr<ItemStack> removeStackFromSlot(int index) override { return furnaceItemStacks.removeStackFromSlot(index); }
    void setInventorySlotContents(int index, std::shared_ptr<ItemStack> stack) override;
    std::string getName() const override { return hasCustomName() ? furnaceCustomName : "container.furnace"; }
    bool hasCustomName() const override { return !furnaceCustomName.empty(); }
    void setCustomInventoryName(std::string name) { furnaceCustomName = std::move(name); }
    void readFromNBT(const NBTTagCompound& compound) override;
    void writeToNBT(NBTTagCompound& compound) const override;
    int getInventoryStackLimit() const override { return 64; }
    bool isBurning() const { return furnaceBurnTime > 0; }
    // Java: update() -- smelting and BlockFurnace.setState, stage 9 with the block ticks.
    bool isTickable() const override { return true; }
    static int getItemBurnTime(const ItemStack* stack);
    static bool isItemFuel(const ItemStack* stack) { return getItemBurnTime(stack) > 0; }
    bool isUseableByPlayer(EntityPlayer* player) override;
    void openInventory(EntityPlayer*) override { }
    void closeInventory(EntityPlayer*) override { }
    bool isItemValidForSlot(int index, const ItemStack& stack) const override;
    int getField(int id) const override;
    void setField(int id, int value) override;
    int getFieldCount() const override { return 4; }
    void clear() override { furnaceItemStacks.clear(); }
    std::string_view getMappingName() const override { return "Furnace"; }

private:
    InventorySlots<3> furnaceItemStacks;
    int furnaceBurnTime = 0;
    int currentItemBurnTime = 0;
    int cookTime = 0;
    int totalCookTime = 0;
    std::string furnaceCustomName;
};

class MobSpawnerBaseLogic {
public:
    struct WeightedRandomMinecart {
        WeightedRandomMinecart(const NBTTagCompound& tagCompound);
        WeightedRandomMinecart(std::shared_ptr<NBTTagCompound> tagCompound, std::string type, int weight = 1);
        std::shared_ptr<NBTTagCompound> toNBT() const;

        int itemWeight;
        std::shared_ptr<NBTTagCompound> nbtData;
        std::string entityType;
    };

    explicit MobSpawnerBaseLogic(class TileEntityMobSpawner& owner)
        : owner(owner)
    {
    }

    void setEntityName(std::string name) { mobID = std::move(name); }
    // Java: updateSpawner -- spawning needs entities (stage 10).
    void updateSpawner();
    void readFromNBT(const NBTTagCompound& nbt);
    void writeToNBT(NBTTagCompound& nbt) const;
    bool setDelayToMin(int delay);
    void setRandomEntity(std::optional<WeightedRandomMinecart> entity);
    double getMobRotation() const { return mobRotation; }
    double getPrevMobRotation() const { return prevMobRotation; }
    std::string getEntityNameToSpawn() const;

private:
    World* getSpawnerWorld() const;
    BlockPos getSpawnerPosition() const;

    TileEntityMobSpawner& owner;
    int spawnDelay = 20;
    std::string mobID = "Pig";
    std::vector<WeightedRandomMinecart> minecartToSpawn;
    std::optional<WeightedRandomMinecart> randomEntity;
    double mobRotation = 0.0;
    double prevMobRotation = 0.0;
    int minSpawnDelay = 200;
    int maxSpawnDelay = 800;
    int spawnCount = 4;
    int maxNearbyEntities = 6;
    int activatingRangeFromPlayer = 16;
    int spawnRange = 4;
};

class TileEntityMobSpawner : public TileEntity {
public:
    void readFromNBT(const NBTTagCompound& compound) override;
    void writeToNBT(NBTTagCompound& compound) const override;
    bool isTickable() const override { return true; }
    void update() override { spawnerLogic.updateSpawner(); }
    bool receiveClientEvent(int id, int type) override;
    bool func_183000_F() const override { return true; }
    std::shared_ptr<Packet> getDescriptionPacket() override;
    MobSpawnerBaseLogic& getSpawnerBaseLogic() { return spawnerLogic; }
    std::string_view getMappingName() const override { return "MobSpawner"; }

private:
    friend class MobSpawnerBaseLogic;
    MobSpawnerBaseLogic spawnerLogic { *this };
};

class TileEntityFlowerPot : public TileEntity {
public:
    TileEntityFlowerPot() = default;
    TileEntityFlowerPot(Item* potItem, int potData)
        : flowerPotItem(potItem)
        , flowerPotData(potData)
    {
    }

    void writeToNBT(NBTTagCompound& compound) const override;
    void readFromNBT(const NBTTagCompound& compound) override;
    void setFlowerPotData(Item* potItem, int potData)
    {
        flowerPotItem = potItem;
        flowerPotData = potData;
    }
    Item* getFlowerPotItem() const { return flowerPotItem; }
    int getFlowerPotData() const { return flowerPotData; }
    std::shared_ptr<Packet> getDescriptionPacket() override;
    std::string_view getMappingName() const override { return "FlowerPot"; }

private:
    Item* flowerPotItem = nullptr;
    int flowerPotData = 0;
};

class TileEntityEnderChest : public TileEntity {
public:
    // Java: update() -- the lid animation with viewers (the GUI stages).
    bool isTickable() const override { return true; }
    std::string_view getMappingName() const override { return "EnderChest"; }

    float lidAngle = 0.0f;
    float prevLidAngle = 0.0f;
    int numPlayersUsing = 0;
};

class TileEntityEnchantmentTable : public TileEntity {
public:
    void readFromNBT(const NBTTagCompound& compound) override;
    void writeToNBT(NBTTagCompound& compound) const override;
    // Java: update() -- the book animation, needs the nearest player (stage 10).
    bool isTickable() const override { return true; }
    std::string_view getMappingName() const override { return "EnchantTable"; }

private:
    std::optional<std::string> customName;
};

class TileEntityEndPortal : public TileEntity {
public:
    std::string_view getMappingName() const override { return "Airportal"; }
};

class TileEntityNote : public TileEntity {
public:
    void readFromNBT(const NBTTagCompound& compound) override;
    void writeToNBT(NBTTagCompound& compound) const override;
    std::string_view getMappingName() const override { return "Music"; }

    int8_t note = 0;
    bool previousRedstoneState = false;
};

class TileEntityComparator : public TileEntity {
public:
    void readFromNBT(const NBTTagCompound& compound) override;
    void writeToNBT(NBTTagCompound& compound) const override;
    int getOutputSignal() const { return outputSignal; }
    void setOutputSignal(int value) { outputSignal = value; }
    std::string_view getMappingName() const override { return "Comparator"; }

private:
    int outputSignal = 0;
};

class TileEntityDaylightDetector : public TileEntity {
public:
    // Java: update() -- BlockDaylightDetector.updatePower every 20 ticks, stage 9.
    bool isTickable() const override { return true; }
    std::string_view getMappingName() const override { return "DLDetector"; }
};

} // namespace mc

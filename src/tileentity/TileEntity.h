#pragma once

// Original: net/minecraft/tileentity/TileEntity.java, ITickable.java, inventory/IInventory.java,
// world/IWorldNameable.java, world/LockCode.java and TileEntityLockable.java.
// Tile entities are shared between the chunk's map and the world's lists, as Java references are.
// Ported classes are in tileentity/TileEntities.h. The other registered ids load as TileEntityUnported, which
// keeps the NBT it was read from and writes it back, so saves round-trip until those classes are ported.

#include "util/BlockPos.h"

#include <functional>
#include <memory>
#include <string>
#include <string_view>

namespace mc {

class Block;
class EntityPlayer;
class ItemStack;
class NBTTagCompound;
class Packet;
class World;

class TileEntity {
public:
    virtual ~TileEntity() = default;

    // Java: addMapping -- the static initializer's table, called from Bootstrap.
    static void registerTileEntities();
    // Null for ids without a mapping (Java logs "Skipping BlockEntity with id ...").
    static std::shared_ptr<TileEntity> createAndLoadEntity(const NBTTagCompound& nbt);

    World* getWorld() const { return worldObj; }
    void setWorldObj(World* worldIn) { worldObj = worldIn; }
    bool hasWorldObj() const { return worldObj != nullptr; }
    virtual void readFromNBT(const NBTTagCompound& compound);
    virtual void writeToNBT(NBTTagCompound& compound) const;
    int getBlockMetadata();
    virtual void markDirty();
    double getDistanceSq(double x, double y, double z) const;
    virtual double getMaxRenderDistanceSquared() const { return 4096.0; }
    const BlockPos& getPos() const { return pos; }
    Block* getBlockType();
    // The packet that shows the tile entity to clients; null when they need none.
    virtual std::shared_ptr<Packet> getDescriptionPacket() { return nullptr; }
    bool isInvalid() const { return tileEntityInvalid; }
    virtual void invalidate() { tileEntityInvalid = true; }
    virtual void validate() { tileEntityInvalid = false; }
    virtual bool receiveClientEvent(int id, int type);
    virtual void updateContainingBlockInfo();
    void setPos(const BlockPos& posIn) { pos = posIn; }
    virtual bool func_183000_F() const { return false; }
    // Java: instanceof ITickable / ITickable.update().
    virtual bool isTickable() const { return false; }
    virtual void update() { }
    // The id written to NBT (Java: classToNameMap.get(getClass())).
    virtual std::string_view getMappingName() const = 0;

protected:
    World* worldObj = nullptr;
    BlockPos pos = BlockPos::ORIGIN;
    bool tileEntityInvalid = false;
    Block* blockType = nullptr;

private:
    int blockMetadata = -1;
};

// A registered id whose class isn't ported yet: keeps its NBT.
class TileEntityUnported : public TileEntity {
public:
    explicit TileEntityUnported(std::string id)
        : mappingName(std::move(id))
    {
    }

    void readFromNBT(const NBTTagCompound& compound) override;
    void writeToNBT(NBTTagCompound& compound) const override;
    std::string_view getMappingName() const override { return mappingName; }

private:
    std::string mappingName;
    std::shared_ptr<NBTTagCompound> data;
};

class IWorldNameable {
public:
    virtual ~IWorldNameable() = default;
    virtual std::string getName() const = 0;
    virtual bool hasCustomName() const = 0;
    // Java: getDisplayName() -- a chat component (the GUI stages).
};

class IInventory : public IWorldNameable {
public:
    virtual int getSizeInventory() const = 0;
    virtual std::shared_ptr<ItemStack> getStackInSlot(int index) const = 0;
    virtual std::shared_ptr<ItemStack> decrStackSize(int index, int count) = 0;
    virtual std::shared_ptr<ItemStack> removeStackFromSlot(int index) = 0;
    virtual void setInventorySlotContents(int index, std::shared_ptr<ItemStack> stack) = 0;
    virtual int getInventoryStackLimit() const = 0;
    virtual void markDirty() = 0;
    virtual bool isUseableByPlayer(EntityPlayer* player) = 0;
    virtual void openInventory(EntityPlayer* player) = 0;
    virtual void closeInventory(EntityPlayer* player) = 0;
    virtual bool isItemValidForSlot(int index, const ItemStack& stack) const = 0;
    virtual int getField(int id) const = 0;
    virtual void setField(int id, int value) = 0;
    virtual int getFieldCount() const = 0;
    virtual void clear() = 0;
};

class TileEntityLockable : public TileEntity, public IInventory {
public:
    void readFromNBT(const NBTTagCompound& compound) override;
    void writeToNBT(NBTTagCompound& compound) const override;
    bool isLocked() const { return !code.empty(); }
    const std::string& getLockCode() const { return code; }
    void setLockCode(std::string lockCode) { code = std::move(lockCode); }
    void markDirty() override { TileEntity::markDirty(); }

private:
    // Java: LockCode; the empty string is EMPTY_CODE.
    std::string code;
};

} // namespace mc

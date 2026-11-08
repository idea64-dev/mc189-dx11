#include "tileentity/TileEntityJukebox.h"

#include "item/Item.h"
#include "item/ItemStack.h"
#include "nbt/NBTTagCompound.h"

namespace mc {

void TileEntityJukebox::readFromNBT(const NBTTagCompound& compound)
{
    TileEntity::readFromNBT(compound);
    if (compound.hasKey("RecordItem", 10)) {
        auto loaded = ItemStack::loadItemStackFromNBT(*compound.getCompoundTag("RecordItem"));
        setRecord(loaded ? std::shared_ptr<ItemStack>(std::move(loaded)) : nullptr);
    } else if (compound.getInteger("Record") > 0) {
        setRecord(std::make_shared<ItemStack>(Item::getItemById(compound.getInteger("Record")), 1, 0));
    }
}

void TileEntityJukebox::writeToNBT(NBTTagCompound& compound) const
{
    TileEntity::writeToNBT(compound);
    if (record) {
        auto nbttagcompound = std::make_shared<NBTTagCompound>();
        record->writeToNBT(*nbttagcompound);
        compound.setTag("RecordItem", nbttagcompound);
    }
}

void TileEntityJukebox::setRecord(std::shared_ptr<ItemStack> recordStack)
{
    record = std::move(recordStack);
    markDirty();
}

} // namespace mc

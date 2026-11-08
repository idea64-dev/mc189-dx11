#pragma once

// Original: net/minecraft/block/BlockJukebox.java (TileEntityJukebox) -- the record in a jukebox.

#include "tileentity/TileEntity.h"

#include <memory>

namespace mc {

class TileEntityJukebox : public TileEntity {
public:
    void readFromNBT(const NBTTagCompound& compound) override;
    void writeToNBT(NBTTagCompound& compound) const override;
    const std::shared_ptr<ItemStack>& getRecord() const { return record; }
    void setRecord(std::shared_ptr<ItemStack> recordStack);
    std::string_view getMappingName() const override { return "RecordPlayer"; }

private:
    std::shared_ptr<ItemStack> record;
};

} // namespace mc

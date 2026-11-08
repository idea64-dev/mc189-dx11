#pragma once

// Original: net/minecraft/inventory/ISidedInventory.java -- an inventory whose slots depend on the side used.
// Java extends IInventory; here it is a separate interface next to IInventory (TileEntityLockable already derives
// from IInventory), reached with dynamic_cast. Kept with the tile entities until inventory/ has it.

#include "util/EnumFacing.h"

#include <optional>
#include <vector>

namespace mc {

class ItemStack;

class ISidedInventory {
public:
    virtual ~ISidedInventory() = default;
    virtual std::vector<int> getSlotsForFace(EnumFacing side) = 0;
    // Java passes a null direction when a hopper inserts without a side.
    virtual bool canInsertItem(int index, const ItemStack& itemStackIn, std::optional<EnumFacing> direction) = 0;
    virtual bool canExtractItem(int index, const ItemStack& stack, EnumFacing direction) = 0;
};

} // namespace mc

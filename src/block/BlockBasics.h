#pragma once

// Includes and small helpers shared by the block classes.

#include "block/Block.h"
#include "core/EnumValues.h"
#include "creativetab/CreativeTabs.h"
#include "init/Blocks.h"
#include "init/Items.h"
#include "item/ItemStack.h"
#include "util/Random.h"

namespace mc {

// EnumFacing.Plane.HORIZONTAL as a property filter.
inline bool isHorizontalFacing(EnumFacing facing)
{
    return isHorizontal(getAxis(facing));
}

} // namespace mc

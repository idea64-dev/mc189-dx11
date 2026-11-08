#pragma once

// Includes and small helpers shared by the block classes.

#include "block/Block.h"
#include "core/EnumValues.h"
#include "creativetab/CreativeTabs.h"

namespace mc {

// EnumFacing.Plane.HORIZONTAL as a property filter.
inline bool isHorizontalFacing(EnumFacing facing)
{
    return isHorizontal(getAxis(facing));
}

} // namespace mc

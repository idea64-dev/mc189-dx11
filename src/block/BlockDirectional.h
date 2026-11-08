#pragma once

// Original: net/minecraft/block/BlockDirectional.java

#include "block/BlockBasics.h"

namespace mc {

class BlockDirectional : public Block {
public:
    static const PropertyDirection FACING;

    explicit BlockDirectional(const Material& materialIn)
        : Block(materialIn)
    {
    }
    BlockDirectional(const Material& materialIn, const MapColor& mapColorIn)
        : Block(materialIn, mapColorIn)
    {
    }
};

inline const PropertyDirection BlockDirectional::FACING("facing", isHorizontalFacing);

} // namespace mc

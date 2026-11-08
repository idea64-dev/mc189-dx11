#pragma once

// Original: net/minecraft/block/BlockCompressedPowered.java (the redstone block)

#include "block/BlockBasics.h"

namespace mc {

class BlockCompressedPowered : public Block {
public:
    MC_BLOCK_CLASS(BlockCompressedPowered)

    BlockCompressedPowered(const Material& materialIn, const MapColor& mapColorIn)
        : Block(materialIn, mapColorIn)
    {
    }

    bool canProvidePower() const override { return true; }
    int getWeakPower(IBlockAccess*, const BlockPos&, const IBlockState*, EnumFacing) override { return 15; }
};

} // namespace mc

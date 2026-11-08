#pragma once

// Original: net/minecraft/block/BlockAir.java

#include "block/BlockBasics.h"

namespace mc {

class BlockAir : public Block {
public:
    MC_BLOCK_CLASS(BlockAir)

    BlockAir()
        : Block(Material::air)
    {
    }

    int getRenderType() const override { return -1; }
    bool isOpaqueCube() const override { return false; }
    bool canCollideCheck(const IBlockState*, bool) const override { return false; }
    std::optional<AxisAlignedBB> getCollisionBoundingBox(World*, const BlockPos&, const IBlockState*) override { return std::nullopt; }
    bool isReplaceable(World*, const BlockPos&) override { return true; }
    void dropBlockAsItemWithChance(World*, const BlockPos&, const IBlockState*, float, int) override { }
};

} // namespace mc

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
    // Java: getCollisionBoundingBox returns null, isReplaceable true, no drops (stage 9).
};

} // namespace mc

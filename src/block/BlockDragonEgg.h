#pragma once

// Original: net/minecraft/block/BlockDragonEgg.java

#include "block/BlockBasics.h"

namespace mc {

class BlockDragonEgg : public Block {
public:
    MC_BLOCK_CLASS(BlockDragonEgg)

    BlockDragonEgg()
        : Block(Material::dragonEgg, MapColor::blackColor)
    {
    }

    bool isOpaqueCube() const override { return false; }
    bool isFullCube() const override { return false; }

protected:
    void construct() override
    {
        Block::construct();
        setBlockBounds(0.0625f, 0.0f, 0.0625f, 0.9375f, 1.0f, 0.9375f);
    }
};

} // namespace mc

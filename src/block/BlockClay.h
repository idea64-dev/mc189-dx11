#pragma once

// Original: net/minecraft/block/BlockClay.java

#include "block/BlockBasics.h"

namespace mc {

class BlockClay : public Block {
public:
    MC_BLOCK_CLASS(BlockClay)

    BlockClay()
        : Block(Material::clay)
    {
    }

    int quantityDropped(Random&) const override { return 4; }
    Item* getItemDropped(const IBlockState*, Random&, int) const override { return Items::clay_ball; }

protected:
    void construct() override
    {
        Block::construct();
        setCreativeTab(&CreativeTabs::tabBlock);
    }
};

} // namespace mc

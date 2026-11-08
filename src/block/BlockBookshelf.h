#pragma once

// Original: net/minecraft/block/BlockBookshelf.java

#include "block/BlockBasics.h"

namespace mc {

class BlockBookshelf : public Block {
public:
    MC_BLOCK_CLASS(BlockBookshelf)

    BlockBookshelf()
        : Block(Material::wood)
    {
    }

    int quantityDropped(Random&) const override { return 3; }

protected:
    void construct() override
    {
        Block::construct();
        setCreativeTab(&CreativeTabs::tabBlock);
    }
};

} // namespace mc

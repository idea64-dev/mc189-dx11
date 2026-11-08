#pragma once

// Original: net/minecraft/block/BlockContainer.java (+ ITileEntityProvider; tile entities arrive in stage 9)

#include "block/BlockBasics.h"

namespace mc {

class BlockContainer : public Block {
public:
    explicit BlockContainer(const Material& materialIn)
        : BlockContainer(materialIn, materialIn.getMaterialMapColor())
    {
    }
    BlockContainer(const Material& materialIn, const MapColor& mapColorIn)
        : Block(materialIn, mapColorIn)
    {
    }

    int getRenderType() const override { return -1; }

protected:
    void construct() override
    {
        Block::construct();
        isBlockContainer = true;
    }
};

} // namespace mc

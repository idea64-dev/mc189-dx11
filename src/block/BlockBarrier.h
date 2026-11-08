#pragma once

// Original: net/minecraft/block/BlockBarrier.java

#include "block/BlockBasics.h"

namespace mc {

class BlockBarrier : public Block {
public:
    MC_BLOCK_CLASS(BlockBarrier)

    BlockBarrier()
        : Block(Material::barrier)
    {
    }

    int getRenderType() const override { return -1; }
    bool isOpaqueCube() const override { return false; }
    float getAmbientOcclusionLightValue() const override { return 1.0f; }

protected:
    void construct() override
    {
        Block::construct();
        setBlockUnbreakable();
        setResistance(6000001.0f);
        disableStats();
        translucent = true;
    }
};

} // namespace mc

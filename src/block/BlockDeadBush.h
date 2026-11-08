#pragma once

// Original: net/minecraft/block/BlockDeadBush.java

#include "block/BlockBush.h"

namespace mc {

class BlockDeadBush : public BlockBush {
public:
    MC_BLOCK_CLASS(BlockDeadBush)

    BlockDeadBush()
        : BlockBush(Material::vine)
    {
    }

    const MapColor& getMapColor(const IBlockState*) const override { return MapColor::woodColor; }
    Item* getItemDropped(const IBlockState*, Random&, int) const override { return nullptr; }
    bool isReplaceable(World*, const BlockPos&) override { return true; }
    // Java: harvestBlock drops the bush itself for shears -- the player comes with stage 8.

protected:
    bool canPlaceBlockOn(const Block* ground) const override;
    void construct() override
    {
        BlockBush::construct();
        const float f = 0.4f;
        setBlockBounds(0.5f - f, 0.0f, 0.5f - f, 0.5f + f, 0.8f, 0.5f + f);
    }
};

} // namespace mc

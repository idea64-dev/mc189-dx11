#pragma once

// Original: net/minecraft/block/BlockContainer.java (+ ITileEntityProvider)

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
    void breakBlock(World* worldIn, const BlockPos& pos, const IBlockState* state) override;
    bool onBlockEventReceived(World* worldIn, const BlockPos& pos, const IBlockState* state, int eventID, int eventParam) override;

protected:
    void construct() override
    {
        Block::construct();
        isBlockContainer = true;
    }

    // Whether a cactus touches the side / any horizontal side (signs and banners can't be placed there).
    bool isInvalidNeighbor(World* worldIn, const BlockPos& pos, EnumFacing side);
    bool hasInvalidNeighbor(World* worldIn, const BlockPos& pos);
};

} // namespace mc

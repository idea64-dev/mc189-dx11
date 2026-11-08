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
    void onBlockAdded(World* worldIn, const BlockPos& pos, const IBlockState* state) override;
    void onNeighborBlockChange(World* worldIn, const BlockPos& pos, const IBlockState* state, Block* neighborBlock) override;
    void updateTick(World* worldIn, const BlockPos& pos, const IBlockState* state, Random& rand) override;
    bool onBlockActivated(World* worldIn, const BlockPos& pos, const IBlockState* state, EntityPlayer* playerIn, EnumFacing side, float hitX,
        float hitY, float hitZ) override;
    void onBlockClicked(World* worldIn, const BlockPos& pos, EntityPlayer* playerIn) override;
    int tickRate(World*) override { return 5; }
    bool shouldSideBeRendered(IBlockAccess*, const BlockPos&, EnumFacing) override { return true; }
    Item* getItem(World*, const BlockPos&) override { return nullptr; }

private:
    void checkFall(World* worldIn, const BlockPos& pos);
    void teleport(World* worldIn, const BlockPos& pos);

public:

protected:
    void construct() override
    {
        Block::construct();
        setBlockBounds(0.0625f, 0.0f, 0.0625f, 0.9375f, 1.0f, 0.9375f);
    }
};

} // namespace mc

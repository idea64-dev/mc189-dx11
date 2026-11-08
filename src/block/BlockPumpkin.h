#pragma once

// Original: net/minecraft/block/BlockPumpkin.java (golem and snowman patterns come with entities, stage 10)

#include "block/BlockDirectional.h"

namespace mc {

class BlockPumpkin : public BlockDirectional {
public:
    bool canPlaceBlockAt(World* worldIn, const BlockPos& pos) override;
    MC_BLOCK_CLASS(BlockPumpkin)

    BlockPumpkin()
        : BlockDirectional(Material::gourd, MapColor::adobeColor)
    {
    }

    const IBlockState* getStateFromMeta(int meta) const override { return getDefaultState()->withProperty(FACING, getHorizontal(meta)); }
    int getMetaFromState(const IBlockState* state) const override { return getHorizontalIndex(state->getValue(FACING)); }
    // Java: onBlockAdded then trySpawnGolem builds snow and iron golems from the block patterns (BlockPattern) --
    // entities come with stage 10.
    const IBlockState* onBlockPlaced(World* worldIn, const BlockPos& pos, EnumFacing facing, float hitX, float hitY, float hitZ, int meta,
        EntityLivingBase* placer) override;

protected:
    void construct() override
    {
        BlockDirectional::construct();
        setDefaultState(blockState->getBaseState()->withProperty(FACING, EnumFacing::NORTH));
        setTickRandomly(true);
        setCreativeTab(&CreativeTabs::tabBlock);
    }
    std::unique_ptr<BlockState> createBlockState() override { return makeBlockState({ &FACING }); }
};

} // namespace mc

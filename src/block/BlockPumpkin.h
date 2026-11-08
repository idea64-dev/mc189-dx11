#pragma once

// Original: net/minecraft/block/BlockPumpkin.java (golem and snowman patterns come with entities, stage 11)

#include "block/BlockDirectional.h"

namespace mc {

class BlockPumpkin : public BlockDirectional {
public:
    MC_BLOCK_CLASS(BlockPumpkin)

    BlockPumpkin()
        : BlockDirectional(Material::gourd, MapColor::adobeColor)
    {
    }

    const IBlockState* getStateFromMeta(int meta) const override { return getDefaultState()->withProperty(FACING, getHorizontal(meta)); }
    int getMetaFromState(const IBlockState* state) const override { return getHorizontalIndex(state->getValue(FACING)); }

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

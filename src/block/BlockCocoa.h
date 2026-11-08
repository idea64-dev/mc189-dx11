#pragma once

// Original: net/minecraft/block/BlockCocoa.java

#include "block/BlockDirectional.h"

namespace mc {

class BlockCocoa : public BlockDirectional {
public:
    MC_BLOCK_CLASS(BlockCocoa)

    static const PropertyInteger AGE;

    BlockCocoa()
        : BlockDirectional(Material::plants)
    {
    }

    bool isFullCube() const override { return false; }
    bool isOpaqueCube() const override { return false; }
    EnumWorldBlockLayer getBlockLayer() const override { return EnumWorldBlockLayer::CUTOUT; }
    const IBlockState* getStateFromMeta(int meta) const override
    {
        return getDefaultState()->withProperty(FACING, getHorizontal(meta))->withProperty(AGE, (meta & 15) >> 2);
    }
    int getMetaFromState(const IBlockState* state) const override
    {
        int i = 0;
        i = i | getHorizontalIndex(state->getValue(FACING));
        i = i | state->getValue(AGE) << 2;
        return i;
    }

protected:
    void construct() override
    {
        BlockDirectional::construct();
        setDefaultState(blockState->getBaseState()->withProperty(FACING, EnumFacing::NORTH)->withProperty(AGE, 0));
        setTickRandomly(true);
    }
    std::unique_ptr<BlockState> createBlockState() override { return makeBlockState({ &FACING, &AGE }); }
};

inline const PropertyInteger BlockCocoa::AGE("age", 0, 2);

} // namespace mc

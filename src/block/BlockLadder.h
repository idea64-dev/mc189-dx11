#pragma once

// Original: net/minecraft/block/BlockLadder.java

#include "block/BlockBasics.h"

namespace mc {

class BlockLadder : public Block {
public:
    MC_BLOCK_CLASS(BlockLadder)

    static const PropertyDirection FACING;

    BlockLadder()
        : Block(Material::circuits)
    {
    }

    bool isOpaqueCube() const override { return false; }
    bool isFullCube() const override { return false; }
    EnumWorldBlockLayer getBlockLayer() const override { return EnumWorldBlockLayer::CUTOUT; }
    const IBlockState* getStateFromMeta(int meta) const override
    {
        EnumFacing enumfacing = getFront(meta);
        if (getAxis(enumfacing) == Axis::Y)
            enumfacing = EnumFacing::NORTH;
        return getDefaultState()->withProperty(FACING, enumfacing);
    }
    int getMetaFromState(const IBlockState* state) const override { return getIndex(state->getValue(FACING)); }

protected:
    void construct() override
    {
        Block::construct();
        setDefaultState(blockState->getBaseState()->withProperty(FACING, EnumFacing::NORTH));
        setCreativeTab(&CreativeTabs::tabDecorations);
    }
    std::unique_ptr<BlockState> createBlockState() override { return makeBlockState({ &FACING }); }
};

inline const PropertyDirection BlockLadder::FACING("facing", isHorizontalFacing);

} // namespace mc

#pragma once

// Original: net/minecraft/block/BlockEnderChest.java

#include "block/BlockContainer.h"

namespace mc {

class BlockEnderChest : public BlockContainer {
public:
    MC_BLOCK_CLASS(BlockEnderChest)

    static const PropertyDirection FACING;

    BlockEnderChest()
        : BlockContainer(Material::rock)
    {
    }

    bool isOpaqueCube() const override { return false; }
    bool isFullCube() const override { return false; }
    int getRenderType() const override { return 2; }
    int quantityDropped(Random&) const override { return 8; }
    bool canSilkHarvest() const override { return true; }
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
        BlockContainer::construct();
        setDefaultState(blockState->getBaseState()->withProperty(FACING, EnumFacing::NORTH));
        setCreativeTab(&CreativeTabs::tabDecorations);
        setBlockBounds(0.0625f, 0.0f, 0.0625f, 0.9375f, 0.875f, 0.9375f);
    }
    std::unique_ptr<BlockState> createBlockState() override { return makeBlockState({ &FACING }); }
};

inline const PropertyDirection BlockEnderChest::FACING("facing", isHorizontalFacing);

} // namespace mc

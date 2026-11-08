#pragma once

// Original: net/minecraft/block/BlockChest.java

#include "block/BlockContainer.h"

namespace mc {

class BlockChest : public BlockContainer {
public:
    MC_BLOCK_CLASS(BlockChest)

    static const PropertyDirection FACING;

    explicit BlockChest(int type)
        : BlockContainer(Material::wood)
        , typeArg(type)
    {
    }

    bool isOpaqueCube() const override { return false; }
    bool isFullCube() const override { return false; }
    int getRenderType() const override { return 2; }
    bool canProvidePower() const override { return chestType == 1; }
    bool hasComparatorInputOverride() const override { return true; }
    const IBlockState* getStateFromMeta(int meta) const override
    {
        EnumFacing enumfacing = getFront(meta);
        if (getAxis(enumfacing) == Axis::Y)
            enumfacing = EnumFacing::NORTH;
        return getDefaultState()->withProperty(FACING, enumfacing);
    }
    int getMetaFromState(const IBlockState* state) const override { return getIndex(state->getValue(FACING)); }

    int chestType = 0;

protected:
    void construct() override
    {
        BlockContainer::construct();
        setDefaultState(blockState->getBaseState()->withProperty(FACING, EnumFacing::NORTH));
        chestType = typeArg;
        setCreativeTab(&CreativeTabs::tabDecorations);
        setBlockBounds(0.0625f, 0.0f, 0.0625f, 0.9375f, 0.875f, 0.9375f);
    }
    std::unique_ptr<BlockState> createBlockState() override { return makeBlockState({ &FACING }); }

private:
    int typeArg;
};

inline const PropertyDirection BlockChest::FACING("facing", isHorizontalFacing);

} // namespace mc

#pragma once

// Original: net/minecraft/block/BlockFurnace.java

#include "block/BlockContainer.h"

namespace mc {

class BlockFurnace : public BlockContainer {
public:
    MC_BLOCK_CLASS(BlockFurnace)

    static const PropertyDirection FACING;

    explicit BlockFurnace(bool isBurningIn)
        : BlockContainer(Material::rock)
        , isBurning(isBurningIn)
    {
    }

    bool hasComparatorInputOverride() const override { return true; }
    int getRenderType() const override { return 3; }
    const IBlockState* getStateForEntityRender(const IBlockState*) const override { return getDefaultState()->withProperty(FACING, EnumFacing::SOUTH); }
    const IBlockState* getStateFromMeta(int meta) const override
    {
        EnumFacing enumfacing = getFront(meta);
        if (getAxis(enumfacing) == Axis::Y)
            enumfacing = EnumFacing::NORTH;
        return getDefaultState()->withProperty(FACING, enumfacing);
    }
    int getMetaFromState(const IBlockState* state) const override { return getIndex(state->getValue(FACING)); }
    Item* getItemDropped(const IBlockState*, Random&, int) const override { return Item::getItemFromBlock(Blocks::furnace); }

protected:
    void construct() override
    {
        BlockContainer::construct();
        setDefaultState(blockState->getBaseState()->withProperty(FACING, EnumFacing::NORTH));
    }
    std::unique_ptr<BlockState> createBlockState() override { return makeBlockState({ &FACING }); }

private:
    [[maybe_unused]] const bool isBurning;
};

inline const PropertyDirection BlockFurnace::FACING("facing", isHorizontalFacing);

} // namespace mc

#pragma once

// Original: net/minecraft/block/BlockBanner.java (with BlockBannerHanging and BlockBannerStanding)

#include "block/BlockContainer.h"

namespace mc {

class BlockBanner : public BlockContainer {
public:
    class BlockBannerHanging;
    class BlockBannerStanding;

    static const PropertyDirection FACING;
    static const PropertyInteger ROTATION;

    BlockBanner()
        : BlockContainer(Material::wood)
    {
    }

    bool isFullCube() const override { return false; }
    bool isOpaqueCube() const override { return false; }
    bool canSpawnInBlock() const override { return true; }
    Item* getItemDropped(const IBlockState*, Random&, int) const override { return Items::banner; }

protected:
    void construct() override
    {
        BlockContainer::construct();
        const float f = 0.25f;
        const float f1 = 1.0f;
        setBlockBounds(0.5f - f, 0.0f, 0.5f - f, 0.5f + f, f1, 0.5f + f);
    }
};

class BlockBanner::BlockBannerHanging : public BlockBanner {
public:
    MC_BLOCK_CLASS(BlockBannerHanging)

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
        BlockBanner::construct();
        setDefaultState(blockState->getBaseState()->withProperty(FACING, EnumFacing::NORTH));
    }
    std::unique_ptr<BlockState> createBlockState() override { return makeBlockState({ &FACING }); }
};

class BlockBanner::BlockBannerStanding : public BlockBanner {
public:
    MC_BLOCK_CLASS(BlockBannerStanding)

    const IBlockState* getStateFromMeta(int meta) const override { return getDefaultState()->withProperty(ROTATION, meta); }
    int getMetaFromState(const IBlockState* state) const override { return state->getValue(ROTATION); }

protected:
    void construct() override
    {
        BlockBanner::construct();
        setDefaultState(blockState->getBaseState()->withProperty(ROTATION, 0));
    }
    std::unique_ptr<BlockState> createBlockState() override { return makeBlockState({ &ROTATION }); }
};

inline const PropertyDirection BlockBanner::FACING("facing", isHorizontalFacing);
inline const PropertyInteger BlockBanner::ROTATION("rotation", 0, 15);

} // namespace mc

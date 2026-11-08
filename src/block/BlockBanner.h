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

    std::optional<AxisAlignedBB> getCollisionBoundingBox(World*, const BlockPos&, const IBlockState*) override { return std::nullopt; }
    AxisAlignedBB getSelectedBoundingBox(World* worldIn, const BlockPos& pos) override;
    bool isFullCube() const override { return false; }
    bool isPassable(IBlockAccess*, const BlockPos&) override { return true; }
    bool isOpaqueCube() const override { return false; }
    bool canSpawnInBlock() const override { return true; }
    std::shared_ptr<TileEntity> createNewTileEntity(World* worldIn, int meta) override;
    Item* getItemDropped(const IBlockState*, Random&, int) const override { return Items::banner; }
    Item* getItem(World*, const BlockPos&) override { return Items::banner; }
    void dropBlockAsItemWithChance(World* worldIn, const BlockPos& pos, const IBlockState* state, float chance, int fortune) override;
    bool canPlaceBlockAt(World* worldIn, const BlockPos& pos) override;
    void harvestBlock(World* worldIn, EntityPlayer* player, const BlockPos& pos, const IBlockState* state, TileEntity* te) override;


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

    void setBlockBoundsBasedOnState(IBlockAccess* worldIn, const BlockPos& pos) override;
    void onNeighborBlockChange(World* worldIn, const BlockPos& pos, const IBlockState* state, Block* neighborBlock) override;

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

    void onNeighborBlockChange(World* worldIn, const BlockPos& pos, const IBlockState* state, Block* neighborBlock) override;

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

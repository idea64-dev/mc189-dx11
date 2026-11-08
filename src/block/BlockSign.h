#pragma once

// Original: net/minecraft/block/BlockSign.java, BlockStandingSign.java and BlockWallSign.java

#include "block/BlockContainer.h"

namespace mc {

class BlockSign : public BlockContainer {
public:
    BlockSign()
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
    Item* getItemDropped(const IBlockState*, Random&, int) const override { return Items::sign; }
    Item* getItem(World*, const BlockPos&) override { return Items::sign; }
    bool onBlockActivated(World* worldIn, const BlockPos& pos, const IBlockState* state, EntityPlayer* playerIn, EnumFacing side, float hitX,
        float hitY, float hitZ) override;
    bool canPlaceBlockAt(World* worldIn, const BlockPos& pos) override;


protected:
    void construct() override
    {
        BlockContainer::construct();
        const float f = 0.25f;
        const float f1 = 1.0f;
        setBlockBounds(0.5f - f, 0.0f, 0.5f - f, 0.5f + f, f1, 0.5f + f);
    }
};

class BlockStandingSign : public BlockSign {
public:
    MC_BLOCK_CLASS(BlockStandingSign)

    static const PropertyInteger ROTATION;

    void onNeighborBlockChange(World* worldIn, const BlockPos& pos, const IBlockState* state, Block* neighborBlock) override;

    const IBlockState* getStateFromMeta(int meta) const override { return getDefaultState()->withProperty(ROTATION, meta); }
    int getMetaFromState(const IBlockState* state) const override { return state->getValue(ROTATION); }

protected:
    void construct() override
    {
        BlockSign::construct();
        setDefaultState(blockState->getBaseState()->withProperty(ROTATION, 0));
    }
    std::unique_ptr<BlockState> createBlockState() override { return makeBlockState({ &ROTATION }); }
};

class BlockWallSign : public BlockSign {
public:
    MC_BLOCK_CLASS(BlockWallSign)

    static const PropertyDirection FACING;

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
        BlockSign::construct();
        setDefaultState(blockState->getBaseState()->withProperty(FACING, EnumFacing::NORTH));
    }
    std::unique_ptr<BlockState> createBlockState() override { return makeBlockState({ &FACING }); }
};

inline const PropertyInteger BlockStandingSign::ROTATION("rotation", 0, 15);
inline const PropertyDirection BlockWallSign::FACING("facing", isHorizontalFacing);

} // namespace mc

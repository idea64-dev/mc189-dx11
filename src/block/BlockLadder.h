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

    std::optional<AxisAlignedBB> getCollisionBoundingBox(World* worldIn, const BlockPos& pos, const IBlockState* state) override;
    AxisAlignedBB getSelectedBoundingBox(World* worldIn, const BlockPos& pos) override;
    void setBlockBoundsBasedOnState(IBlockAccess* worldIn, const BlockPos& pos) override;
    bool isOpaqueCube() const override { return false; }
    bool isFullCube() const override { return false; }
    bool canPlaceBlockAt(World* worldIn, const BlockPos& pos) override;
    const IBlockState* onBlockPlaced(World* worldIn, const BlockPos& pos, EnumFacing facing, float hitX, float hitY, float hitZ, int meta,
        EntityLivingBase* placer) override;
    void onNeighborBlockChange(World* worldIn, const BlockPos& pos, const IBlockState* state, Block* neighborBlock) override;
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

    bool canBlockStay(World* worldIn, const BlockPos& pos, EnumFacing facing);
};

inline const PropertyDirection BlockLadder::FACING("facing", isHorizontalFacing);

} // namespace mc

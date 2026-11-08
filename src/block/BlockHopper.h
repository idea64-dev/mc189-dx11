#pragma once

// Original: net/minecraft/block/BlockHopper.java

#include "block/BlockContainer.h"

namespace mc {

class BlockHopper : public BlockContainer {
public:
    MC_BLOCK_CLASS(BlockHopper)

    static const PropertyDirection FACING;
    static const PropertyBool ENABLED;

    BlockHopper()
        : BlockContainer(Material::iron, MapColor::stoneColor)
    {
    }

    void setBlockBoundsBasedOnState(IBlockAccess* worldIn, const BlockPos& pos) override;
    void addCollisionBoxesToList(World* worldIn, const BlockPos& pos, const IBlockState* state, const AxisAlignedBB& mask,
        std::vector<AxisAlignedBB>& list, Entity* collidingEntity) override;
    const IBlockState* onBlockPlaced(World* worldIn, const BlockPos& pos, EnumFacing facing, float hitX, float hitY, float hitZ, int meta,
        EntityLivingBase* placer) override;
    std::shared_ptr<TileEntity> createNewTileEntity(World* worldIn, int meta) override;
    void onBlockPlacedBy(World* worldIn, const BlockPos& pos, const IBlockState* state, EntityLivingBase* placer, const ItemStack& stack) override;
    void onBlockAdded(World* worldIn, const BlockPos& pos, const IBlockState* state) override;
    // Java: onBlockActivated opens the hopper GUI -- stage 8.
    void onNeighborBlockChange(World* worldIn, const BlockPos& pos, const IBlockState* state, Block* neighborBlock) override;
    void breakBlock(World* worldIn, const BlockPos& pos, const IBlockState* state) override;
    bool shouldSideBeRendered(IBlockAccess*, const BlockPos&, EnumFacing) override { return true; }
    int getComparatorInputOverride(World* worldIn, const BlockPos& pos) override;
    static EnumFacing getFacing(int meta) { return getFront(meta & 7); }
    static bool isEnabled(int meta) { return (meta & 8) != 8; }

    int getRenderType() const override { return 3; }
    bool isFullCube() const override { return false; }
    bool isOpaqueCube() const override { return false; }
    bool hasComparatorInputOverride() const override { return true; }
    EnumWorldBlockLayer getBlockLayer() const override { return EnumWorldBlockLayer::CUTOUT_MIPPED; }
    const IBlockState* getStateFromMeta(int meta) const override
    {
        return getDefaultState()->withProperty(FACING, getFacing(meta))->withProperty(ENABLED, isEnabled(meta));
    }
    int getMetaFromState(const IBlockState* state) const override
    {
        int i = 0;
        i = i | getIndex(state->getValue(FACING));
        if (!state->getValue(ENABLED))
            i |= 8;
        return i;
    }

protected:
    void construct() override
    {
        BlockContainer::construct();
        setDefaultState(blockState->getBaseState()->withProperty(FACING, EnumFacing::DOWN)->withProperty(ENABLED, true));
        setCreativeTab(&CreativeTabs::tabRedstone);
        setBlockBounds(0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f);
    }
    std::unique_ptr<BlockState> createBlockState() override { return makeBlockState({ &FACING, &ENABLED }); }

private:
    void updateState(World* worldIn, const BlockPos& pos, const IBlockState* state);
};

inline const PropertyDirection BlockHopper::FACING("facing", [](EnumFacing f) { return f != EnumFacing::UP; });
inline const PropertyBool BlockHopper::ENABLED("enabled");

} // namespace mc

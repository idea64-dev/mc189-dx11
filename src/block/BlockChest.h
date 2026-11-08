#pragma once

// Original: net/minecraft/block/BlockChest.java

#include "block/BlockContainer.h"

namespace mc {

class IInventory;

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
    void setBlockBoundsBasedOnState(IBlockAccess* worldIn, const BlockPos& pos) override;
    void onBlockAdded(World* worldIn, const BlockPos& pos, const IBlockState* state) override;
    const IBlockState* onBlockPlaced(World* worldIn, const BlockPos& pos, EnumFacing facing, float hitX, float hitY, float hitZ, int meta,
        EntityLivingBase* placer) override;
    void onBlockPlacedBy(World* worldIn, const BlockPos& pos, const IBlockState* state, EntityLivingBase* placer, const ItemStack& stack) override;
    const IBlockState* checkForSurroundingChests(World* worldIn, const BlockPos& pos, const IBlockState* state);
    bool canProvidePower() const override { return chestType == 1; }
    bool canPlaceBlockAt(World* worldIn, const BlockPos& pos) override;
    void onNeighborBlockChange(World* worldIn, const BlockPos& pos, const IBlockState* state, Block* neighborBlock) override;
    void breakBlock(World* worldIn, const BlockPos& pos, const IBlockState* state) override;
    // Java: onBlockActivated opens the chest GUI -- stage 8.
    // Java: an ILockableContainer -- the chest, or both halves as an InventoryLargeChest; null when blocked.
    std::shared_ptr<IInventory> getLockableContainer(World* worldIn, const BlockPos& pos);
    std::shared_ptr<TileEntity> createNewTileEntity(World* worldIn, int meta) override;
    int getWeakPower(IBlockAccess* worldIn, const BlockPos& pos, const IBlockState* state, EnumFacing side) override;
    int getStrongPower(IBlockAccess* worldIn, const BlockPos& pos, const IBlockState* state, EnumFacing side) override;
    int getComparatorInputOverride(World* worldIn, const BlockPos& pos) override;

    bool hasComparatorInputOverride() const override { return true; }
    const IBlockState* getStateFromMeta(int meta) const override
    {
        EnumFacing enumfacing = getFront(meta);
        if (getAxis(enumfacing) == Axis::Y)
            enumfacing = EnumFacing::NORTH;
        return getDefaultState()->withProperty(FACING, enumfacing);
    }
    int getMetaFromState(const IBlockState* state) const override { return getIndex(state->getValue(FACING)); }
    const IBlockState* correctFacing(World* worldIn, const BlockPos& pos, const IBlockState* state);

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
    bool isDoubleChest(World* worldIn, const BlockPos& pos);
    bool isBlocked(World* worldIn, const BlockPos& pos);
    bool isBelowSolidBlock(World* worldIn, const BlockPos& pos);
    bool isOcelotSittingOnChest(World* worldIn, const BlockPos& pos);

    int typeArg;
};

inline const PropertyDirection BlockChest::FACING("facing", isHorizontalFacing);

} // namespace mc

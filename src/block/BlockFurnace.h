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

    void onBlockAdded(World* worldIn, const BlockPos& pos, const IBlockState* state) override;
    void randomDisplayTick(World* worldIn, const BlockPos& pos, const IBlockState* state, Random& rand) override;
    // Java: onBlockActivated opens the furnace GUI -- stage 8.
    // Swaps between furnace and lit_furnace and keeps the tile entity.
    static void setState(bool active, World* worldIn, const BlockPos& pos);
    std::shared_ptr<TileEntity> createNewTileEntity(World* worldIn, int meta) override;
    const IBlockState* onBlockPlaced(World* worldIn, const BlockPos& pos, EnumFacing facing, float hitX, float hitY, float hitZ, int meta,
        EntityLivingBase* placer) override;
    void onBlockPlacedBy(World* worldIn, const BlockPos& pos, const IBlockState* state, EntityLivingBase* placer, const ItemStack& stack) override;
    void breakBlock(World* worldIn, const BlockPos& pos, const IBlockState* state) override;
    int getComparatorInputOverride(World* worldIn, const BlockPos& pos) override;
    Item* getItem(World*, const BlockPos&) override { return Item::getItemFromBlock(Blocks::furnace); }
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
    void setDefaultFacing(World* worldIn, const BlockPos& pos, const IBlockState* state);

    const bool isBurning;
    static inline bool keepInventory = false;
};

inline const PropertyDirection BlockFurnace::FACING("facing", isHorizontalFacing);

} // namespace mc

#pragma once

// Original: net/minecraft/block/BlockCocoa.java

#include "block/BlockDirectional.h"

namespace mc {

class BlockCocoa : public BlockDirectional {
public:
    MC_BLOCK_CLASS(BlockCocoa)

    static const PropertyInteger AGE;

    BlockCocoa()
        : BlockDirectional(Material::plants)
    {
    }

    bool isFullCube() const override { return false; }
    bool isOpaqueCube() const override { return false; }
    EnumWorldBlockLayer getBlockLayer() const override { return EnumWorldBlockLayer::CUTOUT; }
    const IBlockState* getStateFromMeta(int meta) const override
    {
        return getDefaultState()->withProperty(FACING, getHorizontal(meta))->withProperty(AGE, (meta & 15) >> 2);
    }
    int getMetaFromState(const IBlockState* state) const override
    {
        int i = 0;
        i = i | getHorizontalIndex(state->getValue(FACING));
        i = i | state->getValue(AGE) << 2;
        return i;
    }
    void updateTick(World* worldIn, const BlockPos& pos, const IBlockState* state, Random& rand) override;
    bool canBlockStay(World* worldIn, const BlockPos& pos, const IBlockState* state);
    std::optional<AxisAlignedBB> getCollisionBoundingBox(World* worldIn, const BlockPos& pos, const IBlockState* state) override;
    AxisAlignedBB getSelectedBoundingBox(World* worldIn, const BlockPos& pos) override;
    void setBlockBoundsBasedOnState(IBlockAccess* worldIn, const BlockPos& pos) override;
    void onBlockPlacedBy(World* worldIn, const BlockPos& pos, const IBlockState* state, EntityLivingBase* placer, const ItemStack& stack) override;
    const IBlockState* onBlockPlaced(World* worldIn, const BlockPos& pos, EnumFacing facing, float hitX, float hitY, float hitZ, int meta,
        EntityLivingBase* placer) override;
    void onNeighborBlockChange(World* worldIn, const BlockPos& pos, const IBlockState* state, Block* neighborBlock) override;
    void dropBlockAsItemWithChance(World* worldIn, const BlockPos& pos, const IBlockState* state, float chance, int fortune) override;
    Item* getItem(World*, const BlockPos&) override { return Items::dye; }
    int getDamageValue(World*, const BlockPos&) override;
    // Java: IGrowable
    bool canGrow(World*, const BlockPos&, const IBlockState* state, bool) { return state->getValue(AGE) < 2; }
    bool canUseBonemeal(World*, Random&, const BlockPos&, const IBlockState*) { return true; }
    void grow(World* worldIn, Random& rand, const BlockPos& pos, const IBlockState* state);

private:
    void dropBlock(World* worldIn, const BlockPos& pos, const IBlockState* state);

public:

protected:
    void construct() override
    {
        BlockDirectional::construct();
        setDefaultState(blockState->getBaseState()->withProperty(FACING, EnumFacing::NORTH)->withProperty(AGE, 0));
        setTickRandomly(true);
    }
    std::unique_ptr<BlockState> createBlockState() override { return makeBlockState({ &FACING, &AGE }); }
};

inline const PropertyInteger BlockCocoa::AGE("age", 0, 2);

} // namespace mc

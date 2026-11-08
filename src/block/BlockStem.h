#pragma once

// Original: net/minecraft/block/BlockStem.java

#include "block/BlockBush.h"

namespace mc {

class BlockStem : public BlockBush {
public:
    MC_BLOCK_CLASS(BlockStem)

    static const PropertyInteger AGE;
    static const PropertyDirection FACING;

    explicit BlockStem(Block* cropIn)
        : cropArg(cropIn)
    {
    }

    int getRenderColor(const IBlockState* state) const override
    {
        if (state->getBlock() != this)
            return BlockBush::getRenderColor(state);
        const int i = state->getValue(AGE);
        const int j = i * 32;
        const int k = 255 - i * 8;
        const int l = i * 4;
        return j << 16 | k << 8 | l;
    }
    void setBlockBoundsForItemRender() override
    {
        const float f = 0.125f;
        setBlockBounds(0.5f - f, 0.0f, 0.5f - f, 0.5f + f, 0.25f, 0.5f + f);
    }
    const IBlockState* getStateFromMeta(int meta) const override { return getDefaultState()->withProperty(AGE, meta); }
    int getMetaFromState(const IBlockState* state) const override { return state->getValue(AGE); }
    Item* getItemDropped(const IBlockState*, Random&, int) const override { return nullptr; }
    const IBlockState* getActualState(const IBlockState* state, IBlockAccess* worldIn, const BlockPos& pos) override;
    void updateTick(World* worldIn, const BlockPos& pos, const IBlockState* state, Random& rand) override;
    void growStem(World* worldIn, const BlockPos& pos, const IBlockState* state);
    int colorMultiplier(IBlockAccess* worldIn, const BlockPos& pos, int renderPass) override;
    void setBlockBoundsBasedOnState(IBlockAccess* worldIn, const BlockPos& pos) override;
    void dropBlockAsItemWithChance(World* worldIn, const BlockPos& pos, const IBlockState* state, float chance, int fortune) override;
    Item* getItem(World*, const BlockPos&) override { return getSeedItem(); }
    // Java: IGrowable
    bool canGrow(World*, const BlockPos&, const IBlockState* state, bool) { return state->getValue(AGE) != 7; }
    bool canUseBonemeal(World*, Random&, const BlockPos&, const IBlockState*) { return true; }
    void grow(World* worldIn, Random&, const BlockPos& pos, const IBlockState* state) { growStem(worldIn, pos, state); }

protected:
    bool canPlaceBlockOn(const Block* ground) const override { return ground == Blocks::farmland; }
    Item* getSeedItem() const;
    void construct() override
    {
        BlockBush::construct();
        setDefaultState(blockState->getBaseState()->withProperty(AGE, 0)->withProperty(FACING, EnumFacing::UP));
        crop = cropArg;
        setTickRandomly(true);
        const float f = 0.125f;
        setBlockBounds(0.5f - f, 0.0f, 0.5f - f, 0.5f + f, 0.25f, 0.5f + f);
        setCreativeTab(nullptr);
    }
    std::unique_ptr<BlockState> createBlockState() override { return makeBlockState({ &AGE, &FACING }); }

private:
    Block* crop = nullptr;
    Block* cropArg;
};

inline const PropertyInteger BlockStem::AGE("age", 0, 7);
inline const PropertyDirection BlockStem::FACING("facing", [](EnumFacing f) { return f != EnumFacing::DOWN; });

} // namespace mc

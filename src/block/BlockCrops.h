#pragma once

// Original: net/minecraft/block/BlockCrops.java, BlockCarrot.java and BlockPotato.java

#include "block/BlockBush.h"

namespace mc {

class BlockCrops : public BlockBush {
public:
    MC_BLOCK_CLASS(BlockCrops)

    static const PropertyInteger AGE;

    const IBlockState* getStateFromMeta(int meta) const override { return getDefaultState()->withProperty(AGE, meta); }
    int getMetaFromState(const IBlockState* state) const override { return state->getValue(AGE); }
    Item* getItemDropped(const IBlockState* state, Random&, int) const override { return state->getValue(AGE) == 7 ? getCrop() : getSeed(); }
    virtual Item* getSeed() const;
    virtual Item* getCrop() const;
    void updateTick(World* worldIn, const BlockPos& pos, const IBlockState* state, Random& rand) override;
    void grow(World* worldIn, const BlockPos& pos, const IBlockState* state);
    static float getGrowthChance(const Block* blockIn, World* worldIn, const BlockPos& pos);
    bool canBlockStay(World* worldIn, const BlockPos& pos, const IBlockState* state) override;
    void dropBlockAsItemWithChance(World* worldIn, const BlockPos& pos, const IBlockState* state, float chance, int fortune) override;
    Item* getItem(World*, const BlockPos&) override { return getSeed(); }
    // Java: IGrowable
    bool canGrow(World*, const BlockPos&, const IBlockState* state, bool) { return state->getValue(AGE) < 7; }
    bool canUseBonemeal(World*, Random&, const BlockPos&, const IBlockState*) { return true; }
    void grow(World* worldIn, Random&, const BlockPos& pos, const IBlockState* state) { grow(worldIn, pos, state); }

protected:
    bool canPlaceBlockOn(const Block* ground) const override { return ground == Blocks::farmland; }
    void construct() override
    {
        BlockBush::construct();
        setDefaultState(blockState->getBaseState()->withProperty(AGE, 0));
        setTickRandomly(true);
        const float f = 0.5f;
        setBlockBounds(0.5f - f, 0.0f, 0.5f - f, 0.5f + f, 0.25f, 0.5f + f);
        setCreativeTab(nullptr);
        setHardness(0.0f);
        setStepSound(soundTypeGrass);
        disableStats();
    }
    std::unique_ptr<BlockState> createBlockState() override { return makeBlockState({ &AGE }); }
};

class BlockCarrot : public BlockCrops {
public:
    MC_BLOCK_CLASS(BlockCarrot)
    Item* getSeed() const override;
    Item* getCrop() const override;
};

class BlockPotato : public BlockCrops {
public:
    MC_BLOCK_CLASS(BlockPotato)
    void dropBlockAsItemWithChance(World* worldIn, const BlockPos& pos, const IBlockState* state, float chance, int fortune) override;
    Item* getSeed() const override;
    Item* getCrop() const override;
};

inline const PropertyInteger BlockCrops::AGE("age", 0, 7);

} // namespace mc

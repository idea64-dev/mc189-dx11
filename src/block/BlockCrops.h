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

protected:
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
    // Java: dropBlockAsItemWithChance adds a poisonous potato now and then (world drops, stage 9).
    Item* getSeed() const override;
    Item* getCrop() const override;
};

inline const PropertyInteger BlockCrops::AGE("age", 0, 7);

} // namespace mc

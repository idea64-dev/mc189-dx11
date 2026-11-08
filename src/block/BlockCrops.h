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
    // Java: getSeed / getCrop are the carrot item (stage 4 items).
};

class BlockPotato : public BlockCrops {
public:
    MC_BLOCK_CLASS(BlockPotato)
    // Java: getSeed / getCrop are the potato item; a poisonous potato may drop (stage 9).
};

inline const PropertyInteger BlockCrops::AGE("age", 0, 7);

} // namespace mc

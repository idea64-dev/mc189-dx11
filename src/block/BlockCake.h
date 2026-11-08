#pragma once

// Original: net/minecraft/block/BlockCake.java

#include "block/BlockBasics.h"

namespace mc {

class BlockCake : public Block {
public:
    MC_BLOCK_CLASS(BlockCake)

    static const PropertyInteger BITES;

    BlockCake()
        : Block(Material::cake)
    {
    }

    void setBlockBoundsForItemRender() override
    {
        const float f = 0.0625f;
        const float f1 = 0.5f;
        setBlockBounds(f, 0.0f, f, 1.0f - f, f1, 1.0f - f);
    }
    bool isFullCube() const override { return false; }
    bool isOpaqueCube() const override { return false; }
    int quantityDropped(Random&) const override { return 0; }
    EnumWorldBlockLayer getBlockLayer() const override { return EnumWorldBlockLayer::CUTOUT; }
    const IBlockState* getStateFromMeta(int meta) const override { return getDefaultState()->withProperty(BITES, meta); }
    int getMetaFromState(const IBlockState* state) const override { return state->getValue(BITES); }
    bool hasComparatorInputOverride() const override { return true; }

protected:
    void construct() override
    {
        Block::construct();
        setDefaultState(blockState->getBaseState()->withProperty(BITES, 0));
        setTickRandomly(true);
    }
    std::unique_ptr<BlockState> createBlockState() override { return makeBlockState({ &BITES }); }
};

inline const PropertyInteger BlockCake::BITES("bites", 0, 6);

} // namespace mc

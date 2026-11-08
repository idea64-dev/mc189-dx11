#pragma once

// Original: net/minecraft/block/BlockCactus.java

#include "block/BlockBasics.h"

namespace mc {

class BlockCactus : public Block {
public:
    MC_BLOCK_CLASS(BlockCactus)

    static const PropertyInteger AGE;

    BlockCactus()
        : Block(Material::cactus)
    {
    }

    bool isFullCube() const override { return false; }
    bool isOpaqueCube() const override { return false; }
    EnumWorldBlockLayer getBlockLayer() const override { return EnumWorldBlockLayer::CUTOUT; }
    const IBlockState* getStateFromMeta(int meta) const override { return getDefaultState()->withProperty(AGE, meta); }
    int getMetaFromState(const IBlockState* state) const override { return state->getValue(AGE); }

protected:
    void construct() override
    {
        Block::construct();
        setDefaultState(blockState->getBaseState()->withProperty(AGE, 0));
        setTickRandomly(true);
        setCreativeTab(&CreativeTabs::tabDecorations);
    }
    std::unique_ptr<BlockState> createBlockState() override { return makeBlockState({ &AGE }); }
};

inline const PropertyInteger BlockCactus::AGE("age", 0, 15);

} // namespace mc

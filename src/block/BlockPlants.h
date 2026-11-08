#pragma once

// BlockBush subclasses without much state: net/minecraft/block/BlockLilyPad.java, BlockMushroom.java and
// BlockNetherWart.java

#include "block/BlockBush.h"

namespace mc {

class BlockLilyPad : public BlockBush {
public:
    MC_BLOCK_CLASS(BlockLilyPad)

    int getBlockColor() const override { return 7455580; }
    int getRenderColor(const IBlockState*) const override { return 7455580; }
    int getMetaFromState(const IBlockState*) const override { return 0; }

protected:
    void construct() override
    {
        BlockBush::construct();
        const float f = 0.5f;
        const float f1 = 0.015625f;
        setBlockBounds(0.5f - f, 0.0f, 0.5f - f, 0.5f + f, f1, 0.5f + f);
        setCreativeTab(&CreativeTabs::tabDecorations);
    }
};

class BlockMushroom : public BlockBush {
public:
    MC_BLOCK_CLASS(BlockMushroom)

protected:
    void construct() override
    {
        BlockBush::construct();
        const float f = 0.2f;
        setBlockBounds(0.5f - f, 0.0f, 0.5f - f, 0.5f + f, f * 2.0f, 0.5f + f);
        setTickRandomly(true);
    }
};

class BlockNetherWart : public BlockBush {
public:
    MC_BLOCK_CLASS(BlockNetherWart)

    static const PropertyInteger AGE;

    BlockNetherWart()
        : BlockBush(Material::plants, MapColor::redColor)
    {
    }

    int quantityDropped(Random&) const override { return 0; }
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
    }
    std::unique_ptr<BlockState> createBlockState() override { return makeBlockState({ &AGE }); }
};

inline const PropertyInteger BlockNetherWart::AGE("age", 0, 3);

} // namespace mc

#pragma once

// Original: net/minecraft/block/BlockPane.java and BlockStainedGlassPane.java

#include "block/BlockBasics.h"
#include "item/EnumDyeColor.h"

namespace mc {

class BlockPane : public Block {
public:
    MC_BLOCK_CLASS(BlockPane)

    static const PropertyBool NORTH;
    static const PropertyBool EAST;
    static const PropertyBool SOUTH;
    static const PropertyBool WEST;

    BlockPane(const Material& materialIn, bool canDropIn)
        : Block(materialIn)
        , canDropArg(canDropIn)
    {
    }

    bool isOpaqueCube() const override { return false; }
    bool isFullCube() const override { return false; }
    void setBlockBoundsForItemRender() override { setBlockBounds(0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f); }
    bool canSilkHarvest() const override { return true; }
    EnumWorldBlockLayer getBlockLayer() const override { return EnumWorldBlockLayer::CUTOUT_MIPPED; }
    int getMetaFromState(const IBlockState*) const override { return 0; }

protected:
    void construct() override
    {
        Block::construct();
        setDefaultState(blockState->getBaseState()->withProperty(NORTH, false)->withProperty(EAST, false)->withProperty(SOUTH, false)->withProperty(WEST, false));
        canDrop = canDropArg;
        setCreativeTab(&CreativeTabs::tabDecorations);
    }
    std::unique_ptr<BlockState> createBlockState() override { return makeBlockState({ &NORTH, &EAST, &WEST, &SOUTH }); }

    bool canDrop = false;

private:
    bool canDropArg;
};

class BlockStainedGlassPane : public BlockPane {
public:
    MC_BLOCK_CLASS(BlockStainedGlassPane)

    static const PropertyEnum<EnumDyeColor> COLOR;

    BlockStainedGlassPane()
        : BlockPane(Material::glass, false)
    {
    }

    int damageDropped(const IBlockState* state) const override { return getMetadata(state->getValue(COLOR)); }
    const MapColor& getMapColor(const IBlockState* state) const override { return *info(state->getValue(COLOR)).mapColor; }
    EnumWorldBlockLayer getBlockLayer() const override { return EnumWorldBlockLayer::TRANSLUCENT; }
    const IBlockState* getStateFromMeta(int meta) const override { return getDefaultState()->withProperty(COLOR, dyeColorByMetadata(meta)); }
    int getMetaFromState(const IBlockState* state) const override { return getMetadata(state->getValue(COLOR)); }

protected:
    void construct() override
    {
        BlockPane::construct();
        setDefaultState(blockState->getBaseState()->withProperty(NORTH, false)->withProperty(EAST, false)->withProperty(SOUTH, false)->withProperty(WEST, false)->withProperty(COLOR, EnumDyeColor::WHITE));
        setCreativeTab(&CreativeTabs::tabDecorations);
    }
    std::unique_ptr<BlockState> createBlockState() override { return makeBlockState({ &NORTH, &EAST, &WEST, &SOUTH, &COLOR }); }
};

inline const PropertyBool BlockPane::NORTH("north");
inline const PropertyBool BlockPane::EAST("east");
inline const PropertyBool BlockPane::SOUTH("south");
inline const PropertyBool BlockPane::WEST("west");
inline const PropertyEnum<EnumDyeColor> BlockStainedGlassPane::COLOR("color");

} // namespace mc

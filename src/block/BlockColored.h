#pragma once

// Original: net/minecraft/block/BlockColored.java (wool and stained hardened clay)

#include "block/BlockBasics.h"
#include "item/EnumDyeColor.h"

namespace mc {

class BlockColored : public Block {
public:
    MC_BLOCK_CLASS(BlockColored)

    static const PropertyEnum<EnumDyeColor> COLOR;

    explicit BlockColored(const Material& materialIn)
        : Block(materialIn)
    {
    }

    int damageDropped(const IBlockState* state) const override { return getMetadata(state->getValue(COLOR)); }
    const MapColor& getMapColor(const IBlockState* state) const override { return *info(state->getValue(COLOR)).mapColor; }
    const IBlockState* getStateFromMeta(int meta) const override { return getDefaultState()->withProperty(COLOR, dyeColorByMetadata(meta)); }
    int getMetaFromState(const IBlockState* state) const override { return getMetadata(state->getValue(COLOR)); }

protected:
    void construct() override
    {
        Block::construct();
        setDefaultState(blockState->getBaseState()->withProperty(COLOR, EnumDyeColor::WHITE));
        setCreativeTab(&CreativeTabs::tabBlock);
    }
    std::unique_ptr<BlockState> createBlockState() override { return makeBlockState({ &COLOR }); }
};

inline const PropertyEnum<EnumDyeColor> BlockColored::COLOR("color");

} // namespace mc

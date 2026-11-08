#pragma once

// Original: net/minecraft/block/BlockCarpet.java

#include "block/BlockBasics.h"
#include "item/EnumDyeColor.h"

namespace mc {

class BlockCarpet : public Block {
public:
    MC_BLOCK_CLASS(BlockCarpet)

    static const PropertyEnum<EnumDyeColor> COLOR;

    BlockCarpet()
        : Block(Material::carpet)
    {
    }

    const MapColor& getMapColor(const IBlockState* state) const override { return *info(state->getValue(COLOR)).mapColor; }
    bool isOpaqueCube() const override { return false; }
    bool isFullCube() const override { return false; }
    void setBlockBoundsForItemRender() override { setBlockBoundsFromMeta(0); }
    int damageDropped(const IBlockState* state) const override { return getMetadata(state->getValue(COLOR)); }
    const IBlockState* getStateFromMeta(int meta) const override { return getDefaultState()->withProperty(COLOR, dyeColorByMetadata(meta)); }
    int getMetaFromState(const IBlockState* state) const override { return getMetadata(state->getValue(COLOR)); }
    void getSubBlocks(Item* item, const CreativeTabs*, std::vector<ItemStack>& list) const override { addSubBlocks(item, list, 0, 16); }

protected:
    void construct() override
    {
        Block::construct();
        setDefaultState(blockState->getBaseState()->withProperty(COLOR, EnumDyeColor::WHITE));
        setBlockBounds(0.0f, 0.0f, 0.0f, 1.0f, 0.0625f, 1.0f);
        setTickRandomly(true);
        setCreativeTab(&CreativeTabs::tabDecorations);
        setBlockBoundsFromMeta(0);
    }
    std::unique_ptr<BlockState> createBlockState() override { return makeBlockState({ &COLOR }); }

    void setBlockBoundsFromMeta(int /*meta*/)
    {
        const int i = 0;
        const float f = static_cast<float>(1 * (1 + i)) / 16.0f;
        setBlockBounds(0.0f, 0.0f, 0.0f, 1.0f, f, 1.0f);
    }
};

inline const PropertyEnum<EnumDyeColor> BlockCarpet::COLOR("color");

} // namespace mc

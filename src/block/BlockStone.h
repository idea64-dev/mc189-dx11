#pragma once

// Original: net/minecraft/block/BlockStone.java

#include "block/BlockBasics.h"

namespace mc {

class BlockStone : public Block {
public:
    MC_BLOCK_CLASS(BlockStone)

    enum class EnumType { STONE, GRANITE, GRANITE_SMOOTH, DIORITE, DIORITE_SMOOTH, ANDESITE, ANDESITE_SMOOTH };
    struct EnumTypeInfo {
        int meta;
        const MapColor* mapColor;
        std::string_view name;
        std::string_view unlocalizedName;
    };
    static constexpr EnumTypeInfo kTypes[7] {
        { 0, &MapColor::stoneColor, "stone", "stone" },
        { 1, &MapColor::dirtColor, "granite", "granite" },
        { 2, &MapColor::dirtColor, "smooth_granite", "graniteSmooth" },
        { 3, &MapColor::quartzColor, "diorite", "diorite" },
        { 4, &MapColor::quartzColor, "smooth_diorite", "dioriteSmooth" },
        { 5, &MapColor::stoneColor, "andesite", "andesite" },
        { 6, &MapColor::stoneColor, "smooth_andesite", "andesiteSmooth" },
    };
    static const EnumTypeInfo& info(EnumType type) { return kTypes[static_cast<int>(type)]; }
    static EnumType byMetadata(int meta) { return static_cast<EnumType>(meta < 0 || meta >= 7 ? 0 : meta); }

    static const PropertyEnum<EnumType> VARIANT;

    BlockStone()
        : Block(Material::rock)
    {
    }

    const MapColor& getMapColor(const IBlockState* state) const override { return *info(state->getValue(VARIANT)).mapColor; }
    int damageDropped(const IBlockState* state) const override { return info(state->getValue(VARIANT)).meta; }
    const IBlockState* getStateFromMeta(int meta) const override { return getDefaultState()->withProperty(VARIANT, byMetadata(meta)); }
    int getMetaFromState(const IBlockState* state) const override { return info(state->getValue(VARIANT)).meta; }
    void getSubBlocks(Item* item, const CreativeTabs*, std::vector<ItemStack>& list) const override { addSubBlocks(item, list, 0, 7); }
    Item* getItemDropped(const IBlockState* state, Random&, int) const override { return Item::getItemFromBlock(state->getValue(VARIANT) == EnumType::STONE ? Blocks::cobblestone : Blocks::stone); }

protected:
    void construct() override
    {
        Block::construct();
        setDefaultState(blockState->getBaseState()->withProperty(VARIANT, EnumType::STONE));
        setCreativeTab(&CreativeTabs::tabBlock);
    }
    std::unique_ptr<BlockState> createBlockState() override { return makeBlockState({ &VARIANT }); }
};

MC_ENUM_TRAITS(BlockStone::EnumType, 7, BlockStone::info(value).name)

inline const PropertyEnum<BlockStone::EnumType> BlockStone::VARIANT("variant");

} // namespace mc

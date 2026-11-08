#pragma once

// Original: net/minecraft/block/BlockPlanks.java

#include "block/BlockBasics.h"

namespace mc {

class BlockPlanks : public Block {
public:
    MC_BLOCK_CLASS(BlockPlanks)

    enum class EnumType { OAK, SPRUCE, BIRCH, JUNGLE, ACACIA, DARK_OAK };
    struct EnumTypeInfo {
        int meta;
        std::string_view name;
        std::string_view unlocalizedName;
        const MapColor* mapColor;
    };
    static constexpr EnumTypeInfo kTypes[6] {
        { 0, "oak", "oak", &MapColor::woodColor },
        { 1, "spruce", "spruce", &MapColor::obsidianColor },
        { 2, "birch", "birch", &MapColor::sandColor },
        { 3, "jungle", "jungle", &MapColor::dirtColor },
        { 4, "acacia", "acacia", &MapColor::adobeColor },
        { 5, "dark_oak", "big_oak", &MapColor::brownColor },
    };
    static const EnumTypeInfo& info(EnumType type) { return kTypes[static_cast<int>(type)]; }
    static int getMetadata(EnumType type) { return info(type).meta; }
    static const MapColor& getMapColor(EnumType type) { return *info(type).mapColor; }
    static EnumType byMetadata(int meta) { return static_cast<EnumType>(meta < 0 || meta >= 6 ? 0 : meta); }

    static const PropertyEnum<EnumType> VARIANT;

    BlockPlanks()
        : Block(Material::wood)
    {
    }

    int damageDropped(const IBlockState* state) const override { return getMetadata(state->getValue(VARIANT)); }
    const IBlockState* getStateFromMeta(int meta) const override { return getDefaultState()->withProperty(VARIANT, byMetadata(meta)); }
    const MapColor& getMapColor(const IBlockState* state) const override { return getMapColor(state->getValue(VARIANT)); }
    int getMetaFromState(const IBlockState* state) const override { return getMetadata(state->getValue(VARIANT)); }

protected:
    void construct() override
    {
        Block::construct();
        setDefaultState(blockState->getBaseState()->withProperty(VARIANT, EnumType::OAK));
        setCreativeTab(&CreativeTabs::tabBlock);
    }
    std::unique_ptr<BlockState> createBlockState() override { return makeBlockState({ &VARIANT }); }
};

MC_ENUM_TRAITS(BlockPlanks::EnumType, 6, BlockPlanks::info(value).name)

inline const PropertyEnum<BlockPlanks::EnumType> BlockPlanks::VARIANT("variant");

} // namespace mc

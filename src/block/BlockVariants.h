#pragma once

// Plain blocks with a variant enum: net/minecraft/block/BlockPrismarine.java, BlockQuartz.java,
// BlockSandStone.java, BlockRedSandstone.java, BlockStoneBrick.java and BlockSponge.java

#include "block/BlockBasics.h"

namespace mc {

class BlockPrismarine : public Block {
public:
    MC_BLOCK_CLASS(BlockPrismarine)

    enum class EnumType { ROUGH, BRICKS, DARK };
    struct EnumTypeInfo {
        int meta;
        std::string_view name;
        std::string_view unlocalizedName;
    };
    static constexpr EnumTypeInfo kTypes[3] { { 0, "prismarine", "rough" }, { 1, "prismarine_bricks", "bricks" }, { 2, "dark_prismarine", "dark" } };
    static const EnumTypeInfo& info(EnumType type) { return kTypes[static_cast<int>(type)]; }
    static EnumType byMetadata(int meta) { return static_cast<EnumType>(meta < 0 || meta >= 3 ? 0 : meta); }

    static const PropertyEnum<EnumType> VARIANT;
    static constexpr int ROUGH_META = 0;
    static constexpr int BRICKS_META = 1;
    static constexpr int DARK_META = 2;

    BlockPrismarine()
        : Block(Material::rock)
    {
    }

    const MapColor& getMapColor(const IBlockState* state) const override
    {
        return state->getValue(VARIANT) == EnumType::ROUGH ? MapColor::cyanColor : MapColor::diamondColor;
    }
    int damageDropped(const IBlockState* state) const override { return info(state->getValue(VARIANT)).meta; }
    int getMetaFromState(const IBlockState* state) const override { return info(state->getValue(VARIANT)).meta; }
    const IBlockState* getStateFromMeta(int meta) const override { return getDefaultState()->withProperty(VARIANT, byMetadata(meta)); }

protected:
    void construct() override
    {
        Block::construct();
        setDefaultState(blockState->getBaseState()->withProperty(VARIANT, EnumType::ROUGH));
        setCreativeTab(&CreativeTabs::tabBlock);
    }
    std::unique_ptr<BlockState> createBlockState() override { return makeBlockState({ &VARIANT }); }
};

class BlockQuartz : public Block {
public:
    MC_BLOCK_CLASS(BlockQuartz)

    enum class EnumType { DEFAULT, CHISELED, LINES_Y, LINES_X, LINES_Z };
    struct EnumTypeInfo {
        int meta;
        std::string_view name;
        std::string_view unlocalizedName;
    };
    static constexpr EnumTypeInfo kTypes[5] {
        { 0, "default", "default" }, { 1, "chiseled", "chiseled" }, { 2, "lines_y", "lines" }, { 3, "lines_x", "lines" }, { 4, "lines_z", "lines" },
    };
    static const EnumTypeInfo& info(EnumType type) { return kTypes[static_cast<int>(type)]; }
    static EnumType byMetadata(int meta) { return static_cast<EnumType>(meta < 0 || meta >= 5 ? 0 : meta); }

    static const PropertyEnum<EnumType> VARIANT;

    BlockQuartz()
        : Block(Material::rock)
    {
    }

    int damageDropped(const IBlockState* state) const override
    {
        const EnumType type = state->getValue(VARIANT);
        return type != EnumType::LINES_X && type != EnumType::LINES_Z ? info(type).meta : info(EnumType::LINES_Y).meta;
    }
    const MapColor& getMapColor(const IBlockState*) const override { return MapColor::quartzColor; }
    const IBlockState* getStateFromMeta(int meta) const override { return getDefaultState()->withProperty(VARIANT, byMetadata(meta)); }
    int getMetaFromState(const IBlockState* state) const override { return info(state->getValue(VARIANT)).meta; }

protected:
    void construct() override
    {
        Block::construct();
        setDefaultState(blockState->getBaseState()->withProperty(VARIANT, EnumType::DEFAULT));
        setCreativeTab(&CreativeTabs::tabBlock);
    }
    std::unique_ptr<BlockState> createBlockState() override { return makeBlockState({ &VARIANT }); }
};

MC_ENUM_TRAITS(BlockPrismarine::EnumType, 3, BlockPrismarine::info(value).name)
MC_ENUM_TRAITS(BlockQuartz::EnumType, 5, BlockQuartz::info(value).name)

inline const PropertyEnum<BlockPrismarine::EnumType> BlockPrismarine::VARIANT("variant");
inline const PropertyEnum<BlockQuartz::EnumType> BlockQuartz::VARIANT("variant");

} // namespace mc

#pragma once

// Original: net/minecraft/block/BlockSand.java, BlockSandStone.java and BlockRedSandstone.java

#include "block/BlockFalling.h"

namespace mc {

class BlockSand : public BlockFalling {
public:
    MC_BLOCK_CLASS(BlockSand)

    enum class EnumType { SAND, RED_SAND };
    struct EnumTypeInfo {
        int meta;
        std::string_view name;
        std::string_view unlocalizedName;
        const MapColor* mapColor;
    };
    static constexpr EnumTypeInfo kTypes[2] { { 0, "sand", "default", &MapColor::sandColor }, { 1, "red_sand", "red", &MapColor::adobeColor } };
    static const EnumTypeInfo& info(EnumType type) { return kTypes[static_cast<int>(type)]; }
    static EnumType byMetadata(int meta) { return static_cast<EnumType>(meta < 0 || meta >= 2 ? 0 : meta); }

    static const PropertyEnum<EnumType> VARIANT;

    int damageDropped(const IBlockState* state) const override { return info(state->getValue(VARIANT)).meta; }
    const MapColor& getMapColor(const IBlockState* state) const override { return *info(state->getValue(VARIANT)).mapColor; }
    const IBlockState* getStateFromMeta(int meta) const override { return getDefaultState()->withProperty(VARIANT, byMetadata(meta)); }
    int getMetaFromState(const IBlockState* state) const override { return info(state->getValue(VARIANT)).meta; }

protected:
    void construct() override
    {
        BlockFalling::construct();
        setDefaultState(blockState->getBaseState()->withProperty(VARIANT, EnumType::SAND));
    }
    std::unique_ptr<BlockState> createBlockState() override { return makeBlockState({ &VARIANT }); }
};

// The two sandstones share their layout.
struct SandstoneTypeInfo {
    int meta;
    std::string_view name;
    std::string_view unlocalizedName;
};

class BlockSandStone : public Block {
public:
    MC_BLOCK_CLASS(BlockSandStone)

    enum class EnumType { DEFAULT, CHISELED, SMOOTH };
    static constexpr SandstoneTypeInfo kTypes[3] { { 0, "sandstone", "default" }, { 1, "chiseled_sandstone", "chiseled" }, { 2, "smooth_sandstone", "smooth" } };
    static const SandstoneTypeInfo& info(EnumType type) { return kTypes[static_cast<int>(type)]; }
    static EnumType byMetadata(int meta) { return static_cast<EnumType>(meta < 0 || meta >= 3 ? 0 : meta); }

    static const PropertyEnum<EnumType> TYPE;

    BlockSandStone()
        : Block(Material::rock)
    {
    }

    int damageDropped(const IBlockState* state) const override { return info(state->getValue(TYPE)).meta; }
    const MapColor& getMapColor(const IBlockState*) const override { return MapColor::sandColor; }
    const IBlockState* getStateFromMeta(int meta) const override { return getDefaultState()->withProperty(TYPE, byMetadata(meta)); }
    int getMetaFromState(const IBlockState* state) const override { return info(state->getValue(TYPE)).meta; }

protected:
    void construct() override
    {
        Block::construct();
        setDefaultState(blockState->getBaseState()->withProperty(TYPE, EnumType::DEFAULT));
        setCreativeTab(&CreativeTabs::tabBlock);
    }
    std::unique_ptr<BlockState> createBlockState() override { return makeBlockState({ &TYPE }); }
};

class BlockRedSandstone : public Block {
public:
    MC_BLOCK_CLASS(BlockRedSandstone)

    enum class EnumType { DEFAULT, CHISELED, SMOOTH };
    static constexpr SandstoneTypeInfo kTypes[3] {
        { 0, "red_sandstone", "default" }, { 1, "chiseled_red_sandstone", "chiseled" }, { 2, "smooth_red_sandstone", "smooth" },
    };
    static const SandstoneTypeInfo& info(EnumType type) { return kTypes[static_cast<int>(type)]; }
    static EnumType byMetadata(int meta) { return static_cast<EnumType>(meta < 0 || meta >= 3 ? 0 : meta); }

    static const PropertyEnum<EnumType> TYPE;

    BlockRedSandstone()
        : Block(Material::rock, *BlockSand::info(BlockSand::EnumType::RED_SAND).mapColor)
    {
    }

    int damageDropped(const IBlockState* state) const override { return info(state->getValue(TYPE)).meta; }
    const IBlockState* getStateFromMeta(int meta) const override { return getDefaultState()->withProperty(TYPE, byMetadata(meta)); }
    int getMetaFromState(const IBlockState* state) const override { return info(state->getValue(TYPE)).meta; }

protected:
    void construct() override
    {
        Block::construct();
        setDefaultState(blockState->getBaseState()->withProperty(TYPE, EnumType::DEFAULT));
        setCreativeTab(&CreativeTabs::tabBlock);
    }
    std::unique_ptr<BlockState> createBlockState() override { return makeBlockState({ &TYPE }); }
};

MC_ENUM_TRAITS(BlockSand::EnumType, 2, BlockSand::info(value).name)
MC_ENUM_TRAITS(BlockSandStone::EnumType, 3, BlockSandStone::info(value).name)
MC_ENUM_TRAITS(BlockRedSandstone::EnumType, 3, BlockRedSandstone::info(value).name)

inline const PropertyEnum<BlockSand::EnumType> BlockSand::VARIANT("variant");
inline const PropertyEnum<BlockSandStone::EnumType> BlockSandStone::TYPE("type");
inline const PropertyEnum<BlockRedSandstone::EnumType> BlockRedSandstone::TYPE("type");

} // namespace mc

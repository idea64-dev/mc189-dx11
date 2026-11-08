#pragma once

// Original: net/minecraft/block/BlockGrass.java, BlockMycelium.java and BlockDirt.java

#include "block/BlockBasics.h"
#include "item/Item.h"
#include "world/Colorizer.h"

namespace mc {

class BlockGrass : public Block {
public:
    MC_BLOCK_CLASS(BlockGrass)

    static const PropertyBool SNOWY;

    BlockGrass()
        : Block(Material::grass)
    {
    }

    int getBlockColor() const override { return ColorizerGrass::getGrassColor(0.5, 1.0); }
    int getRenderColor(const IBlockState*) const override { return getBlockColor(); }
    EnumWorldBlockLayer getBlockLayer() const override { return EnumWorldBlockLayer::CUTOUT_MIPPED; }
    int getMetaFromState(const IBlockState*) const override { return 0; }

protected:
    void construct() override
    {
        Block::construct();
        setDefaultState(blockState->getBaseState()->withProperty(SNOWY, false));
        setTickRandomly(true);
        setCreativeTab(&CreativeTabs::tabBlock);
    }
    std::unique_ptr<BlockState> createBlockState() override { return makeBlockState({ &SNOWY }); }
};

class BlockMycelium : public Block {
public:
    MC_BLOCK_CLASS(BlockMycelium)

    static const PropertyBool SNOWY;

    BlockMycelium()
        : Block(Material::grass, MapColor::purpleColor)
    {
    }

    int getMetaFromState(const IBlockState*) const override { return 0; }

protected:
    void construct() override
    {
        Block::construct();
        setDefaultState(blockState->getBaseState()->withProperty(SNOWY, false));
        setTickRandomly(true);
        setCreativeTab(&CreativeTabs::tabBlock);
    }
    std::unique_ptr<BlockState> createBlockState() override { return makeBlockState({ &SNOWY }); }
};

class BlockDirt : public Block {
public:
    MC_BLOCK_CLASS(BlockDirt)

    enum class DirtType { DIRT, COARSE_DIRT, PODZOL };
    struct DirtTypeInfo {
        int metadata;
        std::string_view name;
        std::string_view unlocalizedName;
        const MapColor* mapColor;
    };
    static constexpr DirtTypeInfo kTypes[3] {
        { 0, "dirt", "default", &MapColor::dirtColor },
        { 1, "coarse_dirt", "coarse", &MapColor::dirtColor },
        { 2, "podzol", "podzol", &MapColor::obsidianColor },
    };
    static const DirtTypeInfo& info(DirtType type) { return kTypes[static_cast<int>(type)]; }
    static DirtType byMetadata(int meta) { return static_cast<DirtType>(meta < 0 || meta >= 3 ? 0 : meta); }

    static const PropertyEnum<DirtType> VARIANT;
    static const PropertyBool SNOWY;

    BlockDirt()
        : Block(Material::ground)
    {
    }

    const MapColor& getMapColor(const IBlockState* state) const override { return *info(state->getValue(VARIANT)).mapColor; }
    const IBlockState* getStateFromMeta(int meta) const override { return getDefaultState()->withProperty(VARIANT, byMetadata(meta)); }
    int getMetaFromState(const IBlockState* state) const override { return info(state->getValue(VARIANT)).metadata; }
    int damageDropped(const IBlockState* state) const override
    {
        DirtType type = state->getValue(VARIANT);
        if (type == DirtType::PODZOL)
            type = DirtType::DIRT;
        return info(type).metadata;
    }
    void getSubBlocks(Item*, const CreativeTabs*, std::vector<ItemStack>& list) const override { addSubBlocks(Item::getItemFromBlock(this), list, { 0, 1, 2 }); }

protected:
    void construct() override
    {
        Block::construct();
        setDefaultState(blockState->getBaseState()->withProperty(VARIANT, DirtType::DIRT)->withProperty(SNOWY, false));
        setCreativeTab(&CreativeTabs::tabBlock);
    }
    std::unique_ptr<BlockState> createBlockState() override { return makeBlockState({ &VARIANT, &SNOWY }); }
};

MC_ENUM_TRAITS(BlockDirt::DirtType, 3, BlockDirt::info(value).name)

inline const PropertyBool BlockGrass::SNOWY("snowy");
inline const PropertyBool BlockMycelium::SNOWY("snowy");
inline const PropertyEnum<BlockDirt::DirtType> BlockDirt::VARIANT("variant");
inline const PropertyBool BlockDirt::SNOWY("snowy");

} // namespace mc

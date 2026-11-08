#pragma once

// Original: net/minecraft/block/BlockHugeMushroom.java

#include "block/BlockBasics.h"
#include "util/Random.h"

#include <algorithm>

namespace mc {

class BlockHugeMushroom : public Block {
public:
    MC_BLOCK_CLASS(BlockHugeMushroom)

    enum class EnumType { NORTH_WEST, NORTH, NORTH_EAST, WEST, CENTER, EAST, SOUTH_WEST, SOUTH, SOUTH_EAST, STEM, ALL_INSIDE, ALL_OUTSIDE, ALL_STEM };
    struct EnumTypeInfo {
        int meta;
        std::string_view name;
    };
    static constexpr EnumTypeInfo kTypes[13] {
        { 1, "north_west" }, { 2, "north" }, { 3, "north_east" }, { 4, "west" }, { 5, "center" }, { 6, "east" }, { 7, "south_west" },
        { 8, "south" }, { 9, "south_east" }, { 10, "stem" }, { 0, "all_inside" }, { 14, "all_outside" }, { 15, "all_stem" },
    };
    static const EnumTypeInfo& info(EnumType type) { return kTypes[static_cast<int>(type)]; }
    // META_LOOKUP has 16 slots; 11-13 are empty and give ALL_INSIDE (slot 0).
    static EnumType byMetadata(int meta)
    {
        if (meta < 0 || meta >= 16)
            meta = 0;
        for (int i = 0; i < 13; ++i) {
            if (kTypes[i].meta == meta)
                return static_cast<EnumType>(i);
        }
        return EnumType::ALL_INSIDE;
    }

    static const PropertyEnum<EnumType> VARIANT;

    BlockHugeMushroom(const Material& materialIn, const MapColor& mapColorIn, Block* smallBlockIn)
        : Block(materialIn, mapColorIn)
        , smallBlock(smallBlockIn)
    {
    }

    int quantityDropped(Random& random) const override { return std::max(0, random.nextInt(10) - 7); }
    const MapColor& getMapColor(const IBlockState* state) const override
    {
        switch (state->getValue(VARIANT)) {
        case EnumType::ALL_STEM: return MapColor::clothColor;
        case EnumType::ALL_INSIDE: return MapColor::sandColor;
        case EnumType::STEM: return MapColor::sandColor;
        default: return Block::getMapColor(state);
        }
    }
    const IBlockState* getStateFromMeta(int meta) const override { return getDefaultState()->withProperty(VARIANT, byMetadata(meta)); }
    int getMetaFromState(const IBlockState* state) const override { return info(state->getValue(VARIANT)).meta; }
    Item* getItemDropped(const IBlockState*, Random&, int) const override { return Item::getItemFromBlock(smallBlock); }
    Item* getItem(World*, const BlockPos&) override { return Item::getItemFromBlock(smallBlock); }
    const IBlockState* onBlockPlaced(World*, const BlockPos&, EnumFacing, float, float, float, int, EntityLivingBase*) override { return getDefaultState(); }

protected:
    void construct() override
    {
        Block::construct();
        setDefaultState(blockState->getBaseState()->withProperty(VARIANT, EnumType::ALL_OUTSIDE));
    }
    std::unique_ptr<BlockState> createBlockState() override { return makeBlockState({ &VARIANT }); }

private:
    Block* smallBlock;
};

MC_ENUM_TRAITS(BlockHugeMushroom::EnumType, 13, BlockHugeMushroom::info(value).name)

inline const PropertyEnum<BlockHugeMushroom::EnumType> BlockHugeMushroom::VARIANT("variant");

} // namespace mc

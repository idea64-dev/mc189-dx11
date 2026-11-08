#pragma once

// Original: net/minecraft/block/BlockRailBase.java, BlockRail.java, BlockRailDetector.java and
// BlockRailPowered.java (BlockRailBase.Rail, the shape logic, comes with the world, stage 9)

#include "block/BlockBasics.h"

namespace mc {

class BlockRailBase : public Block {
public:
    enum class EnumRailDirection { NORTH_SOUTH, EAST_WEST, ASCENDING_EAST, ASCENDING_WEST, ASCENDING_NORTH, ASCENDING_SOUTH, SOUTH_EAST, SOUTH_WEST, NORTH_WEST, NORTH_EAST };
    static constexpr std::string_view kDirectionNames[10] { "north_south", "east_west", "ascending_east", "ascending_west", "ascending_north",
        "ascending_south", "south_east", "south_west", "north_west", "north_east" };
    static int getMetadata(EnumRailDirection direction) { return static_cast<int>(direction); }
    static bool isAscending(EnumRailDirection d)
    {
        return d == EnumRailDirection::ASCENDING_NORTH || d == EnumRailDirection::ASCENDING_EAST || d == EnumRailDirection::ASCENDING_SOUTH || d == EnumRailDirection::ASCENDING_WEST;
    }
    static EnumRailDirection byMetadata(int meta) { return static_cast<EnumRailDirection>(meta < 0 || meta >= 10 ? 0 : meta); }

    explicit BlockRailBase(bool isPoweredIn)
        : Block(Material::circuits)
        , isPoweredArg(isPoweredIn)
    {
    }

    bool isOpaqueCube() const override { return false; }
    bool isFullCube() const override { return false; }
    int getMobilityFlag() const override { return 0; }
    EnumWorldBlockLayer getBlockLayer() const override { return EnumWorldBlockLayer::CUTOUT; }

protected:
    void construct() override
    {
        Block::construct();
        isPowered = isPoweredArg;
        setBlockBounds(0.0f, 0.0f, 0.0f, 1.0f, 0.125f, 1.0f);
        setCreativeTab(&CreativeTabs::tabTransport);
    }

    bool isPowered = false;

private:
    bool isPoweredArg;
};

MC_ENUM_TRAITS(BlockRailBase::EnumRailDirection, 10, BlockRailBase::kDirectionNames[static_cast<int>(value)])

class BlockRail : public BlockRailBase {
public:
    MC_BLOCK_CLASS(BlockRail)

    static const PropertyEnum<EnumRailDirection> SHAPE;

    BlockRail()
        : BlockRailBase(false)
    {
    }

    const IBlockState* getStateFromMeta(int meta) const override { return getDefaultState()->withProperty(SHAPE, byMetadata(meta)); }
    int getMetaFromState(const IBlockState* state) const override { return getMetadata(state->getValue(SHAPE)); }

protected:
    void construct() override
    {
        BlockRailBase::construct();
        setDefaultState(blockState->getBaseState()->withProperty(SHAPE, EnumRailDirection::NORTH_SOUTH));
    }
    std::unique_ptr<BlockState> createBlockState() override { return makeBlockState({ &SHAPE }); }
};

// No curves for the powered rails.
inline bool isStraightRail(BlockRailBase::EnumRailDirection d)
{
    using D = BlockRailBase::EnumRailDirection;
    return d != D::NORTH_EAST && d != D::NORTH_WEST && d != D::SOUTH_EAST && d != D::SOUTH_WEST;
}

class BlockRailDetector : public BlockRailBase {
public:
    MC_BLOCK_CLASS(BlockRailDetector)

    static const PropertyEnum<EnumRailDirection> SHAPE;
    static const PropertyBool POWERED;

    BlockRailDetector()
        : BlockRailBase(true)
    {
    }

    bool canProvidePower() const override { return true; }
    bool hasComparatorInputOverride() const override { return true; }
    const IBlockState* getStateFromMeta(int meta) const override
    {
        return getDefaultState()->withProperty(SHAPE, byMetadata(meta & 7))->withProperty(POWERED, (meta & 8) > 0);
    }
    int getMetaFromState(const IBlockState* state) const override
    {
        int i = 0;
        i = i | getMetadata(state->getValue(SHAPE));
        if (state->getValue(POWERED))
            i |= 8;
        return i;
    }

protected:
    void construct() override
    {
        BlockRailBase::construct();
        setDefaultState(blockState->getBaseState()->withProperty(POWERED, false)->withProperty(SHAPE, EnumRailDirection::NORTH_SOUTH));
        setTickRandomly(true);
    }
    std::unique_ptr<BlockState> createBlockState() override { return makeBlockState({ &SHAPE, &POWERED }); }
};

class BlockRailPowered : public BlockRailBase {
public:
    MC_BLOCK_CLASS(BlockRailPowered)

    static const PropertyEnum<EnumRailDirection> SHAPE;
    static const PropertyBool POWERED;

    BlockRailPowered()
        : BlockRailBase(true)
    {
    }

    const IBlockState* getStateFromMeta(int meta) const override
    {
        return getDefaultState()->withProperty(SHAPE, byMetadata(meta & 7))->withProperty(POWERED, (meta & 8) > 0);
    }
    int getMetaFromState(const IBlockState* state) const override
    {
        int i = 0;
        i = i | getMetadata(state->getValue(SHAPE));
        if (state->getValue(POWERED))
            i |= 8;
        return i;
    }

protected:
    void construct() override
    {
        BlockRailBase::construct();
        setDefaultState(blockState->getBaseState()->withProperty(SHAPE, EnumRailDirection::NORTH_SOUTH)->withProperty(POWERED, false));
    }
    std::unique_ptr<BlockState> createBlockState() override { return makeBlockState({ &SHAPE, &POWERED }); }
};

inline const PropertyEnum<BlockRailBase::EnumRailDirection> BlockRail::SHAPE("shape");
inline const PropertyEnum<BlockRailBase::EnumRailDirection> BlockRailDetector::SHAPE("shape", isStraightRail);
inline const PropertyBool BlockRailDetector::POWERED("powered");
inline const PropertyEnum<BlockRailBase::EnumRailDirection> BlockRailPowered::SHAPE("shape", isStraightRail);
inline const PropertyBool BlockRailPowered::POWERED("powered");

} // namespace mc

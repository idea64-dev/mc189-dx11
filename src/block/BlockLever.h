#pragma once

// Original: net/minecraft/block/BlockLever.java

#include "block/BlockBasics.h"

namespace mc {

class BlockLever : public Block {
public:
    MC_BLOCK_CLASS(BlockLever)

    enum class EnumOrientation { DOWN_X, EAST, WEST, SOUTH, NORTH, UP_Z, UP_X, DOWN_Z };
    struct EnumOrientationInfo {
        int meta;
        std::string_view name;
        EnumFacing facing;
    };
    static constexpr EnumOrientationInfo kOrientations[8] {
        { 0, "down_x", EnumFacing::DOWN }, { 1, "east", EnumFacing::EAST }, { 2, "west", EnumFacing::WEST }, { 3, "south", EnumFacing::SOUTH },
        { 4, "north", EnumFacing::NORTH }, { 5, "up_z", EnumFacing::UP }, { 6, "up_x", EnumFacing::UP }, { 7, "down_z", EnumFacing::DOWN },
    };
    static const EnumOrientationInfo& info(EnumOrientation orientation) { return kOrientations[static_cast<int>(orientation)]; }
    static EnumOrientation byMetadata(int meta) { return static_cast<EnumOrientation>(meta < 0 || meta >= 8 ? 0 : meta); }
    // Throws std::invalid_argument for impossible combinations, like Java.
    static EnumOrientation forFacings(EnumFacing clickedSide, EnumFacing entityFacing);

    static const PropertyEnum<EnumOrientation> FACING;
    static const PropertyBool POWERED;

    BlockLever()
        : Block(Material::circuits)
    {
    }

    bool isOpaqueCube() const override { return false; }
    bool isFullCube() const override { return false; }
    bool canProvidePower() const override { return true; }
    const IBlockState* getStateFromMeta(int meta) const override
    {
        return getDefaultState()->withProperty(FACING, byMetadata(meta & 7))->withProperty(POWERED, (meta & 8) > 0);
    }
    int getMetaFromState(const IBlockState* state) const override
    {
        int i = 0;
        i = i | info(state->getValue(FACING)).meta;
        if (state->getValue(POWERED))
            i |= 8;
        return i;
    }

protected:
    void construct() override
    {
        Block::construct();
        setDefaultState(blockState->getBaseState()->withProperty(FACING, EnumOrientation::NORTH)->withProperty(POWERED, false));
        setCreativeTab(&CreativeTabs::tabRedstone);
    }
    std::unique_ptr<BlockState> createBlockState() override { return makeBlockState({ &FACING, &POWERED }); }
};

MC_ENUM_TRAITS(BlockLever::EnumOrientation, 8, BlockLever::info(value).name)

inline BlockLever::EnumOrientation BlockLever::forFacings(EnumFacing clickedSide, EnumFacing entityFacing)
{
    switch (clickedSide) {
    case EnumFacing::DOWN:
        switch (getAxis(entityFacing)) {
        case Axis::X: return EnumOrientation::DOWN_X;
        case Axis::Z: return EnumOrientation::DOWN_Z;
        default: throw std::invalid_argument("Invalid entityFacing for facing down");
        }
    case EnumFacing::UP:
        switch (getAxis(entityFacing)) {
        case Axis::X: return EnumOrientation::UP_X;
        case Axis::Z: return EnumOrientation::UP_Z;
        default: throw std::invalid_argument("Invalid entityFacing for facing up");
        }
    case EnumFacing::NORTH: return EnumOrientation::NORTH;
    case EnumFacing::SOUTH: return EnumOrientation::SOUTH;
    case EnumFacing::WEST: return EnumOrientation::WEST;
    case EnumFacing::EAST: return EnumOrientation::EAST;
    }
    throw std::invalid_argument("Invalid facing");
}

inline const PropertyEnum<BlockLever::EnumOrientation> BlockLever::FACING("facing");
inline const PropertyBool BlockLever::POWERED("powered");

} // namespace mc

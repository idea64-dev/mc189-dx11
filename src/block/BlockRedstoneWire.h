#pragma once

// Original: net/minecraft/block/BlockRedstoneWire.java (power propagation comes with the world, stage 9)

#include "block/BlockBasics.h"

namespace mc {

class BlockRedstoneWire : public Block {
public:
    MC_BLOCK_CLASS(BlockRedstoneWire)

    enum class EnumAttachPosition { UP, SIDE, NONE };
    static const PropertyEnum<EnumAttachPosition> NORTH;
    static const PropertyEnum<EnumAttachPosition> EAST;
    static const PropertyEnum<EnumAttachPosition> SOUTH;
    static const PropertyEnum<EnumAttachPosition> WEST;
    static const PropertyInteger POWER;

    BlockRedstoneWire()
        : Block(Material::circuits)
    {
    }

    bool isOpaqueCube() const override { return false; }
    bool isFullCube() const override { return false; }
    bool canProvidePower() const override { return canProvidePowerFlag; }
    EnumWorldBlockLayer getBlockLayer() const override { return EnumWorldBlockLayer::CUTOUT; }
    const IBlockState* getStateFromMeta(int meta) const override { return getDefaultState()->withProperty(POWER, meta); }
    int getMetaFromState(const IBlockState* state) const override { return state->getValue(POWER); }
    Item* getItemDropped(const IBlockState*, Random&, int) const override { return Items::redstone; }

protected:
    void construct() override
    {
        Block::construct();
        setDefaultState(blockState->getBaseState()->withProperty(NORTH, EnumAttachPosition::NONE)->withProperty(EAST, EnumAttachPosition::NONE)->withProperty(SOUTH, EnumAttachPosition::NONE)->withProperty(WEST, EnumAttachPosition::NONE)->withProperty(POWER, 0));
        setBlockBounds(0.0f, 0.0f, 0.0f, 1.0f, 0.0625f, 1.0f);
    }
    std::unique_ptr<BlockState> createBlockState() override { return makeBlockState({ &NORTH, &EAST, &SOUTH, &WEST, &POWER }); }

private:
    // Java: canProvidePower, cleared while the wire computes its own power.
    bool canProvidePowerFlag = true;
};

MC_ENUM_TRAITS(BlockRedstoneWire::EnumAttachPosition, 3,
    value == BlockRedstoneWire::EnumAttachPosition::UP ? "up" : value == BlockRedstoneWire::EnumAttachPosition::SIDE ? "side" : "none")

inline const PropertyEnum<BlockRedstoneWire::EnumAttachPosition> BlockRedstoneWire::NORTH("north");
inline const PropertyEnum<BlockRedstoneWire::EnumAttachPosition> BlockRedstoneWire::EAST("east");
inline const PropertyEnum<BlockRedstoneWire::EnumAttachPosition> BlockRedstoneWire::SOUTH("south");
inline const PropertyEnum<BlockRedstoneWire::EnumAttachPosition> BlockRedstoneWire::WEST("west");
inline const PropertyInteger BlockRedstoneWire::POWER("power", 0, 15);

} // namespace mc

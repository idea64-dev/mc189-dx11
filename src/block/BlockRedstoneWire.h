#pragma once

// Original: net/minecraft/block/BlockRedstoneWire.java

#include "block/BlockBasics.h"
#include "core/JavaHashMap.h"
#include "world/chunk/Chunk.h"

#include <optional>

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

    const IBlockState* getActualState(const IBlockState* state, IBlockAccess* worldIn, const BlockPos& pos) override;
    std::optional<AxisAlignedBB> getCollisionBoundingBox(World*, const BlockPos&, const IBlockState*) override { return std::nullopt; }
    bool isOpaqueCube() const override { return false; }
    bool isFullCube() const override { return false; }
    int colorMultiplier(IBlockAccess* worldIn, const BlockPos& pos, int renderPass) override;
    bool canPlaceBlockAt(World* worldIn, const BlockPos& pos) override;
    void onBlockAdded(World* worldIn, const BlockPos& pos, const IBlockState* state) override;
    void breakBlock(World* worldIn, const BlockPos& pos, const IBlockState* state) override;
    void onNeighborBlockChange(World* worldIn, const BlockPos& pos, const IBlockState* state, Block* neighborBlock) override;
    int getStrongPower(IBlockAccess* worldIn, const BlockPos& pos, const IBlockState* state, EnumFacing side) override;
    int getWeakPower(IBlockAccess* worldIn, const BlockPos& pos, const IBlockState* state, EnumFacing side) override;
    bool canProvidePower() const override { return canProvidePowerFlag; }
    void randomDisplayTick(World* worldIn, const BlockPos& pos, const IBlockState* state, Random& rand) override;
    Item* getItem(World*, const BlockPos&) override { return Items::redstone; }
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

    static bool canConnectUpwardsTo(IBlockAccess* worldIn, const BlockPos& pos);
    static bool canConnectUpwardsTo(const IBlockState* state);
    // Java passes a null side for the upward check.
    static bool canConnectTo(const IBlockState* blockState, std::optional<EnumFacing> side);

private:
    EnumAttachPosition getAttachPosition(IBlockAccess* worldIn, const BlockPos& pos, EnumFacing direction);
    const IBlockState* updateSurroundingRedstone(World* worldIn, const BlockPos& pos, const IBlockState* state);
    const IBlockState* calculateCurrentChanges(World* worldIn, const BlockPos& pos1, const BlockPos& pos2, const IBlockState* state);
    void notifyWireNeighborsOfStateChange(World* worldIn, const BlockPos& pos);
    int getMaxCurrentStrength(World* worldIn, const BlockPos& pos, int strength);
    bool func_176339_d(IBlockAccess* worldIn, const BlockPos& pos, EnumFacing side);
    static int colorMultiplier(int powerLevel);

    // Java: canProvidePower, cleared while the wire computes its own power.
    bool canProvidePowerFlag = true;
    // Java: a HashSet<BlockPos>; its iteration order decides the order of the neighbour updates.
    JavaHashMap<BlockPos, bool> blocksNeedingUpdate;
};

MC_ENUM_TRAITS(BlockRedstoneWire::EnumAttachPosition, 3,
    value == BlockRedstoneWire::EnumAttachPosition::UP ? "up" : value == BlockRedstoneWire::EnumAttachPosition::SIDE ? "side" : "none")

inline const PropertyEnum<BlockRedstoneWire::EnumAttachPosition> BlockRedstoneWire::NORTH("north");
inline const PropertyEnum<BlockRedstoneWire::EnumAttachPosition> BlockRedstoneWire::EAST("east");
inline const PropertyEnum<BlockRedstoneWire::EnumAttachPosition> BlockRedstoneWire::SOUTH("south");
inline const PropertyEnum<BlockRedstoneWire::EnumAttachPosition> BlockRedstoneWire::WEST("west");
inline const PropertyInteger BlockRedstoneWire::POWER("power", 0, 15);

} // namespace mc

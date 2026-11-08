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

    std::optional<AxisAlignedBB> getCollisionBoundingBox(World*, const BlockPos&, const IBlockState*) override { return std::nullopt; }
    bool isOpaqueCube() const override { return false; }
    bool isFullCube() const override { return false; }
    bool canPlaceBlockOnSide(World* worldIn, const BlockPos& pos, EnumFacing side) override;
    bool canPlaceBlockAt(World* worldIn, const BlockPos& pos) override;
    const IBlockState* onBlockPlaced(World* worldIn, const BlockPos& pos, EnumFacing facing, float hitX, float hitY, float hitZ, int meta,
        EntityLivingBase* placer) override;
    static int getMetadataForFacing(EnumFacing facing);
    void onNeighborBlockChange(World* worldIn, const BlockPos& pos, const IBlockState* state, Block* neighborBlock) override;
    void setBlockBoundsBasedOnState(IBlockAccess* worldIn, const BlockPos& pos) override;
    bool onBlockActivated(World* worldIn, const BlockPos& pos, const IBlockState* state, EntityPlayer* playerIn, EnumFacing side, float hitX,
        float hitY, float hitZ) override;
    void breakBlock(World* worldIn, const BlockPos& pos, const IBlockState* state) override;
    int getWeakPower(IBlockAccess*, const BlockPos&, const IBlockState* state, EnumFacing) override { return state->getValue(POWERED) ? 15 : 0; }
    int getStrongPower(IBlockAccess*, const BlockPos&, const IBlockState* state, EnumFacing side) override
    {
        return !state->getValue(POWERED) ? 0 : (info(state->getValue(FACING)).facing == side ? 15 : 0);
    }
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

    static bool func_181090_a(World* worldIn, const BlockPos& pos, EnumFacing side);

private:
    bool func_181091_e(World* worldIn, const BlockPos& pos, const IBlockState* state);
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

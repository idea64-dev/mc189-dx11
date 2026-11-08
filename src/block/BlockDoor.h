#pragma once

// Original: net/minecraft/block/BlockDoor.java

#include "block/BlockBasics.h"

namespace mc {

class BlockDoor : public Block {
public:
    MC_BLOCK_CLASS(BlockDoor)

    enum class EnumDoorHalf { UPPER, LOWER };
    enum class EnumHingePosition { LEFT, RIGHT };

    static const PropertyDirection FACING;
    static const PropertyBool OPEN;
    static const PropertyEnum<EnumHingePosition> HINGE;
    static const PropertyBool POWERED;
    static const PropertyEnum<EnumDoorHalf> HALF;

    explicit BlockDoor(const Material& materialIn)
        : Block(materialIn)
    {
    }

    bool isOpaqueCube() const override { return false; }
    bool isPassable(IBlockAccess* worldIn, const BlockPos& pos) override;
    bool isFullCube() const override { return false; }
    AxisAlignedBB getSelectedBoundingBox(World* worldIn, const BlockPos& pos) override;
    std::optional<AxisAlignedBB> getCollisionBoundingBox(World* worldIn, const BlockPos& pos, const IBlockState* state) override;
    void setBlockBoundsBasedOnState(IBlockAccess* worldIn, const BlockPos& pos) override;
    bool onBlockActivated(World* worldIn, const BlockPos& pos, const IBlockState* state, EntityPlayer* playerIn, EnumFacing side, float hitX,
        float hitY, float hitZ) override;
    void toggleDoor(World* worldIn, const BlockPos& pos, bool open);
    void onNeighborBlockChange(World* worldIn, const BlockPos& pos, const IBlockState* state, Block* neighborBlock) override;
    std::optional<MovingObjectPosition> collisionRayTrace(World* worldIn, const BlockPos& pos, const Vec3& start, const Vec3& end) override;
    bool canPlaceBlockAt(World* worldIn, const BlockPos& pos) override;
    int getMobilityFlag() const override { return 1; }
    static int combineMetadata(IBlockAccess* worldIn, const BlockPos& pos);
    Item* getItem(World*, const BlockPos&) override { return getDoorItem(); }
    // Java: onBlockHarvested removes the lower half for creative players -- the player comes with stage 8.
    const IBlockState* getActualState(const IBlockState* state, IBlockAccess* worldIn, const BlockPos& pos) override;
    static bool isOpen(IBlockAccess* worldIn, const BlockPos& pos);
    static EnumFacing getFacing(IBlockAccess* worldIn, const BlockPos& pos);
    static EnumFacing getFacing(int combinedMeta) { return rotateYCCW(getHorizontal(combinedMeta & 3)); }
    EnumWorldBlockLayer getBlockLayer() const override { return EnumWorldBlockLayer::CUTOUT; }
    const IBlockState* getStateFromMeta(int meta) const override
    {
        return (meta & 8) > 0
            ? getDefaultState()->withProperty(HALF, EnumDoorHalf::UPPER)->withProperty(HINGE, (meta & 1) > 0 ? EnumHingePosition::RIGHT : EnumHingePosition::LEFT)->withProperty(POWERED, (meta & 2) > 0)
            : getDefaultState()->withProperty(HALF, EnumDoorHalf::LOWER)->withProperty(FACING, rotateYCCW(getHorizontal(meta & 3)))->withProperty(OPEN, (meta & 4) > 0);
    }
    int getMetaFromState(const IBlockState* state) const override
    {
        int i = 0;
        if (state->getValue(HALF) == EnumDoorHalf::UPPER) {
            i = i | 8;
            if (state->getValue(HINGE) == EnumHingePosition::RIGHT)
                i |= 1;
            if (state->getValue(POWERED))
                i |= 2;
        } else {
            i = i | getHorizontalIndex(rotateY(state->getValue(FACING)));
            if (state->getValue(OPEN))
                i |= 4;
        }
        return i;
    }
    Item* getItemDropped(const IBlockState* state, Random&, int) const override { return state->getValue(HALF) == EnumDoorHalf::UPPER ? nullptr : getDoorItem(); }

protected:
    void construct() override
    {
        Block::construct();
        setDefaultState(blockState->getBaseState()->withProperty(FACING, EnumFacing::NORTH)->withProperty(OPEN, false)->withProperty(HINGE, EnumHingePosition::LEFT)->withProperty(POWERED, false)->withProperty(HALF, EnumDoorHalf::LOWER));
    }
    std::unique_ptr<BlockState> createBlockState() override { return makeBlockState({ &HALF, &FACING, &OPEN, &HINGE, &POWERED }); }

    static int removeHalfBit(int meta) { return meta & 7; }
    static bool isOpen(int combinedMeta) { return (combinedMeta & 4) != 0; }
    static bool isTop(int meta) { return (meta & 8) != 0; }
    static bool isHingeLeft(int combinedMeta) { return (combinedMeta & 16) != 0; }

private:
    void setBoundBasedOnMeta(int combinedMeta);
    // Java: getItem() -- the door item of this door block.
    Item* getDoorItem() const;
};

MC_ENUM_TRAITS(BlockDoor::EnumDoorHalf, 2, value == BlockDoor::EnumDoorHalf::UPPER ? "upper" : "lower")
MC_ENUM_TRAITS(BlockDoor::EnumHingePosition, 2, value == BlockDoor::EnumHingePosition::LEFT ? "left" : "right")

inline const PropertyDirection BlockDoor::FACING("facing", isHorizontalFacing);
inline const PropertyBool BlockDoor::OPEN("open");
inline const PropertyEnum<BlockDoor::EnumHingePosition> BlockDoor::HINGE("hinge");
inline const PropertyBool BlockDoor::POWERED("powered");
inline const PropertyEnum<BlockDoor::EnumDoorHalf> BlockDoor::HALF("half");

} // namespace mc

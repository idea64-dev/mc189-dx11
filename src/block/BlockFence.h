#pragma once

// Original: net/minecraft/block/BlockFence.java and BlockFenceGate.java

#include "block/BlockDirectional.h"
#include "block/BlockPlanks.h"

namespace mc {

class BlockFence : public Block {
public:
    MC_BLOCK_CLASS(BlockFence)

    static const PropertyBool NORTH;
    static const PropertyBool EAST;
    static const PropertyBool SOUTH;
    static const PropertyBool WEST;

    explicit BlockFence(const Material& materialIn)
        : BlockFence(materialIn, materialIn.getMaterialMapColor())
    {
    }
    BlockFence(const Material& materialIn, const MapColor& mapColorIn)
        : Block(materialIn, mapColorIn)
    {
    }

    void addCollisionBoxesToList(World* worldIn, const BlockPos& pos, const IBlockState* state, const AxisAlignedBB& mask,
        std::vector<AxisAlignedBB>& list, Entity* collidingEntity) override;
    void setBlockBoundsBasedOnState(IBlockAccess* worldIn, const BlockPos& pos) override;
    bool isOpaqueCube() const override { return false; }
    bool isFullCube() const override { return false; }
    bool isPassable(IBlockAccess*, const BlockPos&) override { return false; }
    bool canConnectTo(IBlockAccess* worldIn, const BlockPos& pos);
    bool shouldSideBeRendered(IBlockAccess*, const BlockPos&, EnumFacing) override { return true; }
    // Java: onBlockActivated attaches the player's leashed animals (ItemLead.attachToFence) -- stage 8/10.
    int getMetaFromState(const IBlockState*) const override { return 0; }
    const IBlockState* getActualState(const IBlockState* state, IBlockAccess* worldIn, const BlockPos& pos) override;

protected:
    void construct() override
    {
        Block::construct();
        setDefaultState(blockState->getBaseState()->withProperty(NORTH, false)->withProperty(EAST, false)->withProperty(SOUTH, false)->withProperty(WEST, false));
        setCreativeTab(&CreativeTabs::tabDecorations);
    }
    std::unique_ptr<BlockState> createBlockState() override { return makeBlockState({ &NORTH, &EAST, &WEST, &SOUTH }); }
};

class BlockFenceGate : public BlockDirectional {
public:
    MC_BLOCK_CLASS(BlockFenceGate)

    static const PropertyBool OPEN;
    static const PropertyBool POWERED;
    static const PropertyBool IN_WALL;

    explicit BlockFenceGate(BlockPlanks::EnumType type)
        : BlockDirectional(Material::wood, BlockPlanks::getMapColor(type))
    {
    }

    const IBlockState* getActualState(const IBlockState* state, IBlockAccess* worldIn, const BlockPos& pos) override;
    bool canPlaceBlockAt(World* worldIn, const BlockPos& pos) override;
    std::optional<AxisAlignedBB> getCollisionBoundingBox(World* worldIn, const BlockPos& pos, const IBlockState* state) override;
    void setBlockBoundsBasedOnState(IBlockAccess* worldIn, const BlockPos& pos) override;
    bool isOpaqueCube() const override { return false; }
    bool isFullCube() const override { return false; }
    bool isPassable(IBlockAccess* worldIn, const BlockPos& pos) override;
    const IBlockState* onBlockPlaced(World* worldIn, const BlockPos& pos, EnumFacing facing, float hitX, float hitY, float hitZ, int meta,
        EntityLivingBase* placer) override;
    bool onBlockActivated(World* worldIn, const BlockPos& pos, const IBlockState* state, EntityPlayer* playerIn, EnumFacing side, float hitX,
        float hitY, float hitZ) override;
    void onNeighborBlockChange(World* worldIn, const BlockPos& pos, const IBlockState* state, Block* neighborBlock) override;
    bool shouldSideBeRendered(IBlockAccess*, const BlockPos&, EnumFacing) override { return true; }
    const IBlockState* getStateFromMeta(int meta) const override
    {
        return getDefaultState()->withProperty(FACING, getHorizontal(meta))->withProperty(OPEN, (meta & 4) != 0)->withProperty(POWERED, (meta & 8) != 0);
    }
    int getMetaFromState(const IBlockState* state) const override
    {
        int i = 0;
        i = i | getHorizontalIndex(state->getValue(FACING));
        if (state->getValue(POWERED))
            i |= 8;
        if (state->getValue(OPEN))
            i |= 4;
        return i;
    }

protected:
    void construct() override
    {
        BlockDirectional::construct();
        setDefaultState(blockState->getBaseState()->withProperty(OPEN, false)->withProperty(POWERED, false)->withProperty(IN_WALL, false));
        setCreativeTab(&CreativeTabs::tabRedstone);
    }
    std::unique_ptr<BlockState> createBlockState() override { return makeBlockState({ &FACING, &OPEN, &POWERED, &IN_WALL }); }
};

inline const PropertyBool BlockFence::NORTH("north");
inline const PropertyBool BlockFence::EAST("east");
inline const PropertyBool BlockFence::SOUTH("south");
inline const PropertyBool BlockFence::WEST("west");
inline const PropertyBool BlockFenceGate::OPEN("open");
inline const PropertyBool BlockFenceGate::POWERED("powered");
inline const PropertyBool BlockFenceGate::IN_WALL("in_wall");

} // namespace mc

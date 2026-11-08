#pragma once

// Original: net/minecraft/block/BlockSkull.java

#include "block/BlockContainer.h"

namespace mc {

class TileEntitySkull;

class BlockSkull : public BlockContainer {
public:
    MC_BLOCK_CLASS(BlockSkull)

    static const PropertyDirection FACING;
    static const PropertyBool NODROP;

    BlockSkull()
        : BlockContainer(Material::circuits)
    {
    }

    bool isOpaqueCube() const override { return false; }
    bool isFullCube() const override { return false; }
    void setBlockBoundsBasedOnState(IBlockAccess* worldIn, const BlockPos& pos) override;
    std::optional<AxisAlignedBB> getCollisionBoundingBox(World* worldIn, const BlockPos& pos, const IBlockState* state) override;
    const IBlockState* onBlockPlaced(World* worldIn, const BlockPos& pos, EnumFacing facing, float hitX, float hitY, float hitZ, int meta,
        EntityLivingBase* placer) override;
    std::shared_ptr<TileEntity> createNewTileEntity(World* worldIn, int meta) override;
    Item* getItem(World*, const BlockPos&) override { return Items::skull; }
    int getDamageValue(World* worldIn, const BlockPos& pos) override;
    void dropBlockAsItemWithChance(World*, const BlockPos&, const IBlockState*, float, int) override { }
    // Java: onBlockHarvested sets NODROP (flag 4) for creative players -- the player comes with stage 8.
    void breakBlock(World* worldIn, const BlockPos& pos, const IBlockState* state) override;
    // Java: canDispenserPlace and checkWitherSpawn match the wither BlockPattern and spawn the wither -- the pattern
    // matcher and the wither come with stage 10.
    bool canDispenserPlace(World* worldIn, const BlockPos& pos, const ItemStack& stack);
    void checkWitherSpawn(World* worldIn, const BlockPos& pos, TileEntitySkull* te);
    const IBlockState* getStateFromMeta(int meta) const override
    {
        return getDefaultState()->withProperty(FACING, getFront(meta & 7))->withProperty(NODROP, (meta & 8) > 0);
    }
    int getMetaFromState(const IBlockState* state) const override
    {
        int i = 0;
        i = i | getIndex(state->getValue(FACING));
        if (state->getValue(NODROP))
            i |= 8;
        return i;
    }
    Item* getItemDropped(const IBlockState*, Random&, int) const override { return Items::skull; }

protected:
    void construct() override
    {
        BlockContainer::construct();
        setDefaultState(blockState->getBaseState()->withProperty(FACING, EnumFacing::NORTH)->withProperty(NODROP, false));
        setBlockBounds(0.25f, 0.0f, 0.25f, 0.75f, 0.5f, 0.75f);
    }
    std::unique_ptr<BlockState> createBlockState() override { return makeBlockState({ &FACING, &NODROP }); }
};

inline const PropertyDirection BlockSkull::FACING("facing");
inline const PropertyBool BlockSkull::NODROP("nodrop");

} // namespace mc

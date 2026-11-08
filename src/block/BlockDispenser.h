#pragma once

// Original: net/minecraft/block/BlockDispenser.java and BlockDropper.java

#include "block/BlockContainer.h"

namespace mc {

class BlockDispenser : public BlockContainer {
public:
    MC_BLOCK_CLASS(BlockDispenser)

    static const PropertyDirection FACING;
    static const PropertyBool TRIGGERED;

    BlockDispenser()
        : BlockContainer(Material::rock)
    {
    }

    static EnumFacing getFacing(int meta) { return getFront(meta & 7); }
    int tickRate(World*) override { return 4; }
    void onBlockAdded(World* worldIn, const BlockPos& pos, const IBlockState* state) override;
    // Java: onBlockActivated opens the dispenser GUI -- stage 8.
    void onNeighborBlockChange(World* worldIn, const BlockPos& pos, const IBlockState* state, Block* neighborBlock) override;
    void updateTick(World* worldIn, const BlockPos& pos, const IBlockState* state, Random& rand) override;
    std::shared_ptr<TileEntity> createNewTileEntity(World* worldIn, int meta) override;
    const IBlockState* onBlockPlaced(World* worldIn, const BlockPos& pos, EnumFacing facing, float hitX, float hitY, float hitZ, int meta,
        EntityLivingBase* placer) override;
    void onBlockPlacedBy(World* worldIn, const BlockPos& pos, const IBlockState* state, EntityLivingBase* placer, const ItemStack& stack) override;
    void breakBlock(World* worldIn, const BlockPos& pos, const IBlockState* state) override;
    int getComparatorInputOverride(World* worldIn, const BlockPos& pos) override;


    bool hasComparatorInputOverride() const override { return true; }
    int getRenderType() const override { return 3; }
    const IBlockState* getStateForEntityRender(const IBlockState*) const override { return getDefaultState()->withProperty(FACING, EnumFacing::SOUTH); }
    const IBlockState* getStateFromMeta(int meta) const override
    {
        return getDefaultState()->withProperty(FACING, getFacing(meta))->withProperty(TRIGGERED, (meta & 8) > 0);
    }
    int getMetaFromState(const IBlockState* state) const override
    {
        int i = 0;
        i = i | getIndex(state->getValue(FACING));
        if (state->getValue(TRIGGERED))
            i |= 8;
        return i;
    }

protected:
    void construct() override
    {
        BlockContainer::construct();
        setDefaultState(blockState->getBaseState()->withProperty(FACING, EnumFacing::NORTH)->withProperty(TRIGGERED, false));
        setCreativeTab(&CreativeTabs::tabRedstone);
    }
    std::unique_ptr<BlockState> createBlockState() override { return makeBlockState({ &FACING, &TRIGGERED }); }

    virtual void dispense(World* worldIn, const BlockPos& pos);

private:
    void setDefaultDirection(World* worldIn, const BlockPos& pos, const IBlockState* state);
};

class BlockDropper : public BlockDispenser {
public:
    MC_BLOCK_CLASS(BlockDropper)

    std::shared_ptr<TileEntity> createNewTileEntity(World* worldIn, int meta) override;

protected:
    void dispense(World* worldIn, const BlockPos& pos) override;
};

inline const PropertyDirection BlockDispenser::FACING("facing");
inline const PropertyBool BlockDispenser::TRIGGERED("triggered");

} // namespace mc

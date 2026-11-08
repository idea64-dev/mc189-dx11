#pragma once

// Original: net/minecraft/block/BlockCommandBlock.java

#include "block/BlockContainer.h"

namespace mc {

class BlockCommandBlock : public BlockContainer {
public:
    MC_BLOCK_CLASS(BlockCommandBlock)

    static const PropertyBool TRIGGERED;

    BlockCommandBlock()
        : BlockContainer(Material::iron, MapColor::adobeColor)
    {
    }

    // Java: a TileEntityCommandBlock; CommandBlockLogic comes with the commands (stage 12) -- an unported "Control"
    // tile entity keeps its NBT meanwhile.
    std::shared_ptr<TileEntity> createNewTileEntity(World* worldIn, int meta) override;
    void onNeighborBlockChange(World* worldIn, const BlockPos& pos, const IBlockState* state, Block* neighborBlock) override;
    // Java: updateTick runs the command (getCommandBlockLogic().trigger) and updates comparators -- stage 12.
    void updateTick(World* worldIn, const BlockPos& pos, const IBlockState* state, Random& rand) override;
    int tickRate(World*) override { return 1; }
    // Java: onBlockActivated opens the command editor -- stage 8/12.
    // Java: onBlockPlacedBy names the command block logic and sets its trackOutput -- stage 12.
    const IBlockState* onBlockPlaced(World*, const BlockPos&, EnumFacing, float, float, float, int, EntityLivingBase*) override
    {
        return getDefaultState()->withProperty(TRIGGERED, false);
    }
    bool hasComparatorInputOverride() const override { return true; }
    int quantityDropped(Random&) const override { return 0; }
    int getRenderType() const override { return 3; }
    const IBlockState* getStateFromMeta(int meta) const override { return getDefaultState()->withProperty(TRIGGERED, (meta & 1) > 0); }
    int getMetaFromState(const IBlockState* state) const override
    {
        int i = 0;
        if (state->getValue(TRIGGERED))
            i |= 1;
        return i;
    }

protected:
    void construct() override
    {
        BlockContainer::construct();
        setDefaultState(blockState->getBaseState()->withProperty(TRIGGERED, false));
    }
    std::unique_ptr<BlockState> createBlockState() override { return makeBlockState({ &TRIGGERED }); }
};

inline const PropertyBool BlockCommandBlock::TRIGGERED("triggered");

} // namespace mc

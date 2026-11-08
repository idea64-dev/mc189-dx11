#pragma once

// Original: net/minecraft/block/BlockDispenser.java and BlockDropper.java (the dispense behaviours come with
// the dispenser tile entity, stage 9)

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
};

class BlockDropper : public BlockDispenser {
public:
    MC_BLOCK_CLASS(BlockDropper)
};

inline const PropertyDirection BlockDispenser::FACING("facing");
inline const PropertyBool BlockDispenser::TRIGGERED("triggered");

} // namespace mc

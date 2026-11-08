#pragma once

// Original: net/minecraft/block/BlockSkull.java (the wither pattern comes with entities, stage 10)

#include "block/BlockContainer.h"

namespace mc {

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

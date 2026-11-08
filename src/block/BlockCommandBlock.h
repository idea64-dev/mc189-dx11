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

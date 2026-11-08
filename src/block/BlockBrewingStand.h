#pragma once

// Original: net/minecraft/block/BlockBrewingStand.java

#include "block/BlockContainer.h"

#include <array>

namespace mc {

class BlockBrewingStand : public BlockContainer {
public:
    MC_BLOCK_CLASS(BlockBrewingStand)

    static const std::array<PropertyBool, 3> HAS_BOTTLE;

    BlockBrewingStand()
        : BlockContainer(Material::iron)
    {
    }

    bool isOpaqueCube() const override { return false; }
    int getRenderType() const override { return 3; }
    bool isFullCube() const override { return false; }
    void setBlockBoundsForItemRender() override { setBlockBounds(0.0f, 0.0f, 0.0f, 1.0f, 0.125f, 1.0f); }
    bool hasComparatorInputOverride() const override { return true; }
    EnumWorldBlockLayer getBlockLayer() const override { return EnumWorldBlockLayer::CUTOUT; }
    const IBlockState* getStateFromMeta(int meta) const override
    {
        const IBlockState* iblockstate = getDefaultState();
        for (int i = 0; i < 3; ++i)
            iblockstate = iblockstate->withProperty(HAS_BOTTLE[static_cast<size_t>(i)], (meta & 1 << i) > 0);
        return iblockstate;
    }
    int getMetaFromState(const IBlockState* state) const override
    {
        int i = 0;
        for (int j = 0; j < 3; ++j) {
            if (state->getValue(HAS_BOTTLE[static_cast<size_t>(j)]))
                i |= 1 << j;
        }
        return i;
    }
    Item* getItemDropped(const IBlockState*, Random&, int) const override { return Items::brewing_stand; }

protected:
    void construct() override
    {
        BlockContainer::construct();
        setDefaultState(blockState->getBaseState()->withProperty(HAS_BOTTLE[0], false)->withProperty(HAS_BOTTLE[1], false)->withProperty(HAS_BOTTLE[2], false));
    }
    std::unique_ptr<BlockState> createBlockState() override { return makeBlockState({ &HAS_BOTTLE[0], &HAS_BOTTLE[1], &HAS_BOTTLE[2] }); }
};

inline const std::array<PropertyBool, 3> BlockBrewingStand::HAS_BOTTLE { PropertyBool("has_bottle_0"), PropertyBool("has_bottle_1"), PropertyBool("has_bottle_2") };

} // namespace mc

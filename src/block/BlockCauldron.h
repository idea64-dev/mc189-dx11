#pragma once

// Original: net/minecraft/block/BlockCauldron.java

#include "block/BlockBasics.h"

namespace mc {

class BlockCauldron : public Block {
public:
    MC_BLOCK_CLASS(BlockCauldron)

    static const PropertyInteger LEVEL;

    BlockCauldron()
        : Block(Material::iron, MapColor::stoneColor)
    {
    }

    void setBlockBoundsForItemRender() override { setBlockBounds(0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f); }
    bool isOpaqueCube() const override { return false; }
    bool isFullCube() const override { return false; }
    bool hasComparatorInputOverride() const override { return true; }
    const IBlockState* getStateFromMeta(int meta) const override { return getDefaultState()->withProperty(LEVEL, meta); }
    int getMetaFromState(const IBlockState* state) const override { return state->getValue(LEVEL); }
    Item* getItemDropped(const IBlockState*, Random&, int) const override { return Items::cauldron; }

protected:
    void construct() override
    {
        Block::construct();
        setDefaultState(blockState->getBaseState()->withProperty(LEVEL, 0));
    }
    std::unique_ptr<BlockState> createBlockState() override { return makeBlockState({ &LEVEL }); }
};

inline const PropertyInteger BlockCauldron::LEVEL("level", 0, 3);

} // namespace mc

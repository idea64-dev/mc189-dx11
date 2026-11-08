#pragma once

// Original: net/minecraft/block/BlockLiquid.java, BlockDynamicLiquid.java and BlockStaticLiquid.java
// Flow comes with world ticks (stage 9).

#include "block/BlockBasics.h"

namespace mc {

class BlockLiquid : public Block {
public:
    static const PropertyInteger LEVEL;

    explicit BlockLiquid(const Material& materialIn)
        : Block(materialIn)
    {
    }

    bool isFullCube() const override { return false; }
    bool isOpaqueCube() const override { return false; }
    bool canCollideCheck(const IBlockState* state, bool hitIfLiquid) const override { return hitIfLiquid && state->getValue(LEVEL) == 0; }
    int getRenderType() const override { return 1; }
    int quantityDropped(Random&) const override { return 0; }
    EnumWorldBlockLayer getBlockLayer() const override { return &blockMaterial == &Material::water ? EnumWorldBlockLayer::TRANSLUCENT : EnumWorldBlockLayer::SOLID; }
    const IBlockState* getStateFromMeta(int meta) const override { return getDefaultState()->withProperty(LEVEL, meta); }
    int getMetaFromState(const IBlockState* state) const override { return state->getValue(LEVEL); }

protected:
    void construct() override
    {
        Block::construct();
        setDefaultState(blockState->getBaseState()->withProperty(LEVEL, 0));
        setBlockBounds(0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f);
        setTickRandomly(true);
    }
    std::unique_ptr<BlockState> createBlockState() override { return makeBlockState({ &LEVEL }); }
};

class BlockDynamicLiquid : public BlockLiquid {
public:
    MC_BLOCK_CLASS(BlockDynamicLiquid)

    explicit BlockDynamicLiquid(const Material& materialIn)
        : BlockLiquid(materialIn)
    {
    }

private:
    int adjacentSourceBlocks = 0;
};

class BlockStaticLiquid : public BlockLiquid {
public:
    MC_BLOCK_CLASS(BlockStaticLiquid)

    explicit BlockStaticLiquid(const Material& materialIn)
        : BlockLiquid(materialIn)
    {
    }

protected:
    void construct() override
    {
        BlockLiquid::construct();
        setTickRandomly(false);
        if (&blockMaterial == &Material::lava)
            setTickRandomly(true);
    }
};

inline const PropertyInteger BlockLiquid::LEVEL("level", 0, 15);

} // namespace mc

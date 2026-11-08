#pragma once

// Original: net/minecraft/block/BlockFarmland.java

#include "block/BlockBasics.h"

namespace mc {

class BlockFarmland : public Block {
public:
    MC_BLOCK_CLASS(BlockFarmland)

    static const PropertyInteger MOISTURE;

    BlockFarmland()
        : Block(Material::ground)
    {
    }

    bool isOpaqueCube() const override { return false; }
    bool isFullCube() const override { return false; }
    const IBlockState* getStateFromMeta(int meta) const override { return getDefaultState()->withProperty(MOISTURE, meta & 7); }
    int getMetaFromState(const IBlockState* state) const override { return state->getValue(MOISTURE); }
    Item* getItemDropped(const IBlockState* state, Random& rand, int fortune) const override;

protected:
    void construct() override
    {
        Block::construct();
        setDefaultState(blockState->getBaseState()->withProperty(MOISTURE, 0));
        setTickRandomly(true);
        setBlockBounds(0.0f, 0.0f, 0.0f, 1.0f, 0.9375f, 1.0f);
        setLightOpacity(255);
    }
    std::unique_ptr<BlockState> createBlockState() override { return makeBlockState({ &MOISTURE }); }
};

inline const PropertyInteger BlockFarmland::MOISTURE("moisture", 0, 7);

} // namespace mc

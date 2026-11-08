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

    void addCollisionBoxesToList(World* worldIn, const BlockPos& pos, const IBlockState* state, const AxisAlignedBB& mask,
        std::vector<AxisAlignedBB>& list, Entity* collidingEntity) override;
    void setBlockBoundsForItemRender() override { setBlockBounds(0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f); }
    using Block::onEntityCollidedWithBlock;
    void onEntityCollidedWithBlock(World* worldIn, const BlockPos& pos, const IBlockState* state, Entity* entityIn) override;
    // Java: onBlockActivated fills, empties and washes with the held item -- the player comes with stage 8.
    void setWaterLevel(World* worldIn, const BlockPos& pos, const IBlockState* state, int level);
    void fillWithRain(World* worldIn, const BlockPos& pos) override;
    Item* getItem(World*, const BlockPos&) override { return Items::cauldron; }
    int getComparatorInputOverride(World* worldIn, const BlockPos& pos) override;

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

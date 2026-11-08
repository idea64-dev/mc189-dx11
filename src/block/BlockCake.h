#pragma once

// Original: net/minecraft/block/BlockCake.java

#include "block/BlockBasics.h"

namespace mc {

class BlockCake : public Block {
public:
    MC_BLOCK_CLASS(BlockCake)

    static const PropertyInteger BITES;

    BlockCake()
        : Block(Material::cake)
    {
    }

    void setBlockBoundsForItemRender() override
    {
        const float f = 0.0625f;
        const float f1 = 0.5f;
        setBlockBounds(f, 0.0f, f, 1.0f - f, f1, 1.0f - f);
    }
    void setBlockBoundsBasedOnState(IBlockAccess* worldIn, const BlockPos& pos) override;
    std::optional<AxisAlignedBB> getCollisionBoundingBox(World* worldIn, const BlockPos& pos, const IBlockState* state) override;
    AxisAlignedBB getSelectedBoundingBox(World* worldIn, const BlockPos& pos) override;
    bool isFullCube() const override { return false; }
    bool isOpaqueCube() const override { return false; }
    // Java: onBlockActivated and onBlockClicked eat a slice when the player can eat -- the player comes with stage 8.
    bool canPlaceBlockAt(World* worldIn, const BlockPos& pos) override;
    void onNeighborBlockChange(World* worldIn, const BlockPos& pos, const IBlockState* state, Block* neighborBlock) override;
    int quantityDropped(Random&) const override { return 0; }
    Item* getItem(World*, const BlockPos&) override { return Items::cake; }
    int getComparatorInputOverride(World* worldIn, const BlockPos& pos) override;
    EnumWorldBlockLayer getBlockLayer() const override { return EnumWorldBlockLayer::CUTOUT; }
    const IBlockState* getStateFromMeta(int meta) const override { return getDefaultState()->withProperty(BITES, meta); }
    int getMetaFromState(const IBlockState* state) const override { return state->getValue(BITES); }
    bool hasComparatorInputOverride() const override { return true; }
    Item* getItemDropped(const IBlockState*, Random&, int) const override { return nullptr; }

protected:
    void construct() override
    {
        Block::construct();
        setDefaultState(blockState->getBaseState()->withProperty(BITES, 0));
        setTickRandomly(true);
    }
    std::unique_ptr<BlockState> createBlockState() override { return makeBlockState({ &BITES }); }

private:
    bool canBlockStay(World* worldIn, const BlockPos& pos);
};

inline const PropertyInteger BlockCake::BITES("bites", 0, 6);

} // namespace mc

#pragma once

// Original: net/minecraft/block/BlockBush.java

#include "block/BlockBasics.h"

namespace mc {

class BlockBush : public Block {
public:
    BlockBush()
        : BlockBush(Material::plants)
    {
    }
    explicit BlockBush(const Material& materialIn)
        : BlockBush(materialIn, materialIn.getMaterialMapColor())
    {
    }
    BlockBush(const Material& materialIn, const MapColor& mapColorIn)
        : Block(materialIn, mapColorIn)
    {
    }

    bool isOpaqueCube() const override { return false; }
    bool isFullCube() const override { return false; }
    EnumWorldBlockLayer getBlockLayer() const override { return EnumWorldBlockLayer::CUTOUT; }
    bool canPlaceBlockAt(World* worldIn, const BlockPos& pos) override;
    void onNeighborBlockChange(World* worldIn, const BlockPos& pos, const IBlockState* state, Block* neighborBlock) override;
    void updateTick(World* worldIn, const BlockPos& pos, const IBlockState* state, Random& rand) override;
    virtual bool canBlockStay(World* worldIn, const BlockPos& pos, const IBlockState* state);
    std::optional<AxisAlignedBB> getCollisionBoundingBox(World*, const BlockPos&, const IBlockState*) override { return std::nullopt; }

protected:
    virtual bool canPlaceBlockOn(const Block* ground) const;
    virtual void checkAndDropBlock(World* worldIn, const BlockPos& pos, const IBlockState* state);

    void construct() override
    {
        Block::construct();
        setTickRandomly(true);
        const float f = 0.2f;
        setBlockBounds(0.5f - f, 0.0f, 0.5f - f, 0.5f + f, f * 3.0f, 0.5f + f);
        setCreativeTab(&CreativeTabs::tabDecorations);
    }
};

} // namespace mc

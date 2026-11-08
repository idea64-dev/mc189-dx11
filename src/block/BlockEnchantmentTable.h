#pragma once

// Original: net/minecraft/block/BlockEnchantmentTable.java

#include "block/BlockContainer.h"

namespace mc {

class BlockEnchantmentTable : public BlockContainer {
public:
    MC_BLOCK_CLASS(BlockEnchantmentTable)

    BlockEnchantmentTable()
        : BlockContainer(Material::rock, MapColor::redColor)
    {
    }

    bool isFullCube() const override { return false; }
    void randomDisplayTick(World* worldIn, const BlockPos& pos, const IBlockState* state, Random& rand) override;
    std::shared_ptr<TileEntity> createNewTileEntity(World* worldIn, int meta) override;
    // Java: onBlockActivated opens the enchanting GUI -- stage 8.
    void onBlockPlacedBy(World* worldIn, const BlockPos& pos, const IBlockState* state, EntityLivingBase* placer, const ItemStack& stack) override;

    bool isOpaqueCube() const override { return false; }
    int getRenderType() const override { return 3; }

protected:
    void construct() override
    {
        BlockContainer::construct();
        setBlockBounds(0.0f, 0.0f, 0.0f, 1.0f, 0.75f, 1.0f);
        setLightOpacity(0);
        setCreativeTab(&CreativeTabs::tabDecorations);
    }
};

} // namespace mc

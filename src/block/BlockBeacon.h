#pragma once

// Original: net/minecraft/block/BlockBeacon.java

#include "block/BlockContainer.h"

namespace mc {

class BlockBeacon : public BlockContainer {
public:
    MC_BLOCK_CLASS(BlockBeacon)

    BlockBeacon()
        : BlockContainer(Material::glass, MapColor::diamondColor)
    {
    }

    std::shared_ptr<TileEntity> createNewTileEntity(World* worldIn, int meta) override;
    // Java: onBlockActivated opens the beacon GUI -- stage 8.
    bool isOpaqueCube() const override { return false; }
    void onBlockPlacedBy(World* worldIn, const BlockPos& pos, const IBlockState* state, EntityLivingBase* placer, const ItemStack& stack) override;
    void onNeighborBlockChange(World* worldIn, const BlockPos& pos, const IBlockState* state, Block* neighborBlock) override;
    // Java: updateColorAsync -- a stained glass change re-checks the beacons below it on the HTTP thread pool and
    // back on the server thread; done at once here.
    static void updateColorAsync(World* worldIn, const BlockPos& glassPos);

    bool isFullCube() const override { return false; }
    int getRenderType() const override { return 3; }
    EnumWorldBlockLayer getBlockLayer() const override { return EnumWorldBlockLayer::CUTOUT; }

protected:
    void construct() override
    {
        BlockContainer::construct();
        setHardness(3.0f);
        setCreativeTab(&CreativeTabs::tabMisc);
    }
};

} // namespace mc

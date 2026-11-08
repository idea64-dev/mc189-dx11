#pragma once

// Original: net/minecraft/block/BlockEndPortal.java and BlockEndPortalFrame.java

#include "block/BlockContainer.h"

namespace mc {

class BlockEndPortal : public BlockContainer {
public:
    MC_BLOCK_CLASS(BlockEndPortal)

    explicit BlockEndPortal(const Material& materialIn)
        : BlockContainer(materialIn)
    {
    }

    bool isOpaqueCube() const override { return false; }
    bool isFullCube() const override { return false; }
    int quantityDropped(Random&) const override { return 0; }
    const MapColor& getMapColor(const IBlockState*) const override { return MapColor::blackColor; }
    std::shared_ptr<TileEntity> createNewTileEntity(World* worldIn, int meta) override;
    void setBlockBoundsBasedOnState(IBlockAccess*, const BlockPos&) override { setBlockBounds(0.0f, 0.0f, 0.0f, 1.0f, 0.0625f, 1.0f); }
    bool shouldSideBeRendered(IBlockAccess* worldIn, const BlockPos& pos, EnumFacing side) override
    {
        return side == EnumFacing::DOWN ? BlockContainer::shouldSideBeRendered(worldIn, pos, side) : false;
    }
    void addCollisionBoxesToList(World*, const BlockPos&, const IBlockState*, const AxisAlignedBB&, std::vector<AxisAlignedBB>&, Entity*) override { }
    using Block::onEntityCollidedWithBlock;
    void onEntityCollidedWithBlock(World* worldIn, const BlockPos& pos, const IBlockState* state, Entity* entityIn) override;
    void randomDisplayTick(World* worldIn, const BlockPos& pos, const IBlockState* state, Random& rand) override;
    Item* getItem(World*, const BlockPos&) override { return nullptr; }

protected:
    void construct() override
    {
        BlockContainer::construct();
        setLightLevel(1.0f);
    }
};

class BlockEndPortalFrame : public Block {
public:
    MC_BLOCK_CLASS(BlockEndPortalFrame)

    static const PropertyDirection FACING;
    static const PropertyBool EYE;

    BlockEndPortalFrame()
        : Block(Material::rock, MapColor::greenColor)
    {
    }

    bool isOpaqueCube() const override { return false; }
    void setBlockBoundsForItemRender() override { setBlockBounds(0.0f, 0.0f, 0.0f, 1.0f, 0.8125f, 1.0f); }
    bool hasComparatorInputOverride() const override { return true; }
    const IBlockState* getStateFromMeta(int meta) const override
    {
        return getDefaultState()->withProperty(EYE, (meta & 4) != 0)->withProperty(FACING, getHorizontal(meta & 3));
    }
    int getMetaFromState(const IBlockState* state) const override
    {
        int i = 0;
        i = i | getHorizontalIndex(state->getValue(FACING));
        if (state->getValue(EYE))
            i |= 4;
        return i;
    }
    Item* getItemDropped(const IBlockState*, Random&, int) const override { return nullptr; }
    void addCollisionBoxesToList(World* worldIn, const BlockPos& pos, const IBlockState* state, const AxisAlignedBB& mask,
        std::vector<AxisAlignedBB>& list, Entity* collidingEntity) override;
    const IBlockState* onBlockPlaced(World* worldIn, const BlockPos& pos, EnumFacing facing, float hitX, float hitY, float hitZ, int meta,
        EntityLivingBase* placer) override;
    int getComparatorInputOverride(World* worldIn, const BlockPos& pos) override;

protected:
    void construct() override
    {
        Block::construct();
        setDefaultState(blockState->getBaseState()->withProperty(FACING, EnumFacing::NORTH)->withProperty(EYE, false));
    }
    std::unique_ptr<BlockState> createBlockState() override { return makeBlockState({ &FACING, &EYE }); }
};

inline const PropertyDirection BlockEndPortalFrame::FACING("facing", isHorizontalFacing);
inline const PropertyBool BlockEndPortalFrame::EYE("eye");

} // namespace mc

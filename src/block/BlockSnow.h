#pragma once

// Original: net/minecraft/block/BlockSnow.java, BlockSnowBlock.java, BlockSoulSand.java and BlockSponge.java

#include "block/BlockBasics.h"

namespace mc {

class BlockSnow : public Block {
public:
    bool canPlaceBlockAt(World* worldIn, const BlockPos& pos) override;
    MC_BLOCK_CLASS(BlockSnow)

    static const PropertyInteger LAYERS;

    BlockSnow()
        : Block(Material::snow)
    {
    }

    bool isOpaqueCube() const override { return false; }
    bool isFullCube() const override { return false; }
    void setBlockBoundsForItemRender() override { getBoundsForLayers(0); }
    int quantityDropped(Random&) const override { return 0; }
    const IBlockState* getStateFromMeta(int meta) const override { return getDefaultState()->withProperty(LAYERS, (meta & 7) + 1); }
    int getMetaFromState(const IBlockState* state) const override { return state->getValue(LAYERS) - 1; }
    Item* getItemDropped(const IBlockState*, Random&, int) const override { return Items::snowball; }
    bool isPassable(IBlockAccess* worldIn, const BlockPos& pos) override;
    std::optional<AxisAlignedBB> getCollisionBoundingBox(World* worldIn, const BlockPos& pos, const IBlockState* state) override;
    void setBlockBoundsBasedOnState(IBlockAccess* worldIn, const BlockPos& pos) override;
    void onNeighborBlockChange(World* worldIn, const BlockPos& pos, const IBlockState* state, Block* neighborBlock) override;
    void harvestBlock(World* worldIn, EntityPlayer* player, const BlockPos& pos, const IBlockState* state, TileEntity* te) override;
    void updateTick(World* worldIn, const BlockPos& pos, const IBlockState* state, Random& rand) override;
    bool shouldSideBeRendered(IBlockAccess* worldIn, const BlockPos& pos, EnumFacing side) override
    {
        return side == EnumFacing::UP ? true : Block::shouldSideBeRendered(worldIn, pos, side);
    }
    bool isReplaceable(World* worldIn, const BlockPos& pos) override;

private:
    bool checkAndDropBlock(World* worldIn, const BlockPos& pos, const IBlockState* state);

protected:
    void construct() override
    {
        Block::construct();
        setDefaultState(blockState->getBaseState()->withProperty(LAYERS, 1));
        setBlockBounds(0.0f, 0.0f, 0.0f, 1.0f, 0.125f, 1.0f);
        setTickRandomly(true);
        setCreativeTab(&CreativeTabs::tabDecorations);
        setBlockBoundsForItemRender();
    }
    std::unique_ptr<BlockState> createBlockState() override { return makeBlockState({ &LAYERS }); }

    void getBoundsForLayers(int layers) { setBlockBounds(0.0f, 0.0f, 0.0f, 1.0f, static_cast<float>(layers) / 8.0f, 1.0f); }
};

class BlockSnowBlock : public Block {
public:
    MC_BLOCK_CLASS(BlockSnowBlock)

    BlockSnowBlock()
        : Block(Material::craftedSnow)
    {
    }

    int quantityDropped(Random&) const override { return 4; }
    Item* getItemDropped(const IBlockState*, Random&, int) const override { return Items::snowball; }
    void updateTick(World* worldIn, const BlockPos& pos, const IBlockState* state, Random& rand) override;

protected:
    void construct() override
    {
        Block::construct();
        setTickRandomly(true);
        setCreativeTab(&CreativeTabs::tabBlock);
    }
};

class BlockSoulSand : public Block {
public:
    MC_BLOCK_CLASS(BlockSoulSand)

    BlockSoulSand()
        : Block(Material::sand, MapColor::brownColor)
    {
    }

    std::optional<AxisAlignedBB> getCollisionBoundingBox(World* worldIn, const BlockPos& pos, const IBlockState* state) override;
    using Block::onEntityCollidedWithBlock;
    void onEntityCollidedWithBlock(World* worldIn, const BlockPos& pos, const IBlockState* state, Entity* entityIn) override;

protected:
    void construct() override
    {
        Block::construct();
        setCreativeTab(&CreativeTabs::tabBlock);
    }
};

class BlockSponge : public Block {
public:
    MC_BLOCK_CLASS(BlockSponge)

    static const PropertyBool WET;

    BlockSponge()
        : Block(Material::sponge)
    {
    }

    int damageDropped(const IBlockState* state) const override { return state->getValue(WET) ? 1 : 0; }
    const IBlockState* getStateFromMeta(int meta) const override { return getDefaultState()->withProperty(WET, (meta & 1) == 1); }
    int getMetaFromState(const IBlockState* state) const override { return state->getValue(WET) ? 1 : 0; }
    void getSubBlocks(Item* item, const CreativeTabs*, std::vector<ItemStack>& list) const override { addSubBlocks(item, list, { 0, 1 }); }
    void onBlockAdded(World* worldIn, const BlockPos& pos, const IBlockState* state) override { tryAbsorb(worldIn, pos, state); }
    void onNeighborBlockChange(World* worldIn, const BlockPos& pos, const IBlockState* state, Block* neighborBlock) override
    {
        tryAbsorb(worldIn, pos, state);
        Block::onNeighborBlockChange(worldIn, pos, state, neighborBlock);
    }
    void randomDisplayTick(World* worldIn, const BlockPos& pos, const IBlockState* state, Random& rand) override;

protected:
    void tryAbsorb(World* worldIn, const BlockPos& pos, const IBlockState* state);

private:
    bool absorb(World* worldIn, const BlockPos& pos);

protected:
    void construct() override
    {
        Block::construct();
        setDefaultState(blockState->getBaseState()->withProperty(WET, false));
        setCreativeTab(&CreativeTabs::tabBlock);
    }
    std::unique_ptr<BlockState> createBlockState() override { return makeBlockState({ &WET }); }
};

inline const PropertyInteger BlockSnow::LAYERS("layers", 1, 8);
inline const PropertyBool BlockSponge::WET("wet");

} // namespace mc

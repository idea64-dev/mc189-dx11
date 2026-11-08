#pragma once

// BlockBush subclasses without much state: net/minecraft/block/BlockLilyPad.java, BlockMushroom.java and
// BlockNetherWart.java

#include "block/BlockBush.h"

namespace mc {

class BlockLilyPad : public BlockBush {
public:
    MC_BLOCK_CLASS(BlockLilyPad)

    int getBlockColor() const override { return 7455580; }
    int getRenderColor(const IBlockState*) const override { return 7455580; }
    int getMetaFromState(const IBlockState*) const override { return 0; }
    void addCollisionBoxesToList(World* worldIn, const BlockPos& pos, const IBlockState* state, const AxisAlignedBB& mask,
        std::vector<AxisAlignedBB>& list, Entity* collidingEntity) override;
    std::optional<AxisAlignedBB> getCollisionBoundingBox(World* worldIn, const BlockPos& pos, const IBlockState* state) override
    {
        return Block::getCollisionBoundingBox(worldIn, pos, state);
    }
    int colorMultiplier(IBlockAccess*, const BlockPos&, int) override { return 2129968; }
    bool canBlockStay(World* worldIn, const BlockPos& pos, const IBlockState* state) override;

protected:
    bool canPlaceBlockOn(const Block* ground) const override;
    void construct() override
    {
        BlockBush::construct();
        const float f = 0.5f;
        const float f1 = 0.015625f;
        setBlockBounds(0.5f - f, 0.0f, 0.5f - f, 0.5f + f, f1, 0.5f + f);
        setCreativeTab(&CreativeTabs::tabDecorations);
    }
};

class BlockMushroom : public BlockBush {
public:
    MC_BLOCK_CLASS(BlockMushroom)

    void updateTick(World* worldIn, const BlockPos& pos, const IBlockState* state, Random& rand) override;
    bool canPlaceBlockAt(World* worldIn, const BlockPos& pos) override;
    bool canBlockStay(World* worldIn, const BlockPos& pos, const IBlockState* state) override;
    bool generateBigMushroom(World* worldIn, const BlockPos& pos, const IBlockState* state, Random& rand);
    // Java: IGrowable
    bool canGrow(World*, const BlockPos&, const IBlockState*, bool) { return true; }
    bool canUseBonemeal(World*, Random& rand, const BlockPos&, const IBlockState*) { return static_cast<double>(rand.nextFloat()) < 0.4; }
    void grow(World* worldIn, Random& rand, const BlockPos& pos, const IBlockState* state) { generateBigMushroom(worldIn, pos, state, rand); }

protected:
    void construct() override
    {
        BlockBush::construct();
        const float f = 0.2f;
        setBlockBounds(0.5f - f, 0.0f, 0.5f - f, 0.5f + f, f * 2.0f, 0.5f + f);
        setTickRandomly(true);
    }
    bool canPlaceBlockOn(const Block* ground) const override { return ground->isFullBlock(); }
};

class BlockNetherWart : public BlockBush {
public:
    MC_BLOCK_CLASS(BlockNetherWart)

    static const PropertyInteger AGE;

    BlockNetherWart()
        : BlockBush(Material::plants, MapColor::redColor)
    {
    }

    int quantityDropped(Random&) const override { return 0; }
    const IBlockState* getStateFromMeta(int meta) const override { return getDefaultState()->withProperty(AGE, meta); }
    int getMetaFromState(const IBlockState* state) const override { return state->getValue(AGE); }
    Item* getItemDropped(const IBlockState*, Random&, int) const override { return nullptr; }
    bool canBlockStay(World* worldIn, const BlockPos& pos, const IBlockState* state) override;
    void updateTick(World* worldIn, const BlockPos& pos, const IBlockState* state, Random& rand) override;
    void dropBlockAsItemWithChance(World* worldIn, const BlockPos& pos, const IBlockState* state, float chance, int fortune) override;
    Item* getItem(World* worldIn, const BlockPos& pos) override;

protected:
    bool canPlaceBlockOn(const Block* ground) const override;
    void construct() override
    {
        BlockBush::construct();
        setDefaultState(blockState->getBaseState()->withProperty(AGE, 0));
        setTickRandomly(true);
        const float f = 0.5f;
        setBlockBounds(0.5f - f, 0.0f, 0.5f - f, 0.5f + f, 0.25f, 0.5f + f);
        setCreativeTab(nullptr);
    }
    std::unique_ptr<BlockState> createBlockState() override { return makeBlockState({ &AGE }); }
};

inline const PropertyInteger BlockNetherWart::AGE("age", 0, 3);

} // namespace mc

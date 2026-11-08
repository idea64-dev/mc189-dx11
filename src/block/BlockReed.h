#pragma once

// Original: net/minecraft/block/BlockReed.java and BlockSapling.java

#include "block/BlockBush.h"
#include "block/BlockPlanks.h"

namespace mc {

class BlockReed : public Block {
public:
    MC_BLOCK_CLASS(BlockReed)

    static const PropertyInteger AGE;

    BlockReed()
        : Block(Material::plants)
    {
    }

    bool isOpaqueCube() const override { return false; }
    bool isFullCube() const override { return false; }
    EnumWorldBlockLayer getBlockLayer() const override { return EnumWorldBlockLayer::CUTOUT; }
    const IBlockState* getStateFromMeta(int meta) const override { return getDefaultState()->withProperty(AGE, meta); }
    int getMetaFromState(const IBlockState* state) const override { return state->getValue(AGE); }
    Item* getItemDropped(const IBlockState*, Random&, int) const override { return Items::reeds; }

protected:
    void construct() override
    {
        Block::construct();
        setDefaultState(blockState->getBaseState()->withProperty(AGE, 0));
        const float f = 0.375f;
        setBlockBounds(0.5f - f, 0.0f, 0.5f - f, 0.5f + f, 1.0f, 0.5f + f);
        setTickRandomly(true);
    }
    std::unique_ptr<BlockState> createBlockState() override { return makeBlockState({ &AGE }); }
};

class BlockSapling : public BlockBush {
public:
    MC_BLOCK_CLASS(BlockSapling)

    static const PropertyEnum<BlockPlanks::EnumType> TYPE;
    static const PropertyInteger STAGE;

    int damageDropped(const IBlockState* state) const override { return BlockPlanks::getMetadata(state->getValue(TYPE)); }
    const IBlockState* getStateFromMeta(int meta) const override
    {
        return getDefaultState()->withProperty(TYPE, BlockPlanks::byMetadata(meta & 7))->withProperty(STAGE, (meta & 8) >> 3);
    }
    int getMetaFromState(const IBlockState* state) const override
    {
        int i = 0;
        i = i | BlockPlanks::getMetadata(state->getValue(TYPE));
        i = i | state->getValue(STAGE) << 3;
        return i;
    }
    void getSubBlocks(Item* item, const CreativeTabs*, std::vector<ItemStack>& list) const override { addSubBlocks(item, list, 0, 6); }

protected:
    void construct() override
    {
        BlockBush::construct();
        setDefaultState(blockState->getBaseState()->withProperty(TYPE, BlockPlanks::EnumType::OAK)->withProperty(STAGE, 0));
        const float f = 0.4f;
        setBlockBounds(0.5f - f, 0.0f, 0.5f - f, 0.5f + f, f * 2.0f, 0.5f + f);
        setCreativeTab(&CreativeTabs::tabDecorations);
    }
    std::unique_ptr<BlockState> createBlockState() override { return makeBlockState({ &TYPE, &STAGE }); }
};

inline const PropertyInteger BlockReed::AGE("age", 0, 15);
inline const PropertyEnum<BlockPlanks::EnumType> BlockSapling::TYPE("type");
inline const PropertyInteger BlockSapling::STAGE("stage", 0, 1);

} // namespace mc

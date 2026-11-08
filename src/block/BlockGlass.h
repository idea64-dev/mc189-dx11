#pragma once

// Original: net/minecraft/block/BlockGlass.java, BlockStainedGlass.java, BlockIce.java, BlockPortal.java and
// BlockSlime.java -- the BlockBreakable subclasses. (BlockPortal.Size, the frame check, comes with the world.)

#include "block/BlockBreakable.h"
#include "item/EnumDyeColor.h"

namespace mc {

class BlockGlass : public BlockBreakable {
public:
    MC_BLOCK_CLASS(BlockGlass)

    BlockGlass(const Material& materialIn, bool ignoreSimilarityIn)
        : BlockBreakable(materialIn, ignoreSimilarityIn)
    {
    }

    int quantityDropped(Random&) const override { return 0; }
    EnumWorldBlockLayer getBlockLayer() const override { return EnumWorldBlockLayer::CUTOUT; }
    bool isFullCube() const override { return false; }
    bool canSilkHarvest() const override { return true; }

protected:
    void construct() override
    {
        BlockBreakable::construct();
        setCreativeTab(&CreativeTabs::tabBlock);
    }
};

class BlockStainedGlass : public BlockBreakable {
public:
    MC_BLOCK_CLASS(BlockStainedGlass)

    static const PropertyEnum<EnumDyeColor> COLOR;

    explicit BlockStainedGlass(const Material& materialIn)
        : BlockBreakable(materialIn, false)
    {
    }

    int damageDropped(const IBlockState* state) const override { return getMetadata(state->getValue(COLOR)); }
    const MapColor& getMapColor(const IBlockState* state) const override { return *info(state->getValue(COLOR)).mapColor; }
    EnumWorldBlockLayer getBlockLayer() const override { return EnumWorldBlockLayer::TRANSLUCENT; }
    int quantityDropped(Random&) const override { return 0; }
    bool canSilkHarvest() const override { return true; }
    bool isFullCube() const override { return false; }
    const IBlockState* getStateFromMeta(int meta) const override { return getDefaultState()->withProperty(COLOR, dyeColorByMetadata(meta)); }
    int getMetaFromState(const IBlockState* state) const override { return getMetadata(state->getValue(COLOR)); }
    void getSubBlocks(Item* item, const CreativeTabs*, std::vector<ItemStack>& list) const override { addSubBlocks(item, list, 0, 16); }

protected:
    void construct() override
    {
        BlockBreakable::construct();
        setDefaultState(blockState->getBaseState()->withProperty(COLOR, EnumDyeColor::WHITE));
        setCreativeTab(&CreativeTabs::tabBlock);
    }
    std::unique_ptr<BlockState> createBlockState() override { return makeBlockState({ &COLOR }); }
};

class BlockIce : public BlockBreakable {
public:
    MC_BLOCK_CLASS(BlockIce)

    BlockIce()
        : BlockBreakable(Material::ice, false)
    {
    }

    EnumWorldBlockLayer getBlockLayer() const override { return EnumWorldBlockLayer::TRANSLUCENT; }
    int quantityDropped(Random&) const override { return 0; }
    int getMobilityFlag() const override { return 0; }

protected:
    void construct() override
    {
        BlockBreakable::construct();
        slipperiness = 0.98f;
        setTickRandomly(true);
        setCreativeTab(&CreativeTabs::tabBlock);
    }
};

class BlockPortal : public BlockBreakable {
public:
    MC_BLOCK_CLASS(BlockPortal)

    static const PropertyEnum<Axis> AXIS;

    BlockPortal()
        : BlockBreakable(Material::portal, false)
    {
    }

    static int getMetaForAxis(Axis axis) { return axis == Axis::X ? 1 : (axis == Axis::Z ? 2 : 0); }

    bool isFullCube() const override { return false; }
    int quantityDropped(Random&) const override { return 0; }
    EnumWorldBlockLayer getBlockLayer() const override { return EnumWorldBlockLayer::TRANSLUCENT; }
    const IBlockState* getStateFromMeta(int meta) const override { return getDefaultState()->withProperty(AXIS, (meta & 3) == 2 ? Axis::Z : Axis::X); }
    int getMetaFromState(const IBlockState* state) const override { return getMetaForAxis(state->getValue(AXIS)); }

protected:
    void construct() override
    {
        BlockBreakable::construct();
        setDefaultState(blockState->getBaseState()->withProperty(AXIS, Axis::X));
        setTickRandomly(true);
    }
    std::unique_ptr<BlockState> createBlockState() override { return makeBlockState({ &AXIS }); }
};

class BlockSlime : public BlockBreakable {
public:
    MC_BLOCK_CLASS(BlockSlime)

    BlockSlime()
        : BlockBreakable(Material::clay, false, MapColor::grassColor)
    {
    }

    EnumWorldBlockLayer getBlockLayer() const override { return EnumWorldBlockLayer::TRANSLUCENT; }

protected:
    void construct() override
    {
        BlockBreakable::construct();
        setCreativeTab(&CreativeTabs::tabDecorations);
        slipperiness = 0.8f;
    }
};

inline const PropertyEnum<EnumDyeColor> BlockStainedGlass::COLOR("color");
inline const PropertyEnum<Axis> BlockPortal::AXIS("axis", { Axis::X, Axis::Z });

} // namespace mc

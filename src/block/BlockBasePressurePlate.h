#pragma once

// Original: net/minecraft/block/BlockBasePressurePlate.java, BlockPressurePlate.java and
// BlockPressurePlateWeighted.java

#include "block/BlockBasics.h"

namespace mc {

class BlockBasePressurePlate : public Block {
public:
    explicit BlockBasePressurePlate(const Material& materialIn)
        : BlockBasePressurePlate(materialIn, materialIn.getMaterialMapColor())
    {
    }
    BlockBasePressurePlate(const Material& materialIn, const MapColor& mapColorIn)
        : Block(materialIn, mapColorIn)
    {
    }

    bool isOpaqueCube() const override { return false; }
    bool isFullCube() const override { return false; }
    bool canSpawnInBlock() const override { return true; }
    bool canProvidePower() const override { return true; }
    void setBlockBoundsForItemRender() override { setBlockBounds(0.0f, 0.375f, 0.0f, 1.0f, 0.625f, 1.0f); }
    int getMobilityFlag() const override { return 1; }

protected:
    void construct() override
    {
        Block::construct();
        setCreativeTab(&CreativeTabs::tabRedstone);
        setTickRandomly(true);
    }
};

class BlockPressurePlate : public BlockBasePressurePlate {
public:
    MC_BLOCK_CLASS(BlockPressurePlate)

    enum class Sensitivity { EVERYTHING, MOBS };
    static const PropertyBool POWERED;

    BlockPressurePlate(const Material& materialIn, Sensitivity sensitivityIn)
        : BlockBasePressurePlate(materialIn)
        , sensitivity(sensitivityIn)
    {
    }

    const IBlockState* getStateFromMeta(int meta) const override { return getDefaultState()->withProperty(POWERED, meta == 1); }
    int getMetaFromState(const IBlockState* state) const override { return state->getValue(POWERED) ? 1 : 0; }

protected:
    void construct() override
    {
        BlockBasePressurePlate::construct();
        setDefaultState(blockState->getBaseState()->withProperty(POWERED, false));
    }
    std::unique_ptr<BlockState> createBlockState() override { return makeBlockState({ &POWERED }); }

private:
    [[maybe_unused]] Sensitivity sensitivity;
};

class BlockPressurePlateWeighted : public BlockBasePressurePlate {
public:
    MC_BLOCK_CLASS(BlockPressurePlateWeighted)

    static const PropertyInteger POWER;

    BlockPressurePlateWeighted(const Material& materialIn, int maxWeight)
        : BlockPressurePlateWeighted(materialIn, maxWeight, materialIn.getMaterialMapColor())
    {
    }
    BlockPressurePlateWeighted(const Material& materialIn, int maxWeight, const MapColor& mapColorIn)
        : BlockBasePressurePlate(materialIn, mapColorIn)
        , field_150068_a(maxWeight)
    {
    }

    const IBlockState* getStateFromMeta(int meta) const override { return getDefaultState()->withProperty(POWER, meta); }
    int getMetaFromState(const IBlockState* state) const override { return state->getValue(POWER); }

protected:
    void construct() override
    {
        BlockBasePressurePlate::construct();
        setDefaultState(blockState->getBaseState()->withProperty(POWER, 0));
    }
    std::unique_ptr<BlockState> createBlockState() override { return makeBlockState({ &POWER }); }

private:
    [[maybe_unused]] int field_150068_a;
};

inline const PropertyBool BlockPressurePlate::POWERED("powered");
inline const PropertyInteger BlockPressurePlateWeighted::POWER("power", 0, 15);

} // namespace mc

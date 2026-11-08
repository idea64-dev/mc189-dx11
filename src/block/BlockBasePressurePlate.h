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

    void setBlockBoundsBasedOnState(IBlockAccess* worldIn, const BlockPos& pos) override;
    int tickRate(World*) override { return 20; }
    std::optional<AxisAlignedBB> getCollisionBoundingBox(World*, const BlockPos&, const IBlockState*) override { return std::nullopt; }
    bool isOpaqueCube() const override { return false; }
    bool isFullCube() const override { return false; }
    bool isPassable(IBlockAccess*, const BlockPos&) override { return true; }
    bool canSpawnInBlock() const override { return true; }
    bool canPlaceBlockAt(World* worldIn, const BlockPos& pos) override { return canBePlacedOn(worldIn, pos.down()); }
    void onNeighborBlockChange(World* worldIn, const BlockPos& pos, const IBlockState* state, Block* neighborBlock) override;
    void randomTick(World*, const BlockPos&, const IBlockState*, Random&) override { }
    void updateTick(World* worldIn, const BlockPos& pos, const IBlockState* state, Random& rand) override;
    using Block::onEntityCollidedWithBlock;
    void onEntityCollidedWithBlock(World* worldIn, const BlockPos& pos, const IBlockState* state, Entity* entityIn) override;
    void breakBlock(World* worldIn, const BlockPos& pos, const IBlockState* state) override;
    int getWeakPower(IBlockAccess*, const BlockPos&, const IBlockState* state, EnumFacing) override { return getRedstoneStrength(state); }
    int getStrongPower(IBlockAccess*, const BlockPos&, const IBlockState* state, EnumFacing side) override
    {
        return side == EnumFacing::UP ? getRedstoneStrength(state) : 0;
    }
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

    void setBlockBoundsBasedOnState0(const IBlockState* state);
    void updateState(World* worldIn, const BlockPos& pos, const IBlockState* state, int oldRedstoneStrength);
    AxisAlignedBB getSensitiveAABB(const BlockPos& pos) const;
    void updateNeighbors(World* worldIn, const BlockPos& pos);
    virtual int computeRedstoneStrength(World* worldIn, const BlockPos& pos) = 0;
    virtual int getRedstoneStrength(const IBlockState* state) const = 0;
    virtual const IBlockState* setRedstoneStrength(const IBlockState* state, int strength) const = 0;

private:
    bool canBePlacedOn(World* worldIn, const BlockPos& pos);
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

    int getRedstoneStrength(const IBlockState* state) const override { return state->getValue(POWERED) ? 15 : 0; }
    const IBlockState* setRedstoneStrength(const IBlockState* state, int strength) const override { return state->withProperty(POWERED, strength > 0); }
    int computeRedstoneStrength(World* worldIn, const BlockPos& pos) override;

private:
    Sensitivity sensitivity;
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
    int tickRate(World*) override { return 10; }

protected:
    void construct() override
    {
        BlockBasePressurePlate::construct();
        setDefaultState(blockState->getBaseState()->withProperty(POWER, 0));
    }
    std::unique_ptr<BlockState> createBlockState() override { return makeBlockState({ &POWER }); }

    int computeRedstoneStrength(World* worldIn, const BlockPos& pos) override;
    int getRedstoneStrength(const IBlockState* state) const override { return state->getValue(POWER); }
    const IBlockState* setRedstoneStrength(const IBlockState* state, int strength) const override { return state->withProperty(POWER, strength); }

private:
    int field_150068_a;
};

inline const PropertyBool BlockPressurePlate::POWERED("powered");
inline const PropertyInteger BlockPressurePlateWeighted::POWER("power", 0, 15);

} // namespace mc

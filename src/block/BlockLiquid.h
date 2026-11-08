#pragma once

// Original: net/minecraft/block/BlockLiquid.java, BlockDynamicLiquid.java and BlockStaticLiquid.java

#include "block/BlockBasics.h"

namespace mc {

class BlockDynamicLiquid;
class BlockStaticLiquid;

class BlockLiquid : public Block {
public:
    static const PropertyInteger LEVEL;

    explicit BlockLiquid(const Material& materialIn)
        : Block(materialIn)
    {
    }

    bool isFullCube() const override { return false; }
    bool isOpaqueCube() const override { return false; }
    bool canCollideCheck(const IBlockState* state, bool hitIfLiquid) const override { return hitIfLiquid && state->getValue(LEVEL) == 0; }
    int getRenderType() const override { return 1; }
    int quantityDropped(Random&) const override { return 0; }
    EnumWorldBlockLayer getBlockLayer() const override { return &blockMaterial == &Material::water ? EnumWorldBlockLayer::TRANSLUCENT : EnumWorldBlockLayer::SOLID; }
    const IBlockState* getStateFromMeta(int meta) const override { return getDefaultState()->withProperty(LEVEL, meta); }
    int getMetaFromState(const IBlockState* state) const override { return state->getValue(LEVEL); }
    Item* getItemDropped(const IBlockState*, Random&, int) const override { return nullptr; }
    bool isPassable(IBlockAccess*, const BlockPos&) override { return &blockMaterial != &Material::lava; }
    int colorMultiplier(IBlockAccess* worldIn, const BlockPos& pos, int renderPass) override;
    static float getLiquidHeightPercent(int meta);
    bool isBlockSolid(IBlockAccess* worldIn, const BlockPos& pos, EnumFacing side) override;
    bool shouldSideBeRendered(IBlockAccess* worldIn, const BlockPos& pos, EnumFacing side) override;
    bool shouldRenderSides(IBlockAccess* blockAccess, const BlockPos& pos);
    std::optional<AxisAlignedBB> getCollisionBoundingBox(World*, const BlockPos&, const IBlockState*) override { return std::nullopt; }
    Vec3 modifyAcceleration(World* worldIn, const BlockPos& pos, Entity* entityIn, const Vec3& motion) override;
    int tickRate(World* worldIn) override;
    int getMixedBrightnessForBlock(IBlockAccess* worldIn, const BlockPos& pos) override;
    void randomDisplayTick(World* worldIn, const BlockPos& pos, const IBlockState* state, Random& rand) override;
    static double getFlowDirection(IBlockAccess* worldIn, const BlockPos& pos, const Material& materialIn);
    void onBlockAdded(World* worldIn, const BlockPos& pos, const IBlockState* state) override;
    void onNeighborBlockChange(World* worldIn, const BlockPos& pos, const IBlockState* state, Block* neighborBlock) override;
    bool checkForMixing(World* worldIn, const BlockPos& pos, const IBlockState* state);
    static BlockDynamicLiquid* getFlowingBlock(const Material& materialIn);
    static BlockStaticLiquid* getStaticBlock(const Material& materialIn);

protected:
    int getLevel(IBlockAccess* worldIn, const BlockPos& pos);
    int getEffectiveFlowDecay(IBlockAccess* worldIn, const BlockPos& pos);
    Vec3 getFlowVector(IBlockAccess* worldIn, const BlockPos& pos);
    void triggerMixEffects(World* worldIn, const BlockPos& pos);
    void construct() override
    {
        Block::construct();
        setDefaultState(blockState->getBaseState()->withProperty(LEVEL, 0));
        setBlockBounds(0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f);
        setTickRandomly(true);
    }
    std::unique_ptr<BlockState> createBlockState() override { return makeBlockState({ &LEVEL }); }
};

class BlockDynamicLiquid : public BlockLiquid {
public:
    MC_BLOCK_CLASS(BlockDynamicLiquid)

    explicit BlockDynamicLiquid(const Material& materialIn)
        : BlockLiquid(materialIn)
    {
    }

    void updateTick(World* worldIn, const BlockPos& pos, const IBlockState* state, Random& rand) override;
    void onBlockAdded(World* worldIn, const BlockPos& pos, const IBlockState* state) override;

protected:
    int checkAdjacentBlock(World* worldIn, const BlockPos& pos, int currentMinLevel);

private:
    void placeStaticBlock(World* worldIn, const BlockPos& pos, const IBlockState* currentState);
    void tryFlowInto(World* worldIn, const BlockPos& pos, const IBlockState* state, int level);
    int func_176374_a(World* worldIn, const BlockPos& pos, int distance, EnumFacing calculateFlowCost);
    // Java: an EnumSet, iterated in enum order.
    std::vector<EnumFacing> getPossibleFlowDirections(World* worldIn, const BlockPos& pos);
    bool isBlocked(World* worldIn, const BlockPos& pos, const IBlockState* state);
    bool canFlowInto(World* worldIn, const BlockPos& pos, const IBlockState* state);

    int adjacentSourceBlocks = 0;
};

class BlockStaticLiquid : public BlockLiquid {
public:
    MC_BLOCK_CLASS(BlockStaticLiquid)

    explicit BlockStaticLiquid(const Material& materialIn)
        : BlockLiquid(materialIn)
    {
    }

    void onNeighborBlockChange(World* worldIn, const BlockPos& pos, const IBlockState* state, Block* neighborBlock) override;
    void updateTick(World* worldIn, const BlockPos& pos, const IBlockState* state, Random& rand) override;

protected:
    bool isSurroundingBlockFlammable(World* worldIn, const BlockPos& pos);
    bool getCanBlockBurn(World* worldIn, const BlockPos& pos);

    void construct() override
    {
        BlockLiquid::construct();
        setTickRandomly(false);
        if (&blockMaterial == &Material::lava)
            setTickRandomly(true);
    }
};

inline const PropertyInteger BlockLiquid::LEVEL("level", 0, 15);

} // namespace mc

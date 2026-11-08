#pragma once

// Original: net/minecraft/block/BlockTorch.java and BlockRedstoneTorch.java

#include "block/BlockBasics.h"
#include "init/Blocks.h"

#include <optional>
#include <unordered_map>
#include <vector>

namespace mc {

class BlockTorch : public Block {
public:
    MC_BLOCK_CLASS(BlockTorch)

    static const PropertyDirection FACING;

    BlockTorch()
        : Block(Material::circuits)
    {
    }

    std::optional<AxisAlignedBB> getCollisionBoundingBox(World*, const BlockPos&, const IBlockState*) override { return std::nullopt; }
    bool isOpaqueCube() const override { return false; }
    bool isFullCube() const override { return false; }
    bool canPlaceBlockAt(World* worldIn, const BlockPos& pos) override;
    const IBlockState* onBlockPlaced(World* worldIn, const BlockPos& pos, EnumFacing facing, float hitX, float hitY, float hitZ, int meta,
        EntityLivingBase* placer) override;
    void onBlockAdded(World* worldIn, const BlockPos& pos, const IBlockState* state) override;
    void onNeighborBlockChange(World* worldIn, const BlockPos& pos, const IBlockState* state, Block* neighborBlock) override;
    std::optional<MovingObjectPosition> collisionRayTrace(World* worldIn, const BlockPos& pos, const Vec3& start, const Vec3& end) override;
    void randomDisplayTick(World* worldIn, const BlockPos& pos, const IBlockState* state, Random& rand) override;
    EnumWorldBlockLayer getBlockLayer() const override { return EnumWorldBlockLayer::CUTOUT; }
    const IBlockState* getStateFromMeta(int meta) const override
    {
        const IBlockState* iblockstate = getDefaultState();
        switch (meta) {
        case 1: return iblockstate->withProperty(FACING, EnumFacing::EAST);
        case 2: return iblockstate->withProperty(FACING, EnumFacing::WEST);
        case 3: return iblockstate->withProperty(FACING, EnumFacing::SOUTH);
        case 4: return iblockstate->withProperty(FACING, EnumFacing::NORTH);
        default: return iblockstate->withProperty(FACING, EnumFacing::UP);
        }
    }
    int getMetaFromState(const IBlockState* state) const override
    {
        switch (state->getValue(FACING)) {
        case EnumFacing::EAST: return 1;
        case EnumFacing::WEST: return 2;
        case EnumFacing::SOUTH: return 3;
        case EnumFacing::NORTH: return 4;
        default: return 5;
        }
    }

protected:
    void construct() override
    {
        Block::construct();
        setDefaultState(blockState->getBaseState()->withProperty(FACING, EnumFacing::UP));
        setTickRandomly(true);
        setCreativeTab(&CreativeTabs::tabDecorations);
    }
    std::unique_ptr<BlockState> createBlockState() override { return makeBlockState({ &FACING }); }

    bool onNeighborChangeInternal(World* worldIn, const BlockPos& pos, const IBlockState* state);
    bool checkForDrop(World* worldIn, const BlockPos& pos, const IBlockState* state);

private:
    bool canPlaceOn(World* worldIn, const BlockPos& pos);
    bool canPlaceAt(World* worldIn, const BlockPos& pos, EnumFacing facing);
};

class BlockRedstoneTorch : public BlockTorch {
public:
    MC_BLOCK_CLASS(BlockRedstoneTorch)

    explicit BlockRedstoneTorch(bool isOnIn)
        : isOnArg(isOnIn)
    {
    }

    int tickRate(World*) override { return 2; }
    void onBlockAdded(World* worldIn, const BlockPos& pos, const IBlockState* state) override;
    void breakBlock(World* worldIn, const BlockPos& pos, const IBlockState* state) override;
    int getWeakPower(IBlockAccess* worldIn, const BlockPos& pos, const IBlockState* state, EnumFacing side) override;
    void randomTick(World*, const BlockPos&, const IBlockState*, Random&) override { }
    void updateTick(World* worldIn, const BlockPos& pos, const IBlockState* state, Random& rand) override;
    void onNeighborBlockChange(World* worldIn, const BlockPos& pos, const IBlockState* state, Block* neighborBlock) override;
    int getStrongPower(IBlockAccess* worldIn, const BlockPos& pos, const IBlockState* state, EnumFacing side) override;
    bool canProvidePower() const override { return true; }
    void randomDisplayTick(World* worldIn, const BlockPos& pos, const IBlockState* state, Random& rand) override;
    Item* getItem(World*, const BlockPos&) override { return Item::getItemFromBlock(Blocks::redstone_torch); }
    bool isAssociatedBlock(const Block* other) const override { return other == Blocks::unlit_redstone_torch || other == Blocks::redstone_torch; }
    Item* getItemDropped(const IBlockState*, Random&, int) const override { return Item::getItemFromBlock(Blocks::redstone_torch); }

protected:
    void construct() override
    {
        BlockTorch::construct();
        isOn = isOnArg;
        setTickRandomly(true);
        setCreativeTab(nullptr);
    }

private:
    struct Toggle {
        BlockPos pos;
        int64_t time;
    };
    // Java: the static Map<World, List<Toggle>> -- the recent turn-offs per world, for the burnout.
    static std::unordered_map<World*, std::vector<Toggle>>& toggles();

    bool isBurnedOut(World* worldIn, const BlockPos& pos, bool turnOff);
    bool shouldBeOff(World* worldIn, const BlockPos& pos, const IBlockState* state);

    bool isOn = false;
    bool isOnArg;
};

inline const PropertyDirection BlockTorch::FACING("facing", [](EnumFacing f) { return f != EnumFacing::DOWN; });

} // namespace mc

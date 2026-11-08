#pragma once

// Original: net/minecraft/block/BlockRailBase.java, BlockRail.java, BlockRailDetector.java and
// BlockRailPowered.java

#include "block/BlockBasics.h"

#include <optional>
#include <vector>

namespace mc {

class BlockRailBase : public Block {
public:
    enum class EnumRailDirection { NORTH_SOUTH, EAST_WEST, ASCENDING_EAST, ASCENDING_WEST, ASCENDING_NORTH, ASCENDING_SOUTH, SOUTH_EAST, SOUTH_WEST, NORTH_WEST, NORTH_EAST };
    static constexpr std::string_view kDirectionNames[10] { "north_south", "east_west", "ascending_east", "ascending_west", "ascending_north",
        "ascending_south", "south_east", "south_west", "north_west", "north_east" };
    static int getMetadata(EnumRailDirection direction) { return static_cast<int>(direction); }
    static bool isAscending(EnumRailDirection d)
    {
        return d == EnumRailDirection::ASCENDING_NORTH || d == EnumRailDirection::ASCENDING_EAST || d == EnumRailDirection::ASCENDING_SOUTH || d == EnumRailDirection::ASCENDING_WEST;
    }
    static EnumRailDirection byMetadata(int meta) { return static_cast<EnumRailDirection>(meta < 0 || meta >= 10 ? 0 : meta); }

    explicit BlockRailBase(bool isPoweredIn)
        : Block(Material::circuits)
        , isPoweredArg(isPoweredIn)
    {
    }

    static bool isRailBlock(World* worldIn, const BlockPos& pos);
    static bool isRailBlock(const IBlockState* state);

    std::optional<AxisAlignedBB> getCollisionBoundingBox(World*, const BlockPos&, const IBlockState*) override { return std::nullopt; }
    bool isOpaqueCube() const override { return false; }
    std::optional<MovingObjectPosition> collisionRayTrace(World* worldIn, const BlockPos& pos, const Vec3& start, const Vec3& end) override;
    void setBlockBoundsBasedOnState(IBlockAccess* worldIn, const BlockPos& pos) override;
    bool isFullCube() const override { return false; }
    bool canPlaceBlockAt(World* worldIn, const BlockPos& pos) override;
    void onBlockAdded(World* worldIn, const BlockPos& pos, const IBlockState* state) override;
    void onNeighborBlockChange(World* worldIn, const BlockPos& pos, const IBlockState* state, Block* neighborBlock) override;
    int getMobilityFlag() const override { return 0; }
    EnumWorldBlockLayer getBlockLayer() const override { return EnumWorldBlockLayer::CUTOUT; }
    void breakBlock(World* worldIn, const BlockPos& pos, const IBlockState* state) override;
    virtual const PropertyEnum<EnumRailDirection>& getShapeProperty() const = 0;

    // Java: BlockRailBase.Rail -- a rail and the positions it connects to, for the shape updates.
    class Rail {
    public:
        Rail(World* worldIn, const BlockPos& pos, const IBlockState* state);
        int countAdjacentRails();
        Rail& func_180364_a(bool p_180364_1_, bool p_180364_2_);
        const IBlockState* getBlockState() const { return state; }

    private:
        void func_180360_a(EnumRailDirection p_180360_1_);
        void func_150651_b();
        bool hasRailAt(const BlockPos& pos);
        std::optional<Rail> findRailAt(const BlockPos& pos);
        bool func_150653_a(const Rail& p_150653_1_);
        bool func_180363_c(const BlockPos& p_180363_1_);
        bool func_150649_b(const Rail& rail);
        void func_150645_c(const Rail& p_150645_1_);
        bool func_180361_d(const BlockPos& p_180361_1_);

        World* world;
        BlockPos pos;
        BlockRailBase* block;
        const IBlockState* state;
        bool isPowered;
        std::vector<BlockPos> field_150657_g;
    };

protected:
    virtual void onNeighborChangedInternal(World*, const BlockPos&, const IBlockState*, Block*) { }
    const IBlockState* func_176564_a(World* worldIn, const BlockPos& p_176564_2_, const IBlockState* p_176564_3_, bool p_176564_4_);

protected:
    void construct() override
    {
        Block::construct();
        isPowered = isPoweredArg;
        setBlockBounds(0.0f, 0.0f, 0.0f, 1.0f, 0.125f, 1.0f);
        setCreativeTab(&CreativeTabs::tabTransport);
    }

    bool isPowered = false;

private:
    bool isPoweredArg;
};

MC_ENUM_TRAITS(BlockRailBase::EnumRailDirection, 10, BlockRailBase::kDirectionNames[static_cast<int>(value)])

class BlockRail : public BlockRailBase {
public:
    MC_BLOCK_CLASS(BlockRail)

    static const PropertyEnum<EnumRailDirection> SHAPE;

    BlockRail()
        : BlockRailBase(false)
    {
    }

    const PropertyEnum<EnumRailDirection>& getShapeProperty() const override { return SHAPE; }
    const IBlockState* getStateFromMeta(int meta) const override { return getDefaultState()->withProperty(SHAPE, byMetadata(meta)); }
    int getMetaFromState(const IBlockState* state) const override { return getMetadata(state->getValue(SHAPE)); }

protected:
    void construct() override
    {
        BlockRailBase::construct();
        setDefaultState(blockState->getBaseState()->withProperty(SHAPE, EnumRailDirection::NORTH_SOUTH));
    }
    std::unique_ptr<BlockState> createBlockState() override { return makeBlockState({ &SHAPE }); }
    void onNeighborChangedInternal(World* worldIn, const BlockPos& pos, const IBlockState* state, Block* neighborBlock) override;
};

// No curves for the powered rails.
inline bool isStraightRail(BlockRailBase::EnumRailDirection d)
{
    using D = BlockRailBase::EnumRailDirection;
    return d != D::NORTH_EAST && d != D::NORTH_WEST && d != D::SOUTH_EAST && d != D::SOUTH_WEST;
}

class BlockRailDetector : public BlockRailBase {
public:
    MC_BLOCK_CLASS(BlockRailDetector)

    static const PropertyEnum<EnumRailDirection> SHAPE;
    static const PropertyBool POWERED;

    BlockRailDetector()
        : BlockRailBase(true)
    {
    }

    int tickRate(World*) override { return 20; }
    bool canProvidePower() const override { return true; }
    using Block::onEntityCollidedWithBlock;
    void onEntityCollidedWithBlock(World* worldIn, const BlockPos& pos, const IBlockState* state, Entity* entityIn) override;
    void randomTick(World*, const BlockPos&, const IBlockState*, Random&) override { }
    void updateTick(World* worldIn, const BlockPos& pos, const IBlockState* state, Random& rand) override;
    int getWeakPower(IBlockAccess*, const BlockPos&, const IBlockState* state, EnumFacing) override { return state->getValue(POWERED) ? 15 : 0; }
    int getStrongPower(IBlockAccess*, const BlockPos&, const IBlockState* state, EnumFacing side) override
    {
        return !state->getValue(POWERED) ? 0 : (side == EnumFacing::UP ? 15 : 0);
    }
    void onBlockAdded(World* worldIn, const BlockPos& pos, const IBlockState* state) override;
    const PropertyEnum<EnumRailDirection>& getShapeProperty() const override { return SHAPE; }
    bool hasComparatorInputOverride() const override { return true; }
    int getComparatorInputOverride(World* worldIn, const BlockPos& pos) override;
    const IBlockState* getStateFromMeta(int meta) const override
    {
        return getDefaultState()->withProperty(SHAPE, byMetadata(meta & 7))->withProperty(POWERED, (meta & 8) > 0);
    }
    int getMetaFromState(const IBlockState* state) const override
    {
        int i = 0;
        i = i | getMetadata(state->getValue(SHAPE));
        if (state->getValue(POWERED))
            i |= 8;
        return i;
    }

protected:
    void construct() override
    {
        BlockRailBase::construct();
        setDefaultState(blockState->getBaseState()->withProperty(POWERED, false)->withProperty(SHAPE, EnumRailDirection::NORTH_SOUTH));
        setTickRandomly(true);
    }
    std::unique_ptr<BlockState> createBlockState() override { return makeBlockState({ &SHAPE, &POWERED }); }

private:
    void updatePoweredState(World* worldIn, const BlockPos& pos, const IBlockState* state);
    AxisAlignedBB getDectectionBox(const BlockPos& pos) const;
};

class BlockRailPowered : public BlockRailBase {
public:
    MC_BLOCK_CLASS(BlockRailPowered)

    static const PropertyEnum<EnumRailDirection> SHAPE;
    static const PropertyBool POWERED;

    BlockRailPowered()
        : BlockRailBase(true)
    {
    }

    const PropertyEnum<EnumRailDirection>& getShapeProperty() const override { return SHAPE; }

    const IBlockState* getStateFromMeta(int meta) const override
    {
        return getDefaultState()->withProperty(SHAPE, byMetadata(meta & 7))->withProperty(POWERED, (meta & 8) > 0);
    }
    int getMetaFromState(const IBlockState* state) const override
    {
        int i = 0;
        i = i | getMetadata(state->getValue(SHAPE));
        if (state->getValue(POWERED))
            i |= 8;
        return i;
    }

protected:
    void construct() override
    {
        BlockRailBase::construct();
        setDefaultState(blockState->getBaseState()->withProperty(SHAPE, EnumRailDirection::NORTH_SOUTH)->withProperty(POWERED, false));
    }
    std::unique_ptr<BlockState> createBlockState() override { return makeBlockState({ &SHAPE, &POWERED }); }

    bool func_176566_a(World* worldIn, const BlockPos& pos, const IBlockState* state, bool p_176566_4_, int p_176566_5_);
    bool func_176567_a(World* worldIn, const BlockPos& p_176567_2_, bool p_176567_3_, int distance, EnumRailDirection p_176567_5_);
    void onNeighborChangedInternal(World* worldIn, const BlockPos& pos, const IBlockState* state, Block* neighborBlock) override;
};

inline const PropertyEnum<BlockRailBase::EnumRailDirection> BlockRail::SHAPE("shape");
inline const PropertyEnum<BlockRailBase::EnumRailDirection> BlockRailDetector::SHAPE("shape", isStraightRail);
inline const PropertyBool BlockRailDetector::POWERED("powered");
inline const PropertyEnum<BlockRailBase::EnumRailDirection> BlockRailPowered::SHAPE("shape", isStraightRail);
inline const PropertyBool BlockRailPowered::POWERED("powered");

} // namespace mc

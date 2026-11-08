#pragma once

// Original: net/minecraft/block/BlockRedstoneDiode.java, BlockRedstoneRepeater.java and
// BlockRedstoneComparator.java

#include "block/BlockDirectional.h"

namespace mc {

class BlockRedstoneDiode : public BlockDirectional {
public:
    explicit BlockRedstoneDiode(bool powered)
        : BlockDirectional(Material::circuits)
        , isRepeaterPoweredArg(powered)
    {
    }

    bool isFullCube() const override { return false; }
    bool canPlaceBlockAt(World* worldIn, const BlockPos& pos) override;
    bool canBlockStay(World* worldIn, const BlockPos& pos);
    void randomTick(World*, const BlockPos&, const IBlockState*, Random&) override { }
    void updateTick(World* worldIn, const BlockPos& pos, const IBlockState* state, Random& rand) override;
    bool shouldSideBeRendered(IBlockAccess*, const BlockPos&, EnumFacing side) override { return getAxis(side) != Axis::Y; }
    int getStrongPower(IBlockAccess* worldIn, const BlockPos& pos, const IBlockState* state, EnumFacing side) override
    {
        return getWeakPower(worldIn, pos, state, side);
    }
    int getWeakPower(IBlockAccess* worldIn, const BlockPos& pos, const IBlockState* state, EnumFacing side) override;
    void onNeighborBlockChange(World* worldIn, const BlockPos& pos, const IBlockState* state, Block* neighborBlock) override;
    virtual bool isLocked(IBlockAccess*, const BlockPos&, const IBlockState*) { return false; }
    bool canProvidePower() const override { return true; }
    const IBlockState* onBlockPlaced(World* worldIn, const BlockPos& pos, EnumFacing facing, float hitX, float hitY, float hitZ, int meta,
        EntityLivingBase* placer) override;
    void onBlockPlacedBy(World* worldIn, const BlockPos& pos, const IBlockState* state, EntityLivingBase* placer, const ItemStack& stack) override;
    void onBlockAdded(World* worldIn, const BlockPos& pos, const IBlockState* state) override;
    void onBlockDestroyedByPlayer(World* worldIn, const BlockPos& pos, const IBlockState* state) override;
    bool isOpaqueCube() const override { return false; }
    static bool isRedstoneRepeaterBlockID(const Block* blockIn);
    bool isAssociated(const Block* other) const;
    bool isFacingTowardsRepeater(World* worldIn, const BlockPos& pos, const IBlockState* state);
    bool isAssociatedBlock(const Block* other) const override { return isAssociated(other); }
    EnumWorldBlockLayer getBlockLayer() const override { return EnumWorldBlockLayer::CUTOUT; }

protected:
    void construct() override
    {
        BlockDirectional::construct();
        isRepeaterPowered = isRepeaterPoweredArg;
        setBlockBounds(0.0f, 0.0f, 0.0f, 1.0f, 0.125f, 1.0f);
    }

    virtual bool isPowered(const IBlockState*) { return isRepeaterPowered; }
    virtual void updateState(World* worldIn, const BlockPos& pos, const IBlockState* state);
    virtual bool shouldBePowered(World* worldIn, const BlockPos& pos, const IBlockState* state);
    virtual int calculateInputStrength(World* worldIn, const BlockPos& pos, const IBlockState* state);
    int getPowerOnSides(IBlockAccess* worldIn, const BlockPos& pos, const IBlockState* state);
    int getPowerOnSide(IBlockAccess* worldIn, const BlockPos& pos, EnumFacing side);
    void notifyNeighbors(World* worldIn, const BlockPos& pos, const IBlockState* state);
    virtual bool canPowerSide(const Block* blockIn) { return blockIn->canProvidePower(); }
    virtual int getActiveSignal(IBlockAccess*, const BlockPos&, const IBlockState*) { return 15; }
    virtual int getTickDelay(const IBlockState* state) const { return getDelay(state); }
    virtual int getDelay(const IBlockState* state) const = 0;
    virtual const IBlockState* getPoweredState(const IBlockState* unpoweredState) const = 0;
    virtual const IBlockState* getUnpoweredState(const IBlockState* poweredState) const = 0;

    bool isRepeaterPowered = false;

private:
    bool isRepeaterPoweredArg;
};

class BlockRedstoneRepeater : public BlockRedstoneDiode {
public:
    MC_BLOCK_CLASS(BlockRedstoneRepeater)

    static const PropertyBool LOCKED;
    static const PropertyInteger DELAY;

    using BlockRedstoneDiode::BlockRedstoneDiode;

    const IBlockState* getActualState(const IBlockState* state, IBlockAccess* worldIn, const BlockPos& pos) override
    {
        return state->withProperty(LOCKED, isLocked(worldIn, pos, state));
    }
    // Java: onBlockActivated cycles DELAY when playerIn.capabilities.allowEdit -- the player comes with stage 8.
    Item* getItem(World*, const BlockPos&) override { return Items::repeater; }
    bool isLocked(IBlockAccess* worldIn, const BlockPos& pos, const IBlockState* state) override { return getPowerOnSides(worldIn, pos, state) > 0; }
    void randomDisplayTick(World* worldIn, const BlockPos& pos, const IBlockState* state, Random& rand) override;
    void breakBlock(World* worldIn, const BlockPos& pos, const IBlockState* state) override;
    const IBlockState* getStateFromMeta(int meta) const override
    {
        return getDefaultState()->withProperty(FACING, getHorizontal(meta))->withProperty(LOCKED, false)->withProperty(DELAY, 1 + (meta >> 2));
    }
    int getMetaFromState(const IBlockState* state) const override
    {
        int i = 0;
        i = i | getHorizontalIndex(state->getValue(FACING));
        i = i | (state->getValue(DELAY) - 1) << 2;
        return i;
    }
    Item* getItemDropped(const IBlockState*, Random&, int) const override { return Items::repeater; }

protected:
    void construct() override
    {
        BlockRedstoneDiode::construct();
        setDefaultState(blockState->getBaseState()->withProperty(FACING, EnumFacing::NORTH)->withProperty(DELAY, 1)->withProperty(LOCKED, false));
    }
    std::unique_ptr<BlockState> createBlockState() override { return makeBlockState({ &FACING, &DELAY, &LOCKED }); }

    int getDelay(const IBlockState* state) const override { return state->getValue(DELAY) * 2; }
    const IBlockState* getPoweredState(const IBlockState* unpoweredState) const override;
    const IBlockState* getUnpoweredState(const IBlockState* poweredState) const override;
    bool canPowerSide(const Block* blockIn) override { return isRedstoneRepeaterBlockID(blockIn); }
};

class BlockRedstoneComparator : public BlockRedstoneDiode {
public:
    MC_BLOCK_CLASS(BlockRedstoneComparator)

    enum class Mode { COMPARE, SUBTRACT };
    static const PropertyBool POWERED;
    static const PropertyEnum<Mode> MODE;

    using BlockRedstoneDiode::BlockRedstoneDiode;

    Item* getItem(World*, const BlockPos&) override { return Items::comparator; }
    // Java: onBlockActivated cycles MODE when playerIn.capabilities.allowEdit (click sound, setBlockState flag 2,
    // onStateChange) -- the player comes with stage 8.
    void updateTick(World* worldIn, const BlockPos& pos, const IBlockState* state, Random& rand) override;
    void onBlockAdded(World* worldIn, const BlockPos& pos, const IBlockState* state) override;
    void breakBlock(World* worldIn, const BlockPos& pos, const IBlockState* state) override;
    bool onBlockEventReceived(World* worldIn, const BlockPos& pos, const IBlockState* state, int eventID, int eventParam) override;
    std::shared_ptr<TileEntity> createNewTileEntity(World* worldIn, int meta) override;
    const IBlockState* getStateFromMeta(int meta) const override
    {
        return getDefaultState()->withProperty(FACING, getHorizontal(meta))->withProperty(POWERED, (meta & 8) > 0)->withProperty(MODE, (meta & 4) > 0 ? Mode::SUBTRACT : Mode::COMPARE);
    }
    int getMetaFromState(const IBlockState* state) const override
    {
        int i = 0;
        i = i | getHorizontalIndex(state->getValue(FACING));
        if (state->getValue(POWERED))
            i |= 8;
        if (state->getValue(MODE) == Mode::SUBTRACT)
            i |= 4;
        return i;
    }
    Item* getItemDropped(const IBlockState*, Random&, int) const override { return Items::comparator; }

protected:
    void construct() override
    {
        BlockRedstoneDiode::construct();
        setDefaultState(blockState->getBaseState()->withProperty(FACING, EnumFacing::NORTH)->withProperty(POWERED, false)->withProperty(MODE, Mode::COMPARE));
        isBlockContainer = true;
    }
    std::unique_ptr<BlockState> createBlockState() override { return makeBlockState({ &FACING, &MODE, &POWERED }); }

    int getDelay(const IBlockState*) const override { return 2; }
    const IBlockState* getPoweredState(const IBlockState* unpoweredState) const override;
    const IBlockState* getUnpoweredState(const IBlockState* poweredState) const override;
    bool isPowered(const IBlockState* state) override { return isRepeaterPowered || state->getValue(POWERED); }
    int getActiveSignal(IBlockAccess* worldIn, const BlockPos& pos, const IBlockState* state) override;
    bool shouldBePowered(World* worldIn, const BlockPos& pos, const IBlockState* state) override;
    int calculateInputStrength(World* worldIn, const BlockPos& pos, const IBlockState* state) override;
    void updateState(World* worldIn, const BlockPos& pos, const IBlockState* state) override;

private:
    int calculateOutput(World* worldIn, const BlockPos& pos, const IBlockState* state);
    void onStateChange(World* worldIn, const BlockPos& pos, const IBlockState* state);
};

MC_ENUM_TRAITS(BlockRedstoneComparator::Mode, 2, value == BlockRedstoneComparator::Mode::COMPARE ? "compare" : "subtract")

inline const PropertyBool BlockRedstoneRepeater::LOCKED("locked");
inline const PropertyInteger BlockRedstoneRepeater::DELAY("delay", 1, 4);
inline const PropertyBool BlockRedstoneComparator::POWERED("powered");
inline const PropertyEnum<BlockRedstoneComparator::Mode> BlockRedstoneComparator::MODE("mode");

} // namespace mc

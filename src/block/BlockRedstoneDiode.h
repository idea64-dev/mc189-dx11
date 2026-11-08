#pragma once

// Original: net/minecraft/block/BlockRedstoneDiode.java, BlockRedstoneRepeater.java and
// BlockRedstoneComparator.java (signal logic and the powered/unpowered swaps come with the world, stage 9)

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
    bool canProvidePower() const override { return true; }
    bool isOpaqueCube() const override { return false; }
    EnumWorldBlockLayer getBlockLayer() const override { return EnumWorldBlockLayer::CUTOUT; }

protected:
    void construct() override
    {
        BlockDirectional::construct();
        isRepeaterPowered = isRepeaterPoweredArg;
        setBlockBounds(0.0f, 0.0f, 0.0f, 1.0f, 0.125f, 1.0f);
    }

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
};

class BlockRedstoneComparator : public BlockRedstoneDiode {
public:
    MC_BLOCK_CLASS(BlockRedstoneComparator)

    enum class Mode { COMPARE, SUBTRACT };
    static const PropertyBool POWERED;
    static const PropertyEnum<Mode> MODE;

    using BlockRedstoneDiode::BlockRedstoneDiode;

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
};

MC_ENUM_TRAITS(BlockRedstoneComparator::Mode, 2, value == BlockRedstoneComparator::Mode::COMPARE ? "compare" : "subtract")

inline const PropertyBool BlockRedstoneRepeater::LOCKED("locked");
inline const PropertyInteger BlockRedstoneRepeater::DELAY("delay", 1, 4);
inline const PropertyBool BlockRedstoneComparator::POWERED("powered");
inline const PropertyEnum<BlockRedstoneComparator::Mode> BlockRedstoneComparator::MODE("mode");

} // namespace mc

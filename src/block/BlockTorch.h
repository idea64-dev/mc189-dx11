#pragma once

// Original: net/minecraft/block/BlockTorch.java and BlockRedstoneTorch.java

#include "block/BlockBasics.h"
#include "init/Blocks.h"

namespace mc {

class BlockTorch : public Block {
public:
    MC_BLOCK_CLASS(BlockTorch)

    static const PropertyDirection FACING;

    BlockTorch()
        : Block(Material::circuits)
    {
    }

    bool isOpaqueCube() const override { return false; }
    bool isFullCube() const override { return false; }
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
};

class BlockRedstoneTorch : public BlockTorch {
public:
    MC_BLOCK_CLASS(BlockRedstoneTorch)

    explicit BlockRedstoneTorch(bool isOnIn)
        : isOnArg(isOnIn)
    {
    }

    bool canProvidePower() const override { return true; }
    bool isAssociatedBlock(const Block* other) const override { return other == Blocks::unlit_redstone_torch || other == Blocks::redstone_torch; }

protected:
    void construct() override
    {
        BlockTorch::construct();
        isOn = isOnArg;
        setTickRandomly(true);
        setCreativeTab(nullptr);
    }

private:
    bool isOn = false;
    bool isOnArg;
};

inline const PropertyDirection BlockTorch::FACING("facing", [](EnumFacing f) { return f != EnumFacing::DOWN; });

} // namespace mc

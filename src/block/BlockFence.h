#pragma once

// Original: net/minecraft/block/BlockFence.java and BlockFenceGate.java

#include "block/BlockDirectional.h"
#include "block/BlockPlanks.h"

namespace mc {

class BlockFence : public Block {
public:
    MC_BLOCK_CLASS(BlockFence)

    static const PropertyBool NORTH;
    static const PropertyBool EAST;
    static const PropertyBool SOUTH;
    static const PropertyBool WEST;

    explicit BlockFence(const Material& materialIn)
        : BlockFence(materialIn, materialIn.getMaterialMapColor())
    {
    }
    BlockFence(const Material& materialIn, const MapColor& mapColorIn)
        : Block(materialIn, mapColorIn)
    {
    }

    bool isOpaqueCube() const override { return false; }
    bool isFullCube() const override { return false; }
    int getMetaFromState(const IBlockState*) const override { return 0; }

protected:
    void construct() override
    {
        Block::construct();
        setDefaultState(blockState->getBaseState()->withProperty(NORTH, false)->withProperty(EAST, false)->withProperty(SOUTH, false)->withProperty(WEST, false));
        setCreativeTab(&CreativeTabs::tabDecorations);
    }
    std::unique_ptr<BlockState> createBlockState() override { return makeBlockState({ &NORTH, &EAST, &WEST, &SOUTH }); }
};

class BlockFenceGate : public BlockDirectional {
public:
    MC_BLOCK_CLASS(BlockFenceGate)

    static const PropertyBool OPEN;
    static const PropertyBool POWERED;
    static const PropertyBool IN_WALL;

    explicit BlockFenceGate(BlockPlanks::EnumType type)
        : BlockDirectional(Material::wood, BlockPlanks::getMapColor(type))
    {
    }

    bool isOpaqueCube() const override { return false; }
    bool isFullCube() const override { return false; }
    const IBlockState* getStateFromMeta(int meta) const override
    {
        return getDefaultState()->withProperty(FACING, getHorizontal(meta))->withProperty(OPEN, (meta & 4) != 0)->withProperty(POWERED, (meta & 8) != 0);
    }
    int getMetaFromState(const IBlockState* state) const override
    {
        int i = 0;
        i = i | getHorizontalIndex(state->getValue(FACING));
        if (state->getValue(POWERED))
            i |= 8;
        if (state->getValue(OPEN))
            i |= 4;
        return i;
    }

protected:
    void construct() override
    {
        BlockDirectional::construct();
        setDefaultState(blockState->getBaseState()->withProperty(OPEN, false)->withProperty(POWERED, false)->withProperty(IN_WALL, false));
        setCreativeTab(&CreativeTabs::tabRedstone);
    }
    std::unique_ptr<BlockState> createBlockState() override { return makeBlockState({ &FACING, &OPEN, &POWERED, &IN_WALL }); }
};

inline const PropertyBool BlockFence::NORTH("north");
inline const PropertyBool BlockFence::EAST("east");
inline const PropertyBool BlockFence::SOUTH("south");
inline const PropertyBool BlockFence::WEST("west");
inline const PropertyBool BlockFenceGate::OPEN("open");
inline const PropertyBool BlockFenceGate::POWERED("powered");
inline const PropertyBool BlockFenceGate::IN_WALL("in_wall");

} // namespace mc

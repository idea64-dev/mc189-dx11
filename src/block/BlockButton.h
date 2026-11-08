#pragma once

// Original: net/minecraft/block/BlockButton.java, BlockButtonStone.java and BlockButtonWood.java

#include "block/BlockBasics.h"

namespace mc {

class BlockButton : public Block {
public:
    static const PropertyDirection FACING;
    static const PropertyBool POWERED;

    explicit BlockButton(bool woodenIn)
        : Block(Material::circuits)
        , wooden(woodenIn)
    {
    }

    bool isOpaqueCube() const override { return false; }
    bool isFullCube() const override { return false; }
    bool canProvidePower() const override { return true; }
    void setBlockBoundsForItemRender() override
    {
        const float f = 0.1875f;
        const float f1 = 0.125f;
        const float f2 = 0.125f;
        setBlockBounds(0.5f - f, 0.5f - f1, 0.5f - f2, 0.5f + f, 0.5f + f1, 0.5f + f2);
    }
    const IBlockState* getStateFromMeta(int meta) const override
    {
        EnumFacing enumfacing;
        switch (meta & 7) {
        case 0: enumfacing = EnumFacing::DOWN; break;
        case 1: enumfacing = EnumFacing::EAST; break;
        case 2: enumfacing = EnumFacing::WEST; break;
        case 3: enumfacing = EnumFacing::SOUTH; break;
        case 4: enumfacing = EnumFacing::NORTH; break;
        default: enumfacing = EnumFacing::UP;
        }
        return getDefaultState()->withProperty(FACING, enumfacing)->withProperty(POWERED, (meta & 8) > 0);
    }
    int getMetaFromState(const IBlockState* state) const override
    {
        int i;
        switch (state->getValue(FACING)) {
        case EnumFacing::EAST: i = 1; break;
        case EnumFacing::WEST: i = 2; break;
        case EnumFacing::SOUTH: i = 3; break;
        case EnumFacing::NORTH: i = 4; break;
        case EnumFacing::DOWN: i = 0; break;
        default: i = 5; break;
        }
        if (state->getValue(POWERED))
            i |= 8;
        return i;
    }

protected:
    void construct() override
    {
        Block::construct();
        setDefaultState(blockState->getBaseState()->withProperty(FACING, EnumFacing::NORTH)->withProperty(POWERED, false));
        setTickRandomly(true);
        setCreativeTab(&CreativeTabs::tabRedstone);
    }
    std::unique_ptr<BlockState> createBlockState() override { return makeBlockState({ &FACING, &POWERED }); }

    const bool wooden;
};

class BlockButtonStone : public BlockButton {
public:
    MC_BLOCK_CLASS(BlockButtonStone)

    BlockButtonStone()
        : BlockButton(false)
    {
    }
};

class BlockButtonWood : public BlockButton {
public:
    MC_BLOCK_CLASS(BlockButtonWood)

    BlockButtonWood()
        : BlockButton(true)
    {
    }
};

inline const PropertyDirection BlockButton::FACING("facing");
inline const PropertyBool BlockButton::POWERED("powered");

} // namespace mc

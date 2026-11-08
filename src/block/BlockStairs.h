#pragma once

// Original: net/minecraft/block/BlockStairs.java (shapes and collision come with the world, stage 9)

#include "block/BlockBasics.h"

namespace mc {

class BlockStairs : public Block {
public:
    MC_BLOCK_CLASS(BlockStairs)

    enum class EnumHalf { TOP, BOTTOM };
    enum class EnumShape { STRAIGHT, INNER_LEFT, INNER_RIGHT, OUTER_LEFT, OUTER_RIGHT };

    static const PropertyDirection FACING;
    static const PropertyEnum<EnumHalf> HALF;
    static const PropertyEnum<EnumShape> SHAPE;

    explicit BlockStairs(const IBlockState* modelStateIn)
        : Block(modelStateIn->getBlock()->getMaterial())
        , modelBlock(modelStateIn->getBlock())
        , modelState(modelStateIn)
    {
    }

    bool isOpaqueCube() const override { return false; }
    bool isFullCube() const override { return false; }
    float getExplosionResistance() const override { return modelBlock->getExplosionResistance(); }
    EnumWorldBlockLayer getBlockLayer() const override { return modelBlock->getBlockLayer(); }
    bool isCollidable() const override { return modelBlock->isCollidable(); }
    bool canCollideCheck(const IBlockState* state, bool hitIfLiquid) const override { return modelBlock->canCollideCheck(state, hitIfLiquid); }
    const MapColor& getMapColor(const IBlockState*) const override { return modelBlock->getMapColor(modelState); }
    const IBlockState* getStateFromMeta(int meta) const override
    {
        const IBlockState* iblockstate = getDefaultState()->withProperty(HALF, (meta & 4) > 0 ? EnumHalf::TOP : EnumHalf::BOTTOM);
        return iblockstate->withProperty(FACING, getFront(5 - (meta & 3)));
    }
    int getMetaFromState(const IBlockState* state) const override
    {
        int i = 0;
        if (state->getValue(HALF) == EnumHalf::TOP)
            i |= 4;
        i = i | (5 - getIndex(state->getValue(FACING)));
        return i;
    }

protected:
    void construct() override
    {
        Block::construct();
        setDefaultState(blockState->getBaseState()->withProperty(FACING, EnumFacing::NORTH)->withProperty(HALF, EnumHalf::BOTTOM)->withProperty(SHAPE, EnumShape::STRAIGHT));
        setHardness(modelBlock->getBlockHardnessValue());
        setResistance(modelBlock->getBlockResistanceValue() / 3.0f);
        setStepSound(*modelBlock->stepSound);
        setLightOpacity(255);
        setCreativeTab(&CreativeTabs::tabBlock);
    }
    std::unique_ptr<BlockState> createBlockState() override { return makeBlockState({ &FACING, &HALF, &SHAPE }); }

private:
    Block* modelBlock;
    const IBlockState* modelState;
};

MC_ENUM_TRAITS(BlockStairs::EnumHalf, 2, value == BlockStairs::EnumHalf::TOP ? "top" : "bottom")
MC_ENUM_TRAITS(BlockStairs::EnumShape, 5,
    value == BlockStairs::EnumShape::STRAIGHT ? "straight"
        : value == BlockStairs::EnumShape::INNER_LEFT ? "inner_left"
        : value == BlockStairs::EnumShape::INNER_RIGHT ? "inner_right"
        : value == BlockStairs::EnumShape::OUTER_LEFT ? "outer_left"
                                                      : "outer_right")

inline const PropertyDirection BlockStairs::FACING("facing", isHorizontalFacing);
inline const PropertyEnum<BlockStairs::EnumHalf> BlockStairs::HALF("half");
inline const PropertyEnum<BlockStairs::EnumShape> BlockStairs::SHAPE("shape");

} // namespace mc

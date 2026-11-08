#pragma once

// Original: net/minecraft/block/BlockBed.java

#include "block/BlockDirectional.h"

namespace mc {

class BlockBed : public BlockDirectional {
public:
    MC_BLOCK_CLASS(BlockBed)

    enum class EnumPartType { HEAD, FOOT };
    static const PropertyEnum<EnumPartType> PART;
    static const PropertyBool OCCUPIED;

    BlockBed()
        : BlockDirectional(Material::cloth)
    {
    }

    bool isFullCube() const override { return false; }
    bool isOpaqueCube() const override { return false; }
    int getMobilityFlag() const override { return 1; }
    EnumWorldBlockLayer getBlockLayer() const override { return EnumWorldBlockLayer::CUTOUT; }
    const IBlockState* getStateFromMeta(int meta) const override
    {
        const EnumFacing enumfacing = getHorizontal(meta);
        return (meta & 8) > 0
            ? getDefaultState()->withProperty(PART, EnumPartType::HEAD)->withProperty(FACING, enumfacing)->withProperty(OCCUPIED, (meta & 4) > 0)
            : getDefaultState()->withProperty(PART, EnumPartType::FOOT)->withProperty(FACING, enumfacing);
    }
    int getMetaFromState(const IBlockState* state) const override
    {
        int i = 0;
        i = i | getHorizontalIndex(state->getValue(FACING));
        if (state->getValue(PART) == EnumPartType::HEAD) {
            i |= 8;
            if (state->getValue(OCCUPIED))
                i |= 4;
        }
        return i;
    }

protected:
    void construct() override
    {
        BlockDirectional::construct();
        setDefaultState(blockState->getBaseState()->withProperty(PART, EnumPartType::FOOT)->withProperty(OCCUPIED, false));
        setBedBounds();
    }
    std::unique_ptr<BlockState> createBlockState() override { return makeBlockState({ &FACING, &PART, &OCCUPIED }); }

private:
    void setBedBounds() { setBlockBounds(0.0f, 0.0f, 0.0f, 1.0f, 0.5625f, 1.0f); }
};

MC_ENUM_TRAITS(BlockBed::EnumPartType, 2, value == BlockBed::EnumPartType::HEAD ? "head" : "foot")

inline const PropertyEnum<BlockBed::EnumPartType> BlockBed::PART("part");
inline const PropertyBool BlockBed::OCCUPIED("occupied");

} // namespace mc

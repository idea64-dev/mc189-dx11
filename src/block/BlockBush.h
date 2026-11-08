#pragma once

// Original: net/minecraft/block/BlockBush.java

#include "block/BlockBasics.h"

namespace mc {

class BlockBush : public Block {
public:
    BlockBush()
        : BlockBush(Material::plants)
    {
    }
    explicit BlockBush(const Material& materialIn)
        : BlockBush(materialIn, materialIn.getMaterialMapColor())
    {
    }
    BlockBush(const Material& materialIn, const MapColor& mapColorIn)
        : Block(materialIn, mapColorIn)
    {
    }

    bool isOpaqueCube() const override { return false; }
    bool isFullCube() const override { return false; }
    EnumWorldBlockLayer getBlockLayer() const override { return EnumWorldBlockLayer::CUTOUT; }

protected:
    void construct() override
    {
        Block::construct();
        setTickRandomly(true);
        const float f = 0.2f;
        setBlockBounds(0.5f - f, 0.0f, 0.5f - f, 0.5f + f, f * 3.0f, 0.5f + f);
        setCreativeTab(&CreativeTabs::tabDecorations);
    }
};

} // namespace mc

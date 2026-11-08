#pragma once

// Original: net/minecraft/block/BlockBreakable.java

#include "block/BlockBasics.h"

namespace mc {

class BlockBreakable : public Block {
public:
    BlockBreakable(const Material& materialIn, bool ignoreSimilarityIn)
        : BlockBreakable(materialIn, ignoreSimilarityIn, materialIn.getMaterialMapColor())
    {
    }
    BlockBreakable(const Material& materialIn, bool ignoreSimilarityIn, const MapColor& mapColorIn)
        : Block(materialIn, mapColorIn)
        , ignoreSimilarity(ignoreSimilarityIn)
    {
    }

    bool isOpaqueCube() const override { return false; }
    bool shouldSideBeRendered(IBlockAccess* worldIn, const BlockPos& pos, EnumFacing side) override;

protected:
    bool ignoreSimilarity;
};

} // namespace mc

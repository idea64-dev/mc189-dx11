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
    // Java: shouldSideBeRendered uses ignoreSimilarity (stage 7).

protected:
    bool ignoreSimilarity;
};

} // namespace mc

#pragma once

// Original: net/minecraft/client/renderer/block/model/BakedQuad.java, BreakingFour.java and
// client/renderer/EnumFaceDirection.java
// A baked quad is four vertices of seven ints each, the layout of DefaultVertexFormats.BLOCK / ITEM:
//   [0..2] x, y, z as float bits (block space, 0..1)   [3] colour 0xAABBGGRR as an int (shade, -1 = white)
//   [4..5] u, v as float bits (atlas space)             [6] 0 (lightmap / normal slot, filled when rendering)

#include "util/EnumFacing.h"

#include <array>
#include <cstdint>

namespace mc {

class TextureAtlasSprite;

struct BakedQuad {
    std::array<int32_t, 28> vertexData {};
    int tintIndex = -1;
    EnumFacing face = EnumFacing::UP;

    bool hasTintIndex() const { return tintIndex != -1; }
    int getTintIndex() const { return tintIndex; }
    EnumFacing getFace() const { return face; }
    const std::array<int32_t, 28>& getVertexData() const { return vertexData; }
};

// Java: new BreakingFour(quad, texture) -- the quad with its UVs remapped onto the given sprite (the block
// breaking overlay) and its face recomputed from the vertices.
BakedQuad makeBreakingFour(const BakedQuad& quad, const TextureAtlasSprite& texture);

// Java: EnumFaceDirection -- for each face, the corner order of its four vertices as indices into a
// {west, down, north, east, up, south} bounds array (indexed by EnumFacing).
struct VertexInformation {
    int xIndex;
    int yIndex;
    int zIndex;
};

const VertexInformation& getVertexInformation(EnumFacing facing, int index);

} // namespace mc

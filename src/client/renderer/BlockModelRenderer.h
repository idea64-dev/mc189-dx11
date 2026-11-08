#pragma once

// Original: net/minecraft/client/renderer/BlockModelRenderer.java
// Tessellates baked block models into a WorldRenderer: smooth lighting (ambient occlusion) or flat lighting,
// per-face culling against the neighbours and the biome/tint colour multipliers.

#include "block/Block.h"
#include "client/renderer/block/model/BakedQuad.h"

#include <array>
#include <bitset>
#include <vector>

namespace mc {

class IBakedModel;
class IBlockAccess;
class IBlockState;
class WorldRenderer;

class BlockModelRenderer {
public:
    bool renderModel(IBlockAccess* blockAccessIn, const IBakedModel* modelIn, const IBlockState* blockStateIn, const BlockPos& blockPosIn,
        WorldRenderer& worldRendererIn);
    bool renderModel(IBlockAccess* blockAccessIn, const IBakedModel* modelIn, const IBlockState* blockStateIn, const BlockPos& blockPosIn,
        WorldRenderer& worldRendererIn, bool checkSides);
    bool renderModelAmbientOcclusion(IBlockAccess* blockAccessIn, const IBakedModel* modelIn, Block* blockIn, const BlockPos& blockPosIn,
        WorldRenderer& worldRendererIn, bool checkSides);
    bool renderModelStandard(IBlockAccess* blockAccessIn, const IBakedModel* modelIn, Block* blockIn, const BlockPos& blockPosIn,
        WorldRenderer& worldRendererIn, bool checkSides);
    void renderModelBrightnessColor(const IBakedModel* bakedModel, float brightness, float red, float green, float blue);
    void renderModelBrightness(const IBakedModel* model, const IBlockState* state, float brightness, bool useColor);

private:
    using Bounds = std::array<float, 12>;
    using BoundsFlags = std::bitset<3>;

    class AmbientOcclusionFace {
    public:
        void updateVertexBrightness(IBlockAccess* blockAccessIn, Block* blockIn, const BlockPos& blockPosIn, EnumFacing facingIn,
            const Bounds& quadBounds, const BoundsFlags& boundsFlags);

        std::array<float, 4> vertexColorMultiplier {};
        std::array<int, 4> vertexBrightness {};

    private:
        static int getAoBrightness(int br1, int br2, int br3, int br4);
        static int getVertexBrightness(int b1, int b2, int b3, int b4, float w1, float w2, float w3, float w4);
    };

    void renderModelAmbientOcclusionQuads(IBlockAccess* blockAccessIn, Block* blockIn, const BlockPos& blockPosIn, WorldRenderer& worldRendererIn,
        const std::vector<BakedQuad>& listQuadsIn, Bounds& quadBounds, BoundsFlags& boundsFlags, AmbientOcclusionFace& aoFaceIn);
    static void fillQuadBounds(Block* blockIn, const std::array<int32_t, 28>& vertexData, EnumFacing facingIn, Bounds* quadBounds,
        BoundsFlags& boundsFlags);
    void renderModelStandardQuads(IBlockAccess* blockAccessIn, Block* blockIn, const BlockPos& blockPosIn, EnumFacing faceIn, int brightnessIn,
        bool ownBrightness, WorldRenderer& worldRendererIn, const std::vector<BakedQuad>& listQuadsIn, BoundsFlags& boundsFlags);
    void renderModelBrightnessColorQuads(float brightness, float red, float green, float blue, const std::vector<BakedQuad>& listQuads);
};

} // namespace mc

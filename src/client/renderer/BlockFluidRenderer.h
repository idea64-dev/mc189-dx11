#pragma once

// Original: net/minecraft/client/renderer/BlockFluidRenderer.java -- water and lava (render type 1).

#include "util/BlockPos.h"

#include <array>

namespace mc {

class IBlockAccess;
class IBlockState;
class Material;
class TextureAtlasSprite;
class WorldRenderer;

class BlockFluidRenderer {
public:
    BlockFluidRenderer();

    void initAtlasSprites();
    bool renderFluid(IBlockAccess* blockAccess, const IBlockState* blockStateIn, const BlockPos& blockPosIn, WorldRenderer& worldRendererIn);

private:
    float getFluidHeight(IBlockAccess* blockAccess, const BlockPos& blockPosIn, const Material& blockMaterial);

    std::array<TextureAtlasSprite*, 2> atlasSpritesLava {};
    std::array<TextureAtlasSprite*, 2> atlasSpritesWater {};
};

} // namespace mc

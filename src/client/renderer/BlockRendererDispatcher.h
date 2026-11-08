#pragma once

// Original: net/minecraft/client/renderer/BlockRendererDispatcher.java
// Picks the renderer for a block's render type: 1 fluids, 3 baked models; 2 (chests, drawn by tile entity
// renderers) and -1 draw nothing into the chunk. The chest brightness path for falling/held blocks comes with
// the tile entity renderers (stage 10).

#include "client/renderer/BlockFluidRenderer.h"
#include "client/renderer/BlockModelRenderer.h"
#include "client/resources/IResourceManager.h"

namespace mc {

class BlockModelShapes;
class GameSettings;
class IBakedModel;
class TextureAtlasSprite;

class BlockRendererDispatcher : public IResourceManagerReloadListener {
public:
    BlockRendererDispatcher(BlockModelShapes& blockModelShapesIn, GameSettings& gameSettingsIn);

    BlockModelShapes& getBlockModelShapes() { return blockModelShapes; }
    void renderBlockDamage(const IBlockState* state, const BlockPos& pos, const TextureAtlasSprite& texture, IBlockAccess* blockAccess);
    bool renderBlock(const IBlockState* state, const BlockPos& pos, IBlockAccess* blockAccess, WorldRenderer& worldRendererIn);
    BlockModelRenderer& getBlockModelRenderer() { return blockModelRenderer; }
    const IBakedModel* getModelFromBlockState(const IBlockState* state, IBlockAccess* worldIn, const BlockPos* pos);
    void renderBlockBrightness(const IBlockState* state, float brightness);
    bool isRenderTypeChest(const Block* block, int meta) const;
    void onResourceManagerReload(const IResourceManager& resourceManager) override;

private:
    const IBakedModel* getBakedModel(const IBlockState* state, const BlockPos* pos);

    BlockModelShapes& blockModelShapes;
    GameSettings& gameSettings;
    BlockModelRenderer blockModelRenderer;
    BlockFluidRenderer fluidRenderer;
};

} // namespace mc

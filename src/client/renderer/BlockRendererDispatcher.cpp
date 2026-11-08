#include "client/renderer/BlockRendererDispatcher.h"

#include "block/Block.h"
#include "block/state/BlockState.h"
#include "client/renderer/BlockModelShapes.h"
#include "client/renderer/Tessellator.h"
#include "client/resources/model/IBakedModel.h"
#include "client/settings/GameSettings.h"
#include "util/MathHelper.h"
#include "world/IBlockAccess.h"
#include "world/WorldType.h"

namespace mc {

namespace {

// Java: MathHelper.getPositionRandom(pos)
int64_t positionRandom(const BlockPos& pos)
{
    return MathHelper::getCoordinateRandom(pos.getX(), pos.getY(), pos.getZ());
}

} // namespace

BlockRendererDispatcher::BlockRendererDispatcher(BlockModelShapes& blockModelShapesIn, GameSettings& gameSettingsIn)
    : blockModelShapes(blockModelShapesIn)
    , gameSettings(gameSettingsIn)
{
}

void BlockRendererDispatcher::renderBlockDamage(const IBlockState* state, const BlockPos& pos, const TextureAtlasSprite& texture, IBlockAccess* blockAccess)
{
    Block* block = state->getBlock();
    if (block->getRenderType() == 3) {
        state = block->getActualState(state, blockAccess, pos);
        const IBakedModel* ibakedmodel = blockModelShapes.getModelForState(state);
        const std::shared_ptr<IBakedModel> ibakedmodel1 = SimpleBakedModel::Builder(*ibakedmodel, texture).makeBakedModel();
        blockModelRenderer.renderModel(blockAccess, ibakedmodel1.get(), state, pos, Tessellator::getInstance().getWorldRenderer());
    }
}

bool BlockRendererDispatcher::renderBlock(const IBlockState* state, const BlockPos& pos, IBlockAccess* blockAccess, WorldRenderer& worldRendererIn)
{
    switch (state->getBlock()->getRenderType()) {
    case 1:
        return fluidRenderer.renderFluid(blockAccess, state, pos, worldRendererIn);
    case 3:
        return blockModelRenderer.renderModel(blockAccess, getModelFromBlockState(state, blockAccess, &pos), state, pos, worldRendererIn);
    default:
        return false;
    }
}

const IBakedModel* BlockRendererDispatcher::getBakedModel(const IBlockState* state, const BlockPos* pos)
{
    const IBakedModel* ibakedmodel = blockModelShapes.getModelForState(state);
    if (pos != nullptr && gameSettings.allowBlockAlternatives) {
        if (const auto* weighted = dynamic_cast<const WeightedBakedModel*>(ibakedmodel))
            ibakedmodel = weighted->getAlternativeModel(positionRandom(*pos));
    }
    return ibakedmodel;
}

const IBakedModel* BlockRendererDispatcher::getModelFromBlockState(const IBlockState* state, IBlockAccess* worldIn, const BlockPos* pos)
{
    Block* block = state->getBlock();
    if (worldIn->getWorldType() != &WorldType::DEBUG_WORLD) {
        // Java swallows exceptions from getActualState here.
        try {
            state = block->getActualState(state, worldIn, *pos);
        } catch (const std::exception&) {
        }
    }
    return getBakedModel(state, pos);
}

void BlockRendererDispatcher::renderBlockBrightness(const IBlockState* state, float brightness)
{
    // Render type 2 (chests) is drawn by the chest renderer, which comes with the tile entity renderers.
    if (state->getBlock()->getRenderType() == 3)
        blockModelRenderer.renderModelBrightness(getBakedModel(state, nullptr), state, brightness, true);
}

bool BlockRendererDispatcher::isRenderTypeChest(const Block* block, int /*meta*/) const
{
    if (block == nullptr)
        return false;
    const int i = block->getRenderType();
    return i == 3 ? false : i == 2;
}

void BlockRendererDispatcher::onResourceManagerReload(const IResourceManager& /*resourceManager*/)
{
    fluidRenderer.initAtlasSprites();
}

} // namespace mc

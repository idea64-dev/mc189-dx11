#include "client/resources/model/ModelManager.h"

#include "client/renderer/BlockModelShapes.h"
#include "client/resources/model/IBakedModel.h"
#include "client/resources/model/ModelBakery.h"

namespace mc {

ModelManager::ModelManager(TextureMap& textures)
    : texMap(textures)
    , modelProvider(std::make_unique<BlockModelShapes>(*this))
{
}

ModelManager::~ModelManager() = default;

void ModelManager::onResourceManagerReload(const IResourceManager& resourceManager)
{
    ModelBakery bakery(resourceManager, texMap, *modelProvider);
    modelRegistry = bakery.setupModelRegistry();
    const auto it = modelRegistry.find(ModelBakery::MODEL_MISSING);
    defaultModel = it == modelRegistry.end() ? nullptr : it->second.get();
    modelProvider->reloadModels();
}

IBakedModel* ModelManager::getModel(const ModelResourceLocation& modelLocation) const
{
    const auto it = modelRegistry.find(modelLocation);
    return it == modelRegistry.end() || !it->second ? defaultModel : it->second.get();
}

} // namespace mc

#pragma once

// Original: net/minecraft/client/resources/model/ModelManager.java
// Rebakes every block and item model (ModelBakery) on resource reload, restitching the block atlas.

#include "client/resources/IResourceManager.h"
#include "client/resources/model/ModelResourceLocation.h"

#include <memory>
#include <unordered_map>

namespace mc {

class BlockModelShapes;
class IBakedModel;
class TextureMap;

using ModelRegistry = std::unordered_map<ModelResourceLocation, std::shared_ptr<IBakedModel>>;

class ModelManager : public IResourceManagerReloadListener {
public:
    explicit ModelManager(TextureMap& textures);
    ~ModelManager() override;

    void onResourceManagerReload(const IResourceManager& resourceManager) override;
    // The missing model for unknown locations.
    IBakedModel* getModel(const ModelResourceLocation& modelLocation) const;
    IBakedModel* getMissingModel() const { return defaultModel; }
    TextureMap& getTextureMap() const { return texMap; }
    BlockModelShapes& getBlockModelShapes() const { return *modelProvider; }
    const ModelRegistry& getModelRegistry() const { return modelRegistry; }

private:
    ModelRegistry modelRegistry;
    TextureMap& texMap;
    std::unique_ptr<BlockModelShapes> modelProvider;
    IBakedModel* defaultModel = nullptr;
};

} // namespace mc

#pragma once

// Original: net/minecraft/client/resources/model/ModelBakery.java
// Loads the blockstates and models every block state and item needs, resolves parents, registers their
// textures in the atlas, then bakes everything into IBakedModels keyed by "block#variant" / "item#inventory".

#include "client/renderer/block/model/FaceBakery.h"
#include "client/renderer/block/model/ItemModelGenerator.h"
#include "client/renderer/block/model/ModelBlockDefinition.h"
#include "client/resources/model/ModelManager.h"

#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

namespace mc {

class BlockModelShapes;
class IResourceManager;
class Item;
class ModelBlock;
class ModelRotation;
class TextureAtlasSprite;
class TextureMap;

class ModelBakery {
public:
    static const ModelResourceLocation MODEL_MISSING;

    ModelBakery(const IResourceManager& resourceManager, TextureMap& textureMap, BlockModelShapes& blockModelShapes);
    ~ModelBakery();

    ModelRegistry setupModelRegistry();

private:
    void loadVariantItemModels();
    void loadVariants(const std::vector<ModelResourceLocation>& locations);
    void registerVariant(const ModelBlockDefinition& definition, const ModelResourceLocation& location);
    const ModelBlockDefinition& getModelBlockDefinition(const ResourceLocation& location);
    void loadVariantModels();
    std::shared_ptr<ModelBlock> loadModel(const ResourceLocation& location) const;
    void loadItemModels();
    void registerVariantNames();
    std::vector<std::string> getVariantNames(const Item* item) const;
    void bakeBlockModels();
    std::vector<ResourceLocation> getVariantsTextureLocations() const;
    std::shared_ptr<IBakedModel> bakeModel(const ModelBlock& model, const ModelRotation& rotation, bool uvLocked) const;
    void loadModelsCheck();
    void loadModels();
    std::vector<ResourceLocation> getParentPath(const ResourceLocation& location) const;
    void getTextureLocations(const ModelBlock& model, std::vector<ResourceLocation>& out) const;
    void loadSprites();
    std::vector<ResourceLocation> getItemsTextureLocations() const;
    bool hasItemModel(const ModelBlock* model) const;
    bool isCustomRenderer(const ModelBlock* model) const;
    void bakeItemModels();
    TextureAtlasSprite* sprite(const std::string& name) const;
    ModelBlock* model(const ResourceLocation& location) const;
    void putModel(const ResourceLocation& location, std::shared_ptr<ModelBlock> modelBlock);

    const IResourceManager& resourceManager;
    std::unordered_map<ResourceLocation, std::shared_ptr<TextureAtlasSprite>> sprites;
    // Java: LinkedHashMaps; the vectors keep the insertion order.
    std::unordered_map<ResourceLocation, std::shared_ptr<ModelBlock>> models;
    std::vector<ResourceLocation> modelOrder;
    std::vector<std::shared_ptr<ModelBlock>> replacedModels;
    std::unordered_map<ModelResourceLocation, const ModelBlockDefinition::Variants*> variants;
    std::vector<ModelResourceLocation> variantOrder;
    TextureMap& textureMap;
    BlockModelShapes& blockModelShapes;
    FaceBakery faceBakery;
    ItemModelGenerator itemModelGenerator;
    ModelRegistry bakedRegistry;
    std::vector<std::pair<std::string, ResourceLocation>> itemLocations;
    std::unordered_map<ResourceLocation, std::unique_ptr<ModelBlockDefinition>> blockDefinitions;
    std::unordered_map<const Item*, std::vector<std::string>> variantNames;
    ModelBlockDefinition::Variants missingVariants;
};

} // namespace mc

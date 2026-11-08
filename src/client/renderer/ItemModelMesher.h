#pragma once

// Original: net/minecraft/client/renderer/ItemModelMesher.java and ItemMeshDefinition.java, with the
// registrations of RenderItem.registerItems().
// Finds the baked model of an item stack: registered per (item, metadata), else by a per-item function.

#include "client/resources/model/ModelResourceLocation.h"

#include <functional>
#include <span>
#include <unordered_map>

namespace mc {

class IBakedModel;
class Item;
class ItemStack;
class ModelManager;
class TextureAtlasSprite;

// Java: ItemMeshDefinition.getModelLocation
using ItemMeshDefinition = std::function<ModelResourceLocation(const ItemStack&)>;

class ItemModelMesher {
public:
    explicit ItemModelMesher(ModelManager& modelManagerIn);

    TextureAtlasSprite* getParticleIcon(const Item* item, int meta = 0) const;
    // Never null: the missing model when nothing is registered.
    IBakedModel* getItemModel(const ItemStack& stack) const;
    void registerItem(const Item* item, int meta, const ModelResourceLocation& location);
    void registerItem(const Item* item, ItemMeshDefinition definition);
    ModelManager& getModelManager() const { return modelManager; }
    void rebuildCache();

    // Java: RenderItem.registerItems(), the vanilla registrations (run by the RenderItem constructor).
    void registerVanillaItems();

    struct Registration {
        const char* item;
        int meta;
        const char* model;
    };
    // The (item, metadata, model) registrations of registerVanillaItems, in call order.
    static std::span<const Registration> vanillaRegistrations();

private:
    static int getMetadata(const ItemStack& stack);
    IBakedModel* getItemModel(const Item* item, int meta) const;
    static int getIndex(const Item* item, int meta);

    std::unordered_map<int, ModelResourceLocation> simpleShapes;
    std::unordered_map<int, IBakedModel*> simpleShapesCache;
    std::unordered_map<const Item*, ItemMeshDefinition> shapers;
    ModelManager& modelManager;
};

} // namespace mc

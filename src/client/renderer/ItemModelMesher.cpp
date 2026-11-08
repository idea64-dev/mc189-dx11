#include "client/renderer/ItemModelMesher.h"

#include "client/resources/model/IBakedModel.h"
#include "client/resources/model/ModelManager.h"
#include "item/Item.h"
#include "item/ItemStack.h"

namespace mc {

namespace {

constexpr ItemModelMesher::Registration kRegistrations[] {
#include "client/renderer/ItemModelRegistrations.inc"
};

} // namespace

std::span<const ItemModelMesher::Registration> ItemModelMesher::vanillaRegistrations()
{
    return kRegistrations;
}

ItemModelMesher::ItemModelMesher(ModelManager& modelManagerIn)
    : modelManager(modelManagerIn)
{
}

TextureAtlasSprite* ItemModelMesher::getParticleIcon(const Item* item, int meta) const
{
    return getItemModel(ItemStack(const_cast<Item*>(item), 1, meta))->getParticleTexture();
}

IBakedModel* ItemModelMesher::getItemModel(const ItemStack& stack) const
{
    const Item* item = stack.getItem();
    IBakedModel* model = getItemModel(item, getMetadata(stack));
    if (!model) {
        if (const auto it = shapers.find(item); it != shapers.end())
            model = modelManager.getModel(it->second(stack));
    }
    return model ? model : modelManager.getMissingModel();
}

int ItemModelMesher::getMetadata(const ItemStack& stack)
{
    return stack.isItemStackDamageable() ? 0 : stack.getMetadata();
}

IBakedModel* ItemModelMesher::getItemModel(const Item* item, int meta) const
{
    const auto it = simpleShapesCache.find(getIndex(item, meta));
    return it == simpleShapesCache.end() ? nullptr : it->second;
}

int ItemModelMesher::getIndex(const Item* item, int meta)
{
    return static_cast<int>(static_cast<uint32_t>(Item::getIdFromItem(item)) << 16) | meta;
}

void ItemModelMesher::registerItem(const Item* item, int meta, const ModelResourceLocation& location)
{
    simpleShapes.insert_or_assign(getIndex(item, meta), location);
    simpleShapesCache[getIndex(item, meta)] = modelManager.getModel(location);
}

void ItemModelMesher::registerItem(const Item* item, ItemMeshDefinition definition)
{
    shapers[item] = std::move(definition);
}

void ItemModelMesher::rebuildCache()
{
    simpleShapesCache.clear();
    for (const auto& [index, location] : simpleShapes)
        simpleShapesCache[index] = modelManager.getModel(location);
}

void ItemModelMesher::registerVanillaItems()
{
    for (const Registration& registration : kRegistrations)
        registerItem(Item::itemRegistry.getObject(ResourceLocation(registration.item)), registration.meta,
            ModelResourceLocation(registration.model, "inventory"));

    const auto item = [](const char* name) { return Item::itemRegistry.getObject(ResourceLocation(name)); };
    registerItem(item("potion"), [](const ItemStack& stack) {
        // ItemPotion.isSplash
        return (stack.getMetadata() & 16384) != 0 ? ModelResourceLocation("bottle_splash", "inventory") : ModelResourceLocation("bottle_drinkable", "inventory");
    });
    registerItem(item("spawn_egg"), [](const ItemStack&) { return ModelResourceLocation("spawn_egg", "inventory"); });
    registerItem(item("banner"), [](const ItemStack&) { return ModelResourceLocation("banner", "inventory"); });
    registerItem(item("enchanted_book"), [](const ItemStack&) { return ModelResourceLocation("enchanted_book", "inventory"); });
    registerItem(item("filled_map"), [](const ItemStack&) { return ModelResourceLocation("filled_map", "inventory"); });
}

} // namespace mc

#include "client/resources/model/ModelBakery.h"

#include "block/Block.h"
#include "client/renderer/BlockModelShapes.h"
#include "client/renderer/block/model/ModelBlock.h"
#include "client/renderer/texture/TextureAtlasSprite.h"
#include "client/renderer/texture/TextureMap.h"
#include "client/resources/IResourceManager.h"
#include "client/resources/IResourcePack.h"
#include "client/resources/model/IBakedModel.h"
#include "client/resources/model/ModelRotation.h"
#include "core/JavaString.h"
#include "core/Log.h"
#include "item/Item.h"

#include <algorithm>
#include <deque>
#include <unordered_set>

namespace mc {

const ModelResourceLocation ModelBakery::MODEL_MISSING("builtin/missing", "missing");

namespace {

const char* const kBuiltInMissing = R"({ "textures": {   "particle": "missingno",   "missingno": "missingno"}, "elements": [ {     "from": [ 0, 0, 0 ],     "to": [ 16, 16, 16 ],     "faces": {         "down":  { "uv": [ 0, 0, 16, 16 ], "cullface": "down", "texture": "#missingno" },         "up":    { "uv": [ 0, 0, 16, 16 ], "cullface": "up", "texture": "#missingno" },         "north": { "uv": [ 0, 0, 16, 16 ], "cullface": "north", "texture": "#missingno" },         "south": { "uv": [ 0, 0, 16, 16 ], "cullface": "south", "texture": "#missingno" },         "west":  { "uv": [ 0, 0, 16, 16 ], "cullface": "west", "texture": "#missingno" },         "east":  { "uv": [ 0, 0, 16, 16 ], "cullface": "east", "texture": "#missingno" }    }}]})";
const char* const kMarkerModel = R"({"elements":[{  "from": [0, 0, 0],   "to": [16, 16, 16],   "faces": {       "down": {"uv": [0, 0, 16, 16], "texture":""}   }}]})";

std::shared_ptr<ModelBlock> makeMarker(const char* name)
{
    std::shared_ptr<ModelBlock> model = ModelBlock::deserialize(kMarkerModel);
    model->name = name;
    return model;
}

// Identity matters: an item model whose root is one of these is generated, a compass, a clock or drawn by code.
const std::shared_ptr<ModelBlock>& modelGenerated()
{
    static const std::shared_ptr<ModelBlock> model = makeMarker("generation marker");
    return model;
}

const std::shared_ptr<ModelBlock>& modelCompass()
{
    static const std::shared_ptr<ModelBlock> model = makeMarker("compass generation marker");
    return model;
}

const std::shared_ptr<ModelBlock>& modelClock()
{
    static const std::shared_ptr<ModelBlock> model = makeMarker("class generation marker");
    return model;
}

const std::shared_ptr<ModelBlock>& modelEntity()
{
    static const std::shared_ptr<ModelBlock> model = makeMarker("block entity marker");
    return model;
}

const std::vector<ResourceLocation>& builtinTextures()
{
    static const std::vector<ResourceLocation> textures = [] {
        std::vector<ResourceLocation> list;
        for (const char* name : { "blocks/water_flow", "blocks/water_still", "blocks/lava_flow", "blocks/lava_still" })
            list.emplace_back(name);
        for (int i = 0; i < 10; ++i)
            list.emplace_back("blocks/destroy_stage_" + std::to_string(i));
        for (const char* name : { "items/empty_armor_slot_helmet", "items/empty_armor_slot_chestplate", "items/empty_armor_slot_leggings",
                 "items/empty_armor_slot_boots" })
            list.emplace_back(name);
        return list;
    }();
    return textures;
}

const Item* itemByName(std::string_view name)
{
    return Item::itemRegistry.getObject(ResourceLocation(name));
}

const Item* itemOfBlock(std::string_view name)
{
    return Item::getItemFromBlock(Block::getBlockFromName(name));
}

} // namespace

ModelBakery::ModelBakery(const IResourceManager& resourceManagerIn, TextureMap& textureMapIn, BlockModelShapes& blockModelShapesIn)
    : resourceManager(resourceManagerIn)
    , textureMap(textureMapIn)
    , blockModelShapes(blockModelShapesIn)
{
}

ModelBakery::~ModelBakery() = default;

ModelRegistry ModelBakery::setupModelRegistry()
{
    loadVariantItemModels();
    loadModelsCheck();
    loadSprites();
    bakeItemModels();
    bakeBlockModels();
    return std::move(bakedRegistry);
}

ModelBlock* ModelBakery::model(const ResourceLocation& location) const
{
    const auto it = models.find(location);
    return it == models.end() ? nullptr : it->second.get();
}

void ModelBakery::putModel(const ResourceLocation& location, std::shared_ptr<ModelBlock> modelBlock)
{
    const auto it = models.find(location);
    if (it == models.end()) {
        models.emplace(location, std::move(modelBlock));
        modelOrder.push_back(location);
        return;
    }
    // Other models may still point at the replaced one as their parent (Java's garbage collector keeps it).
    if (it->second)
        replacedModels.push_back(std::move(it->second));
    it->second = std::move(modelBlock);
}

TextureAtlasSprite* ModelBakery::sprite(const std::string& name) const
{
    const auto it = sprites.find(ResourceLocation(name));
    return it == sprites.end() ? nullptr : it->second.get();
}

void ModelBakery::loadVariantItemModels()
{
    std::vector<ModelResourceLocation> locations;
    for (const auto& [state, location] : blockModelShapes.getBlockStateMapper().putAllStateModelLocations())
        locations.push_back(location);
    loadVariants(locations);

    missingVariants = { MODEL_MISSING.getVariant(), { { ResourceLocation(MODEL_MISSING.getResourcePath()), &ModelRotation::X0_Y0, false, 1 } } };
    if (variants.insert_or_assign(MODEL_MISSING, &missingVariants).second)
        variantOrder.push_back(MODEL_MISSING);

    const ResourceLocation itemFrame("item_frame");
    const ModelBlockDefinition& definition = getModelBlockDefinition(itemFrame);
    registerVariant(definition, ModelResourceLocation(itemFrame, "normal"));
    registerVariant(definition, ModelResourceLocation(itemFrame, "map"));
    loadVariantModels();
    loadItemModels();
}

void ModelBakery::loadVariants(const std::vector<ModelResourceLocation>& locations)
{
    for (const ModelResourceLocation& location : locations) {
        try {
            const ModelBlockDefinition& definition = getModelBlockDefinition(location);
            try {
                registerVariant(definition, location);
            } catch (const std::exception&) {
                Log::warn("Unable to load variant: {} from {}", location.getVariant(), location.toString());
            }
        } catch (const std::exception& e) {
            Log::warn("Unable to load definition {} ({})", location.toString(), e.what());
        }
    }
}

void ModelBakery::registerVariant(const ModelBlockDefinition& definition, const ModelResourceLocation& location)
{
    const ModelBlockDefinition::Variants& found = definition.getVariants(location.getVariant());
    if (variants.insert_or_assign(location, &found).second)
        variantOrder.push_back(location);
}

const ModelBlockDefinition& ModelBakery::getModelBlockDefinition(const ResourceLocation& location)
{
    const ResourceLocation blockStateLocation(location.getResourceDomain(), "blockstates/" + location.getResourcePath() + ".json");
    if (const auto it = blockDefinitions.find(blockStateLocation); it != blockDefinitions.end())
        return *it->second;

    std::vector<ModelBlockDefinition> list;
    for (const std::shared_ptr<IResource>& resource : resourceManager.getAllResources(blockStateLocation)) {
        const std::vector<uint8_t>& data = resource->getInputStream();
        try {
            list.push_back(ModelBlockDefinition::parseFromReader(std::string_view(reinterpret_cast<const char*>(data.data()), data.size())));
        } catch (const std::exception& e) {
            throw std::runtime_error("Encountered an exception when loading model definition of '" + location.toString() + "' from: '"
                + resource->getResourceLocation().toString() + "' in resourcepack: '" + resource->getResourcePackName() + "': " + e.what());
        }
    }
    auto definition = std::make_unique<ModelBlockDefinition>(list);
    const ModelBlockDefinition& result = *definition;
    blockDefinitions.emplace(blockStateLocation, std::move(definition));
    return result;
}

void ModelBakery::loadVariantModels()
{
    for (const ModelResourceLocation& location : variantOrder) {
        for (const ModelBlockDefinition::Variant& variant : variants.at(location)->getVariants()) {
            const ResourceLocation& modelLocation = variant.getModelLocation();
            if (!model(modelLocation)) {
                try {
                    putModel(modelLocation, loadModel(modelLocation));
                } catch (const std::exception& e) {
                    Log::warn("Unable to load block model: '{}' for variant: '{}' ({})", modelLocation.toString(), location.toString(), e.what());
                }
            }
        }
    }
}

std::shared_ptr<ModelBlock> ModelBakery::loadModel(const ResourceLocation& location) const
{
    const std::string& s = location.getResourcePath();
    if (s == "builtin/generated")
        return modelGenerated();
    if (s == "builtin/compass")
        return modelCompass();
    if (s == "builtin/clock")
        return modelClock();
    if (s == "builtin/entity")
        return modelEntity();

    std::shared_ptr<ModelBlock> modelBlock;
    if (s.starts_with("builtin/")) {
        if (s.substr(8) != "missing")
            throw ResourceNotFoundException(location.toString());
        modelBlock = ModelBlock::deserialize(kBuiltInMissing);
    } else {
        const std::shared_ptr<IResource> resource
            = resourceManager.getResource(ResourceLocation(location.getResourceDomain(), "models/" + location.getResourcePath() + ".json"));
        const std::vector<uint8_t>& data = resource->getInputStream();
        modelBlock = ModelBlock::deserialize(std::string_view(reinterpret_cast<const char*>(data.data()), data.size()));
    }
    modelBlock->name = location.toString();
    return modelBlock;
}

void ModelBakery::loadItemModels()
{
    registerVariantNames();
    for (const Item* item : Item::itemRegistry.values()) {
        for (const std::string& s : getVariantNames(item)) {
            const ResourceLocation name(s);
            const ResourceLocation location(name.getResourceDomain(), "item/" + name.getResourcePath());
            const auto existing = std::find_if(itemLocations.begin(), itemLocations.end(), [&](const auto& entry) { return entry.first == s; });
            if (existing == itemLocations.end())
                itemLocations.emplace_back(s, location);
            else
                existing->second = location;
            if (!model(location)) {
                try {
                    putModel(location, loadModel(location));
                } catch (const std::exception& e) {
                    const auto itemName = Item::getNameForItem(item);
                    Log::warn("Unable to load item model: '{}' for item: '{}' ({})", location.toString(), itemName ? itemName->toString() : "null", e.what());
                }
            }
        }
    }
}

void ModelBakery::registerVariantNames()
{
    const auto put = [&](const Item* item, std::vector<std::string> names) { variantNames[item] = std::move(names); };
    put(itemOfBlock("stone"), { "stone", "granite", "granite_smooth", "diorite", "diorite_smooth", "andesite", "andesite_smooth" });
    put(itemOfBlock("dirt"), { "dirt", "coarse_dirt", "podzol" });
    put(itemOfBlock("planks"), { "oak_planks", "spruce_planks", "birch_planks", "jungle_planks", "acacia_planks", "dark_oak_planks" });
    put(itemOfBlock("sapling"), { "oak_sapling", "spruce_sapling", "birch_sapling", "jungle_sapling", "acacia_sapling", "dark_oak_sapling" });
    put(itemOfBlock("sand"), { "sand", "red_sand" });
    put(itemOfBlock("log"), { "oak_log", "spruce_log", "birch_log", "jungle_log" });
    put(itemOfBlock("leaves"), { "oak_leaves", "spruce_leaves", "birch_leaves", "jungle_leaves" });
    put(itemOfBlock("sponge"), { "sponge", "sponge_wet" });
    put(itemOfBlock("sandstone"), { "sandstone", "chiseled_sandstone", "smooth_sandstone" });
    put(itemOfBlock("red_sandstone"), { "red_sandstone", "chiseled_red_sandstone", "smooth_red_sandstone" });
    put(itemOfBlock("tallgrass"), { "dead_bush", "tall_grass", "fern" });
    put(itemOfBlock("deadbush"), { "dead_bush" });
    put(itemOfBlock("wool"), { "black_wool", "red_wool", "green_wool", "brown_wool", "blue_wool", "purple_wool", "cyan_wool", "silver_wool", "gray_wool",
                                 "pink_wool", "lime_wool", "yellow_wool", "light_blue_wool", "magenta_wool", "orange_wool", "white_wool" });
    put(itemOfBlock("yellow_flower"), { "dandelion" });
    put(itemOfBlock("red_flower"), { "poppy", "blue_orchid", "allium", "houstonia", "red_tulip", "orange_tulip", "white_tulip", "pink_tulip", "oxeye_daisy" });
    put(itemOfBlock("stone_slab"), { "stone_slab", "sandstone_slab", "cobblestone_slab", "brick_slab", "stone_brick_slab", "nether_brick_slab", "quartz_slab" });
    put(itemOfBlock("stone_slab2"), { "red_sandstone_slab" });
    put(itemOfBlock("stained_glass"),
        { "black_stained_glass", "red_stained_glass", "green_stained_glass", "brown_stained_glass", "blue_stained_glass", "purple_stained_glass",
            "cyan_stained_glass", "silver_stained_glass", "gray_stained_glass", "pink_stained_glass", "lime_stained_glass", "yellow_stained_glass",
            "light_blue_stained_glass", "magenta_stained_glass", "orange_stained_glass", "white_stained_glass" });
    put(itemOfBlock("monster_egg"), { "stone_monster_egg", "cobblestone_monster_egg", "stone_brick_monster_egg", "mossy_brick_monster_egg",
                                        "cracked_brick_monster_egg", "chiseled_brick_monster_egg" });
    put(itemOfBlock("stonebrick"), { "stonebrick", "mossy_stonebrick", "cracked_stonebrick", "chiseled_stonebrick" });
    put(itemOfBlock("wooden_slab"), { "oak_slab", "spruce_slab", "birch_slab", "jungle_slab", "acacia_slab", "dark_oak_slab" });
    put(itemOfBlock("cobblestone_wall"), { "cobblestone_wall", "mossy_cobblestone_wall" });
    put(itemOfBlock("anvil"), { "anvil_intact", "anvil_slightly_damaged", "anvil_very_damaged" });
    put(itemOfBlock("quartz_block"), { "quartz_block", "chiseled_quartz_block", "quartz_column" });
    put(itemOfBlock("stained_hardened_clay"),
        { "black_stained_hardened_clay", "red_stained_hardened_clay", "green_stained_hardened_clay", "brown_stained_hardened_clay",
            "blue_stained_hardened_clay", "purple_stained_hardened_clay", "cyan_stained_hardened_clay", "silver_stained_hardened_clay",
            "gray_stained_hardened_clay", "pink_stained_hardened_clay", "lime_stained_hardened_clay", "yellow_stained_hardened_clay",
            "light_blue_stained_hardened_clay", "magenta_stained_hardened_clay", "orange_stained_hardened_clay", "white_stained_hardened_clay" });
    put(itemOfBlock("stained_glass_pane"),
        { "black_stained_glass_pane", "red_stained_glass_pane", "green_stained_glass_pane", "brown_stained_glass_pane", "blue_stained_glass_pane",
            "purple_stained_glass_pane", "cyan_stained_glass_pane", "silver_stained_glass_pane", "gray_stained_glass_pane", "pink_stained_glass_pane",
            "lime_stained_glass_pane", "yellow_stained_glass_pane", "light_blue_stained_glass_pane", "magenta_stained_glass_pane",
            "orange_stained_glass_pane", "white_stained_glass_pane" });
    put(itemOfBlock("leaves2"), { "acacia_leaves", "dark_oak_leaves" });
    put(itemOfBlock("log2"), { "acacia_log", "dark_oak_log" });
    put(itemOfBlock("prismarine"), { "prismarine", "prismarine_bricks", "dark_prismarine" });
    put(itemOfBlock("carpet"), { "black_carpet", "red_carpet", "green_carpet", "brown_carpet", "blue_carpet", "purple_carpet", "cyan_carpet",
                                   "silver_carpet", "gray_carpet", "pink_carpet", "lime_carpet", "yellow_carpet", "light_blue_carpet", "magenta_carpet",
                                   "orange_carpet", "white_carpet" });
    put(itemOfBlock("double_plant"), { "sunflower", "syringa", "double_grass", "double_fern", "double_rose", "paeonia" });
    put(itemByName("bow"), { "bow", "bow_pulling_0", "bow_pulling_1", "bow_pulling_2" });
    put(itemByName("coal"), { "coal", "charcoal" });
    put(itemByName("fishing_rod"), { "fishing_rod", "fishing_rod_cast" });
    put(itemByName("fish"), { "cod", "salmon", "clownfish", "pufferfish" });
    put(itemByName("cooked_fish"), { "cooked_cod", "cooked_salmon" });
    put(itemByName("dye"), { "dye_black", "dye_red", "dye_green", "dye_brown", "dye_blue", "dye_purple", "dye_cyan", "dye_silver", "dye_gray", "dye_pink",
                               "dye_lime", "dye_yellow", "dye_light_blue", "dye_magenta", "dye_orange", "dye_white" });
    put(itemByName("potion"), { "bottle_drinkable", "bottle_splash" });
    put(itemByName("skull"), { "skull_skeleton", "skull_wither", "skull_zombie", "skull_char", "skull_creeper" });
    put(itemOfBlock("fence_gate"), { "oak_fence_gate" });
    put(itemOfBlock("fence"), { "oak_fence" });
    put(itemByName("wooden_door"), { "oak_door" });
}

std::vector<std::string> ModelBakery::getVariantNames(const Item* item) const
{
    if (const auto it = variantNames.find(item); it != variantNames.end())
        return it->second;
    const auto name = Item::getNameForItem(item);
    return { name ? name->toString() : "null" };
}

void ModelBakery::bakeBlockModels()
{
    for (const ModelResourceLocation& location : variantOrder) {
        WeightedBakedModel::Builder builder;
        int i = 0;
        for (const ModelBlockDefinition::Variant& variant : variants.at(location)->getVariants()) {
            const ModelBlock* modelBlock = model(variant.getModelLocation());
            if (modelBlock && modelBlock->isResolved()) {
                ++i;
                builder.add(bakeModel(*modelBlock, variant.getRotation(), variant.isUvLocked()), variant.getWeight());
            } else {
                Log::warn("Missing model for: {}", location.toString());
            }
        }
        if (i == 0)
            Log::warn("No weighted models for: {}", location.toString());
        else if (i == 1)
            bakedRegistry[location] = builder.first();
        else
            bakedRegistry[location] = builder.build();
    }

    for (const auto& [name, location] : itemLocations) {
        const ModelResourceLocation itemLocation(name, "inventory");
        const ModelBlock* modelBlock = model(location);
        if (modelBlock && modelBlock->isResolved()) {
            if (isCustomRenderer(modelBlock))
                bakedRegistry[itemLocation] = std::make_shared<BuiltInModel>(modelBlock->getAllTransforms());
            else
                bakedRegistry[itemLocation] = bakeModel(*modelBlock, ModelRotation::X0_Y0, false);
        } else {
            Log::warn("Missing model for: {}", location.toString());
        }
    }
}

std::vector<ResourceLocation> ModelBakery::getVariantsTextureLocations() const
{
    std::vector<ModelResourceLocation> list = variantOrder;
    std::stable_sort(list.begin(), list.end(), [](const ModelResourceLocation& a, const ModelResourceLocation& b) {
        return java::compareTo(a.toString(), b.toString()) < 0;
    });
    std::vector<ResourceLocation> set;
    for (const ModelResourceLocation& location : list) {
        for (const ModelBlockDefinition::Variant& variant : variants.at(location)->getVariants()) {
            if (const ModelBlock* modelBlock = model(variant.getModelLocation()))
                getTextureLocations(*modelBlock, set);
            else
                Log::warn("Missing model for: {}", location.toString());
        }
    }
    set.insert(set.end(), builtinTextures().begin(), builtinTextures().end());
    return set;
}

std::shared_ptr<IBakedModel> ModelBakery::bakeModel(const ModelBlock& modelBlock, const ModelRotation& rotation, bool uvLocked) const
{
    SimpleBakedModel::Builder builder(modelBlock);
    builder.setTexture(sprite(modelBlock.resolveTextureName("particle")));
    for (const BlockPart& part : modelBlock.getElements()) {
        for (const auto& [facing, face] : part.mapFaces) {
            TextureAtlasSprite* faceSprite = sprite(modelBlock.resolveTextureName(face.texture));
            if (!faceSprite)
                faceSprite = textureMap.getMissingSprite(); // Java: NullPointerException
            BakedQuad quad = faceBakery.makeBakedQuad(part.positionFrom, part.positionTo, face, *faceSprite, facing, rotation, part.partRotation, uvLocked,
                part.shade);
            if (!face.cullFace)
                builder.addGeneralQuad(std::move(quad));
            else
                builder.addFaceQuad(rotation.rotateFace(*face.cullFace), std::move(quad));
        }
    }
    return builder.makeBakedModel();
}

void ModelBakery::loadModelsCheck()
{
    loadModels();
    for (const ResourceLocation& location : modelOrder) {
        if (ModelBlock* modelBlock = model(location))
            modelBlock->getParentFromMap(models);
    }
    std::vector<const ModelBlock*> all;
    for (const ResourceLocation& location : modelOrder)
        all.push_back(model(location));
    checkModelHierarchy(all);
}

void ModelBakery::loadModels()
{
    std::deque<ResourceLocation> deque;
    std::unordered_set<ResourceLocation> set;
    for (const ResourceLocation& location : modelOrder) {
        set.insert(location);
        if (const ModelBlock* modelBlock = model(location); modelBlock && modelBlock->getParentLocation())
            deque.push_back(*modelBlock->getParentLocation());
    }
    while (!deque.empty()) {
        const ResourceLocation location = deque.front();
        deque.pop_front();
        if (model(location))
            continue;
        try {
            std::shared_ptr<ModelBlock> modelBlock = loadModel(location);
            const std::optional<ResourceLocation> parent = modelBlock->getParentLocation();
            putModel(location, std::move(modelBlock));
            if (parent && !set.contains(*parent))
                deque.push_back(*parent);
        } catch (const std::exception& e) {
            std::string path;
            for (const ResourceLocation& step : getParentPath(location))
                path += (path.empty() ? "" : " -> ") + step.toString();
            Log::warn("In parent chain: {}; unable to load model: '{}' ({})", path, location.toString(), e.what());
        }
        set.insert(location);
    }
}

std::vector<ResourceLocation> ModelBakery::getParentPath(const ResourceLocation& location) const
{
    std::vector<ResourceLocation> list { location };
    std::optional<ResourceLocation> current = location;
    while (true) {
        std::optional<ResourceLocation> next;
        for (const ResourceLocation& key : modelOrder) {
            const ModelBlock* modelBlock = model(key);
            if (modelBlock && modelBlock->getParentLocation() == *current) {
                next = key;
                break;
            }
        }
        if (!next || std::find(list.begin(), list.end(), *next) != list.end())
            break; // Java loops forever on a cycle; the cycle is reported by checkModelHierarchy instead
        list.insert(list.begin(), *next);
        current = next;
    }
    return list;
}

void ModelBakery::getTextureLocations(const ModelBlock& modelBlock, std::vector<ResourceLocation>& out) const
{
    for (const BlockPart& part : modelBlock.getElements()) {
        for (const auto& [facing, face] : part.mapFaces)
            out.emplace_back(modelBlock.resolveTextureName(face.texture));
    }
    out.emplace_back(modelBlock.resolveTextureName("particle"));
}

void ModelBakery::loadSprites()
{
    std::vector<ResourceLocation> list = getVariantsTextureLocations();
    const std::vector<ResourceLocation> items = getItemsTextureLocations();
    list.insert(list.end(), items.begin(), items.end());
    std::unordered_set<ResourceLocation> seen;
    std::vector<ResourceLocation> set;
    for (const ResourceLocation& location : list) {
        if (!(location == TextureMap::LOCATION_MISSING_TEXTURE) && seen.insert(location).second)
            set.push_back(location);
    }
    textureMap.loadSprites(resourceManager, [&](TextureMap& iconRegistry) {
        for (const ResourceLocation& location : set)
            sprites[location] = iconRegistry.registerSprite(location);
    });
    // Not owned here: the atlas keeps its missing sprite.
    sprites[ResourceLocation("missingno")] = std::shared_ptr<TextureAtlasSprite>(std::shared_ptr<TextureAtlasSprite>(), textureMap.getMissingSprite());
}

std::vector<ResourceLocation> ModelBakery::getItemsTextureLocations() const
{
    std::vector<ResourceLocation> set;
    for (const auto& [name, location] : itemLocations) {
        const ModelBlock* modelBlock = model(location);
        if (!modelBlock)
            continue;
        set.emplace_back(modelBlock->resolveTextureName("particle"));
        if (hasItemModel(modelBlock)) {
            for (const std::string& layer : ItemModelGenerator::LAYERS) {
                const ResourceLocation texture(modelBlock->resolveTextureName(layer));
                if (modelBlock->getRootModel() == modelCompass().get() && !(texture == TextureMap::LOCATION_MISSING_TEXTURE))
                    TextureAtlasSprite::setLocationNameCompass(texture.toString());
                else if (modelBlock->getRootModel() == modelClock().get() && !(texture == TextureMap::LOCATION_MISSING_TEXTURE))
                    TextureAtlasSprite::setLocationNameClock(texture.toString());
                set.push_back(texture);
            }
        } else if (!isCustomRenderer(modelBlock)) {
            for (const BlockPart& part : modelBlock->getElements()) {
                for (const auto& [facing, face] : part.mapFaces)
                    set.emplace_back(modelBlock->resolveTextureName(face.texture));
            }
        }
    }
    return set;
}

bool ModelBakery::hasItemModel(const ModelBlock* modelBlock) const
{
    if (!modelBlock)
        return false;
    const ModelBlock* root = modelBlock->getRootModel();
    return root == modelGenerated().get() || root == modelCompass().get() || root == modelClock().get();
}

bool ModelBakery::isCustomRenderer(const ModelBlock* modelBlock) const
{
    return modelBlock && modelBlock->getRootModel() == modelEntity().get();
}

void ModelBakery::bakeItemModels()
{
    for (const auto& [name, location] : itemLocations) {
        const std::shared_ptr<ModelBlock> modelBlock = [&]() -> std::shared_ptr<ModelBlock> {
            const auto it = models.find(location);
            return it == models.end() ? nullptr : it->second;
        }();
        if (hasItemModel(modelBlock.get())) {
            std::shared_ptr<ModelBlock> generated = itemModelGenerator.makeItemModel(textureMap, *modelBlock);
            if (generated)
                generated->name = location.toString();
            putModel(location, std::move(generated));
        } else if (isCustomRenderer(modelBlock.get())) {
            putModel(location, modelBlock);
        }
    }
    for (const auto& [location, atlasSprite] : sprites) {
        if (!atlasSprite->hasAnimationMetadata())
            atlasSprite->clearFramesTextureData();
    }
}

} // namespace mc

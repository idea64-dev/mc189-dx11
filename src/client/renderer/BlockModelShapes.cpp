#include "client/renderer/BlockModelShapes.h"

#include "block/Block.h"
#include "block/state/BlockState.h"
#include "client/renderer/texture/TextureMap.h"
#include "client/resources/model/IBakedModel.h"
#include "client/resources/model/ModelManager.h"

namespace mc {

namespace {

const Block* block(std::string_view name)
{
    return Block::getBlockFromName(name);
}

std::string valueName(const IBlockState* state, const std::string& property)
{
    return StatePropertyMap(state).remove(property);
}

} // namespace

BlockModelShapes::BlockModelShapes(ModelManager& manager)
    : modelManager(manager)
{
    registerAllBlocks();
}

TextureAtlasSprite* BlockModelShapes::getTexture(const IBlockState* state) const
{
    const Block* b = state->getBlock();
    IBakedModel* model = getModelForState(state);
    if (!model || model == modelManager.getMissingModel()) {
        const TextureMap& textureMap = modelManager.getTextureMap();
        if (b == block("wall_sign") || b == block("standing_sign") || b == block("chest") || b == block("trapped_chest") || b == block("standing_banner")
            || b == block("wall_banner"))
            return textureMap.getAtlasSprite("minecraft:blocks/planks_oak");
        if (b == block("ender_chest"))
            return textureMap.getAtlasSprite("minecraft:blocks/obsidian");
        if (b == block("flowing_lava") || b == block("lava"))
            return textureMap.getAtlasSprite("minecraft:blocks/lava_still");
        if (b == block("flowing_water") || b == block("water"))
            return textureMap.getAtlasSprite("minecraft:blocks/water_still");
        if (b == block("skull"))
            return textureMap.getAtlasSprite("minecraft:blocks/soul_sand");
        if (b == block("barrier"))
            return textureMap.getAtlasSprite("minecraft:items/barrier");
    }
    if (!model)
        model = modelManager.getMissingModel();
    return model->getParticleTexture();
}

IBakedModel* BlockModelShapes::getModelForState(const IBlockState* state) const
{
    const auto it = bakedModelStore.find(state);
    return it == bakedModelStore.end() || !it->second ? modelManager.getMissingModel() : it->second;
}

void BlockModelShapes::reloadModels()
{
    bakedModelStore.clear();
    for (const auto& [state, location] : blockStateMapper.putAllStateModelLocations())
        bakedModelStore[state] = modelManager.getModel(location);
}

void BlockModelShapes::registerBlockWithStateMapper(const Block* b, std::unique_ptr<IStateMapper> stateMapper)
{
    blockStateMapper.registerBlockStateMapper(b, std::move(stateMapper));
}

void BlockModelShapes::registerBuiltInBlocks(std::initializer_list<const Block*> builtIns)
{
    blockStateMapper.registerBuiltInBlocks(builtIns);
}

void BlockModelShapes::registerAllBlocks()
{
    using Builder = StateMap::Builder;
    registerBuiltInBlocks({ block("air"), block("flowing_water"), block("water"), block("flowing_lava"), block("lava"), block("piston_extension"),
        block("chest"), block("ender_chest"), block("trapped_chest"), block("standing_sign"), block("skull"), block("end_portal"), block("barrier"),
        block("wall_sign"), block("wall_banner"), block("standing_banner") });
    registerBlockWithStateMapper(block("stone"), Builder().withName("variant").build());
    registerBlockWithStateMapper(block("prismarine"), Builder().withName("variant").build());
    registerBlockWithStateMapper(block("leaves"), Builder().withName("variant").withSuffix("_leaves").ignore({ "check_decay", "decayable" }).build());
    registerBlockWithStateMapper(block("leaves2"), Builder().withName("variant").withSuffix("_leaves").ignore({ "check_decay", "decayable" }).build());
    registerBlockWithStateMapper(block("cactus"), Builder().ignore({ "age" }).build());
    registerBlockWithStateMapper(block("reeds"), Builder().ignore({ "age" }).build());
    registerBlockWithStateMapper(block("jukebox"), Builder().ignore({ "has_record" }).build());
    registerBlockWithStateMapper(block("command_block"), Builder().ignore({ "triggered" }).build());
    registerBlockWithStateMapper(block("cobblestone_wall"), Builder().withName("variant").withSuffix("_wall").build());
    registerBlockWithStateMapper(block("double_plant"), Builder().withName("variant").ignore({ "facing" }).build());
    for (const char* gate : { "fence_gate", "spruce_fence_gate", "birch_fence_gate", "jungle_fence_gate", "dark_oak_fence_gate", "acacia_fence_gate" })
        registerBlockWithStateMapper(block(gate), Builder().ignore({ "powered" }).build());
    registerBlockWithStateMapper(block("tripwire"), Builder().ignore({ "disarmed", "powered" }).build());
    registerBlockWithStateMapper(block("double_wooden_slab"), Builder().withName("variant").withSuffix("_double_slab").build());
    registerBlockWithStateMapper(block("wooden_slab"), Builder().withName("variant").withSuffix("_slab").build());
    registerBlockWithStateMapper(block("tnt"), Builder().ignore({ "explode" }).build());
    registerBlockWithStateMapper(block("fire"), Builder().ignore({ "age" }).build());
    registerBlockWithStateMapper(block("redstone_wire"), Builder().ignore({ "power" }).build());
    for (const char* door : { "wooden_door", "spruce_door", "birch_door", "jungle_door", "acacia_door", "dark_oak_door", "iron_door" })
        registerBlockWithStateMapper(block(door), Builder().ignore({ "powered" }).build());
    registerBlockWithStateMapper(block("wool"), Builder().withName("color").withSuffix("_wool").build());
    registerBlockWithStateMapper(block("carpet"), Builder().withName("color").withSuffix("_carpet").build());
    registerBlockWithStateMapper(block("stained_hardened_clay"), Builder().withName("color").withSuffix("_stained_hardened_clay").build());
    registerBlockWithStateMapper(block("stained_glass_pane"), Builder().withName("color").withSuffix("_stained_glass_pane").build());
    registerBlockWithStateMapper(block("stained_glass"), Builder().withName("color").withSuffix("_stained_glass").build());
    registerBlockWithStateMapper(block("sandstone"), Builder().withName("type").build());
    registerBlockWithStateMapper(block("red_sandstone"), Builder().withName("type").build());
    registerBlockWithStateMapper(block("tallgrass"), Builder().withName("type").build());
    registerBlockWithStateMapper(block("bed"), Builder().ignore({ "occupied" }).build());
    registerBlockWithStateMapper(block("yellow_flower"), Builder().withName("type").build());
    registerBlockWithStateMapper(block("red_flower"), Builder().withName("type").build());
    registerBlockWithStateMapper(block("stone_slab"), Builder().withName("variant").withSuffix("_slab").build());
    registerBlockWithStateMapper(block("stone_slab2"), Builder().withName("variant").withSuffix("_slab").build());
    registerBlockWithStateMapper(block("monster_egg"), Builder().withName("variant").withSuffix("_monster_egg").build());
    registerBlockWithStateMapper(block("stonebrick"), Builder().withName("variant").build());
    registerBlockWithStateMapper(block("dispenser"), Builder().ignore({ "triggered" }).build());
    registerBlockWithStateMapper(block("dropper"), Builder().ignore({ "triggered" }).build());
    registerBlockWithStateMapper(block("log"), Builder().withName("variant").withSuffix("_log").build());
    registerBlockWithStateMapper(block("log2"), Builder().withName("variant").withSuffix("_log").build());
    registerBlockWithStateMapper(block("planks"), Builder().withName("variant").withSuffix("_planks").build());
    registerBlockWithStateMapper(block("sapling"), Builder().withName("type").withSuffix("_sapling").build());
    registerBlockWithStateMapper(block("sand"), Builder().withName("variant").build());
    registerBlockWithStateMapper(block("hopper"), Builder().ignore({ "enabled" }).build());
    registerBlockWithStateMapper(block("flower_pot"), Builder().ignore({ "legacy_data" }).build());

    registerBlockWithStateMapper(block("quartz_block"), std::make_unique<LambdaStateMapper>([](const IBlockState* state) {
        const std::string type = valueName(state, "variant");
        if (type == "chiseled")
            return ModelResourceLocation("chiseled_quartz_block", "normal");
        if (type == "lines_y")
            return ModelResourceLocation("quartz_column", "axis=y");
        if (type == "lines_x")
            return ModelResourceLocation("quartz_column", "axis=x");
        if (type == "lines_z")
            return ModelResourceLocation("quartz_column", "axis=z");
        return ModelResourceLocation("quartz_block", "normal");
    }));
    registerBlockWithStateMapper(block("deadbush"),
        std::make_unique<LambdaStateMapper>([](const IBlockState*) { return ModelResourceLocation("dead_bush", "normal"); }));
    const auto stem = [](const IBlockState* state) {
        StatePropertyMap map(state);
        if (valueName(state, "facing") != "up")
            map.remove("age");
        return ModelResourceLocation(Block::getNameForBlock(state->getBlock()), map.toPropertyString());
    };
    registerBlockWithStateMapper(block("pumpkin_stem"), std::make_unique<LambdaStateMapper>(stem));
    registerBlockWithStateMapper(block("melon_stem"), std::make_unique<LambdaStateMapper>(stem));
    registerBlockWithStateMapper(block("dirt"), std::make_unique<LambdaStateMapper>([](const IBlockState* state) {
        StatePropertyMap map(state);
        const std::string s = map.remove("variant");
        if (s != "podzol")
            map.remove("snowy");
        return ModelResourceLocation(s, map.toPropertyString());
    }));
    const auto doubleSlab = [](const IBlockState* state) {
        const std::string s = valueName(state, "variant");
        return ModelResourceLocation(s + "_double_slab", valueName(state, "seamless") == "true" ? "all" : "normal");
    };
    registerBlockWithStateMapper(block("double_stone_slab"), std::make_unique<LambdaStateMapper>(doubleSlab));
    registerBlockWithStateMapper(block("double_stone_slab2"), std::make_unique<LambdaStateMapper>(doubleSlab));
}

} // namespace mc

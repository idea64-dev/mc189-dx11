// Block and item models and the block atlas against the original: tests/reference/models.txt is written by
// tools/reference/net/minecraft/client/renderer/texture/ModelDump.java, which bakes the vanilla client jar's
// models the way ModelManager does at startup (4 mip levels, the atlas composed in memory instead of uploaded).
// This bakes the installed jar the same way and writes the same text: atlas size and per-level hash, each
// sprite's placement and UVs, each block state's variant and particle sprite, a hash of every baked model's
// quads and RenderItem's item model registrations. Skipped when Minecraft 1.8.9 is not installed.

#include "block/Block.h"
#include "block/state/BlockState.h"
#include "client/main/GameInstallation.h"
#include "client/renderer/BlockModelShapes.h"
#include "client/renderer/ItemModelMesher.h"
#include "client/renderer/texture/TextureAtlasSprite.h"
#include "client/renderer/texture/TextureMap.h"
#include "client/resources/DefaultResourcePack.h"
#include "client/resources/ResourceIndex.h"
#include "client/resources/SimpleReloadableResourceManager.h"
#include "client/resources/model/IBakedModel.h"
#include "client/resources/model/ModelManager.h"
#include "core/Java.h"
#include "core/ZipArchive.h"
#include "init/Bootstrap.h"

#include <doctest/doctest.h>

#include <algorithm>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <optional>
#include <string>
#include <vector>

using namespace mc;

namespace {

int32_t arraysHashCode(const int32_t* data, size_t size)
{
    int32_t h = 1;
    for (size_t i = 0; i < size; ++i)
        h = java::add(java::mul(31, h), data[i]);
    return h;
}

int32_t quadHash(const BakedQuad& quad)
{
    const int32_t vertices = arraysHashCode(quad.vertexData.data(), quad.vertexData.size());
    return java::add(java::mul(31, java::add(java::mul(31, vertices), quad.getTintIndex())), ordinal(quad.getFace()));
}

std::string quads(const std::vector<BakedQuad>& list, bool sorted)
{
    std::vector<int32_t> hashes;
    for (const BakedQuad& quad : list)
        hashes.push_back(quadHash(quad));
    if (sorted)
        std::sort(hashes.begin(), hashes.end());
    int32_t h = 1;
    for (const int32_t q : hashes)
        h = java::add(java::mul(31, h), q);
    return std::to_string(list.size()) + ":" + std::to_string(h);
}

std::string describe(const IBakedModel& model, bool sorted)
{
    std::string s = model.isAmbientOcclusion() ? "ao" : "-";
    s += model.isGui3d() ? " gui3d" : " -";
    s += model.isBuiltInRenderer() ? " builtin" : " -";
    s += " particle=" + (model.getParticleTexture() ? model.getParticleTexture()->getIconName() : std::string("null"));
    if (!model.isBuiltInRenderer()) {
        s += " general=" + quads(model.getGeneralQuads(), sorted);
        for (const EnumFacing facing : kFacings)
            s += " " + std::string(getName(facing)) + "=" + quads(model.getFaceQuads(facing), sorted);
    }
    return s;
}

std::string hex(float value)
{
    char buffer[16];
    std::snprintf(buffer, sizeof buffer, "%x", static_cast<uint32_t>(java::floatToRawIntBits(value)));
    return buffer;
}

std::vector<std::string> referenceLines()
{
    std::ifstream file(std::filesystem::path(__FILE__).parent_path() / "reference" / "models.txt");
    REQUIRE(file);
    std::vector<std::string> lines;
    std::string line;
    while (std::getline(file, line)) {
        if (!line.empty() && line.back() == '\r')
            line.pop_back();
        lines.push_back(line);
    }
    return lines;
}

} // namespace

TEST_CASE("Block and item models bake like the original")
{
    const std::optional<GameInstallation> game = findGameInstallation(".", {}, {}, "1.8.9", "1.8");
    if (!game) {
        MESSAGE("Minecraft 1.8.9 not installed, skipping");
        return;
    }
    Bootstrap::registerAll();
    SimpleReloadableResourceManager resourceManager;
    resourceManager.reloadResources({ std::make_shared<DefaultResourcePack>(std::make_shared<const ZipArchive>(game->clientJar),
        ResourceIndex(game->assetsDir, "1.8").getResourceMap()) });

    TextureMap textureMap("textures");
    std::vector<std::vector<uint32_t>> atlas;
    textureMap.setCpuAtlas(&atlas);
    textureMap.setMipmapLevels(4);
    ModelManager modelManager(textureMap);
    modelManager.onResourceManagerReload(resourceManager);

    std::vector<std::string> actual;
    actual.push_back("atlas " + std::to_string(textureMap.getAtlasWidth()) + "x" + std::to_string(textureMap.getAtlasHeight()) + " mipmaps "
        + std::to_string(atlas.size() - 1));
    for (size_t level = 0; level < atlas.size(); ++level) {
        actual.push_back("level " + std::to_string(level) + " "
            + std::to_string(arraysHashCode(reinterpret_cast<const int32_t*>(atlas[level].data()), atlas[level].size())));
    }
    for (const TextureAtlasSprite* sprite : textureMap.getUploadedSprites()) {
        std::string line = "sprite " + sprite->getIconName() + " " + std::to_string(sprite->getOriginX()) + " " + std::to_string(sprite->getOriginY()) + " "
            + std::to_string(sprite->getIconWidth()) + "x" + std::to_string(sprite->getIconHeight());
        if (sprite->isRotated())
            line += " rotated";
        if (sprite->hasAnimationMetadata())
            line += " animated " + std::to_string(sprite->getFrameCount());
        line += " uv " + hex(sprite->getMinU()) + " " + hex(sprite->getMaxU()) + " " + hex(sprite->getMinV()) + " " + hex(sprite->getMaxV());
        actual.push_back(line);
    }

    BlockModelShapes& shapes = modelManager.getBlockModelShapes();
    std::unordered_map<const IBlockState*, std::string> locations;
    for (const auto& [state, location] : shapes.getBlockStateMapper().putAllStateModelLocations())
        locations[state] = location.toString();
    for (const Block* block : Block::blockRegistry.values()) {
        for (const IBlockState* state : block->getBlockState().getValidStates()) {
            const auto it = locations.find(state);
            actual.push_back("state " + state->toString() + " " + (it == locations.end() ? std::string("builtin") : it->second)
                + (shapes.getModelForState(state) == modelManager.getMissingModel() ? " missing" : "") + " particle=" + shapes.getTexture(state)->getIconName());
        }
    }

    std::vector<std::pair<std::string, const IBakedModel*>> models;
    for (const auto& [location, model] : modelManager.getModelRegistry())
        models.emplace_back(location.toString(), model.get());
    std::sort(models.begin(), models.end(), [](const auto& a, const auto& b) { return java::compareTo(a.first, b.first) < 0; });
    for (const auto& [key, model] : models) {
        const bool item = key.ends_with("#inventory");
        if (const auto* weighted = dynamic_cast<const WeightedBakedModel*>(model)) {
            actual.push_back("model " + key + " weighted " + std::to_string(weighted->getModels().size()));
            for (const WeightedBakedModel::Item& entry : weighted->getModels())
                actual.push_back("  weight " + std::to_string(entry.itemWeight) + " " + describe(*entry.model, item));
        } else {
            actual.push_back("model " + key + " " + describe(*model, item));
        }
    }

    for (const ItemModelMesher::Registration& registration : ItemModelMesher::vanillaRegistrations()) {
        actual.push_back("register " + std::string(registration.item) + " " + std::to_string(registration.meta) + " "
            + ModelResourceLocation(registration.model, "inventory").toString());
    }

    const std::vector<std::string> expected = referenceLines();
    CHECK(actual.size() == expected.size());
    int mismatches = 0;
    for (size_t i = 0; i < std::min(expected.size(), actual.size()) && mismatches < 20; ++i) {
        if (actual[i] != expected[i]) {
            CAPTURE(i);
            CHECK(actual[i] == expected[i]);
            ++mismatches;
        }
    }
}

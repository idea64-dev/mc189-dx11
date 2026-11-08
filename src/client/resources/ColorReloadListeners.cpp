#include "client/resources/ColorReloadListeners.h"

#include "client/renderer/texture/TextureUtil.h"
#include "util/ResourceLocation.h"
#include "world/Colorizer.h"

namespace mc {

namespace {

std::vector<int> toInts(const std::vector<uint32_t>& pixels)
{
    return std::vector<int>(pixels.begin(), pixels.end());
}

} // namespace

void GrassColorReloadListener::onResourceManagerReload(const IResourceManager& resourceManager)
{
    static const ResourceLocation LOC_GRASS_PNG("textures/colormap/grass.png");
    try {
        ColorizerGrass::setGrassBiomeColorizer(toInts(TextureUtil::readImageData(resourceManager, LOC_GRASS_PNG)));
    } catch (const std::exception&) {
        // Java: the IOException is ignored; the old colormap stays.
    }
}

void FoliageColorReloadListener::onResourceManagerReload(const IResourceManager& resourceManager)
{
    static const ResourceLocation LOC_FOLIAGE_PNG("textures/colormap/foliage.png");
    try {
        ColorizerFoliage::setFoliageBiomeColorizer(toInts(TextureUtil::readImageData(resourceManager, LOC_FOLIAGE_PNG)));
    } catch (const std::exception&) {
    }
}

} // namespace mc

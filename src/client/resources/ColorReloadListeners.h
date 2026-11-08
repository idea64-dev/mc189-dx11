#pragma once

// Original: net/minecraft/client/resources/GrassColorReloadListener.java and FoliageColorReloadListener.java --
// reload the biome colormaps (textures/colormap/grass.png, foliage.png) into ColorizerGrass / ColorizerFoliage.

#include "client/resources/IResourceManager.h"

namespace mc {

class GrassColorReloadListener : public IResourceManagerReloadListener {
public:
    void onResourceManagerReload(const IResourceManager& resourceManager) override;
};

class FoliageColorReloadListener : public IResourceManagerReloadListener {
public:
    void onResourceManagerReload(const IResourceManager& resourceManager) override;
};

} // namespace mc

#pragma once

// Original: net/minecraft/client/renderer/texture/TextureManager.java
// Tickable textures (animated atlas, clock, compass) are registered here once they exist (stage 7).

#include "client/resources/IResourceManager.h"
#include "client/renderer/texture/ITextureObject.h"
#include "util/ResourceLocation.h"

#include <memory>
#include <string>
#include <unordered_map>

namespace mc {

class DynamicTexture;

class TextureManager : public IResourceManagerReloadListener {
public:
    explicit TextureManager(const IResourceManager& resourceManager);

    void bindTexture(const ResourceLocation& resource);
    bool loadTexture(const ResourceLocation& textureLocation, std::shared_ptr<ITextureObject> textureObj);
    std::shared_ptr<ITextureObject> getTexture(const ResourceLocation& textureLocation) const;
    ResourceLocation getDynamicTextureLocation(const std::string& name, std::shared_ptr<DynamicTexture> texture);
    void deleteTexture(const ResourceLocation& textureLocation);

    void onResourceManagerReload(const IResourceManager& resourceManager) override;

private:
    std::unordered_map<ResourceLocation, std::shared_ptr<ITextureObject>> mapTextureObjects;
    std::unordered_map<std::string, int> mapTextureCounters;
    const IResourceManager* theResourceManager;
};

} // namespace mc

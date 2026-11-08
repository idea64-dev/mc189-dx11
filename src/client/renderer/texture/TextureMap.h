#pragma once

// Original: net/minecraft/client/renderer/texture/TextureMap.java, IIconCreator.java,
// ITickableTextureObject.java and util/ITickable.java
// The block/item atlas: loads every registered sprite (with its .mcmeta), generates mip levels, stitches them
// into one texture and uploads it; animated sprites are updated every tick.

#include "client/renderer/texture/ITextureObject.h"
#include "util/ResourceLocation.h"

#include <cstdint>
#include <functional>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

namespace mc {

class TextureAtlasSprite;
class TextureMap;

// Java: ITickable (ITickableTextureObject is a texture that is also this).
class ITickable {
public:
    virtual ~ITickable() = default;
    virtual void tick() = 0;
};

// Java: IIconCreator.registerSprites
using IIconCreator = std::function<void(TextureMap&)>;

class TextureMap : public AbstractTexture, public ITickable {
public:
    static const ResourceLocation LOCATION_MISSING_TEXTURE;
    static const ResourceLocation locationBlocksTexture;

    explicit TextureMap(std::string basePathIn, IIconCreator iconCreatorIn = {});

    void loadTexture(const IResourceManager& resourceManager) override;

    void loadSprites(const IResourceManager& resourceManager, const IIconCreator& iconCreator);
    // Without a GL context (tests) only the stitching runs; the upload needs one.
    void loadTextureAtlas(const IResourceManager& resourceManager);
    TextureAtlasSprite* getAtlasSprite(const std::string& iconName) const;
    void updateAnimations();
    std::shared_ptr<TextureAtlasSprite> registerSprite(const ResourceLocation& location);
    void tick() override { updateAnimations(); }
    void setMipmapLevels(int mipmapLevelsIn) { mipmapLevels = mipmapLevelsIn; }
    int getMipmapLevels() const { return mipmapLevels; }
    TextureAtlasSprite* getMissingSprite() const { return missingImage.get(); }

    // Port additions for tests and the debug screen.
    int getAtlasWidth() const { return atlasWidth; }
    int getAtlasHeight() const { return atlasHeight; }
    // Uploaded sprites in stitch order.
    const std::vector<TextureAtlasSprite*>& getUploadedSprites() const { return uploadedInOrder; }
    // Tests run without a device: when set, the atlas levels are composed here instead of being uploaded.
    void setCpuAtlas(std::vector<std::vector<uint32_t>>* atlas) { cpuAtlas = atlas; }

private:
    void initMissingImage();
    void copyToCpuAtlas(const TextureAtlasSprite& sprite);
    ResourceLocation completeResourceLocation(const ResourceLocation& location, int mipLevel) const;

    std::vector<TextureAtlasSprite*> listAnimatedSprites;
    std::unordered_map<std::string, std::shared_ptr<TextureAtlasSprite>> mapRegisteredSprites;
    std::unordered_map<std::string, TextureAtlasSprite*> mapUploadedSprites;
    std::vector<TextureAtlasSprite*> uploadedInOrder;
    std::string basePath;
    IIconCreator iconCreator;
    int mipmapLevels = 0;
    std::shared_ptr<TextureAtlasSprite> missingImage;
    int atlasWidth = 0;
    int atlasHeight = 0;
    std::vector<std::vector<uint32_t>>* cpuAtlas = nullptr;
};

} // namespace mc

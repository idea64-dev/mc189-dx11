#pragma once

// Original: net/minecraft/client/renderer/texture/SimpleTexture.java and DynamicTexture.java

#include "client/renderer/texture/BufferedImage.h"
#include "client/renderer/texture/ITextureObject.h"
#include "util/ResourceLocation.h"

#include <cstdint>
#include <vector>

namespace mc {

class SimpleTexture : public AbstractTexture {
public:
    explicit SimpleTexture(ResourceLocation textureResourceLocation);
    void loadTexture(const IResourceManager& resourceManager) override;

protected:
    ResourceLocation textureLocation;
};

class DynamicTexture : public AbstractTexture {
public:
    explicit DynamicTexture(const BufferedImage& bufferedImage);
    DynamicTexture(int textureWidth, int textureHeight);

    void loadTexture(const IResourceManager& /*resourceManager*/) override { }
    void updateDynamicTexture();
    std::vector<uint32_t>& getTextureData() { return dynamicTextureData; }
    int getWidth() const { return width; }
    int getHeight() const { return height; }

private:
    int width;
    int height;
    std::vector<uint32_t> dynamicTextureData;
};

} // namespace mc

#include "client/renderer/texture/SimpleTexture.h"

#include "client/renderer/texture/TextureUtil.h"
#include "client/resources/IResourceManager.h"
#include "core/Log.h"

#include <nlohmann/json.hpp>

namespace mc {

SimpleTexture::SimpleTexture(ResourceLocation textureResourceLocation)
    : textureLocation(std::move(textureResourceLocation))
{
}

void SimpleTexture::loadTexture(const IResourceManager& resourceManager)
{
    deleteGlTexture();

    const std::shared_ptr<IResource> resource = resourceManager.getResource(textureLocation);
    const BufferedImage image = TextureUtil::readBufferedImage(resource->getInputStream());
    bool blurTexture = false;
    bool clampTexture = false;

    if (resource->hasMetadata()) {
        try {
            // TextureMetadataSection: {"texture": {"blur": bool, "clamp": bool}}
            if (const nlohmann::json* section = resource->getMetadata("texture")) {
                blurTexture = section->value("blur", false);
                clampTexture = section->value("clamp", false);
            }
        } catch (const std::exception& e) {
            Log::warn("Failed reading metadata of: {} ({})", textureLocation.toString(), e.what());
        }
    }

    TextureUtil::uploadTextureImageAllocate(getGlTextureId(), image, blurTexture, clampTexture);
}

DynamicTexture::DynamicTexture(const BufferedImage& bufferedImage)
    : DynamicTexture(bufferedImage.width, bufferedImage.height)
{
    dynamicTextureData = bufferedImage.pixels;
    updateDynamicTexture();
}

DynamicTexture::DynamicTexture(int textureWidth, int textureHeight)
    : width(textureWidth)
    , height(textureHeight)
    , dynamicTextureData(static_cast<size_t>(textureWidth) * static_cast<size_t>(textureHeight))
{
    TextureUtil::allocateTexture(getGlTextureId(), textureWidth, textureHeight);
}

void DynamicTexture::updateDynamicTexture()
{
    TextureUtil::uploadTexture(getGlTextureId(), dynamicTextureData, width, height);
}

} // namespace mc

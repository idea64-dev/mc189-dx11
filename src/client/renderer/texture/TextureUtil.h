#pragma once

// Original: net/minecraft/client/renderer/texture/TextureUtil.java

#include "client/renderer/texture/BufferedImage.h"

#include <cstdint>
#include <memory>
#include <span>
#include <vector>

namespace mc {

class DynamicTexture;
class IResourceManager;
class ResourceLocation;

class TextureUtil {
public:
    // Java: the static missingTexture field, created on first use here because it needs the GL context.
    static DynamicTexture& missingTexture();

    static int glGenTextures();
    static void deleteTexture(int textureId);
    static int uploadTextureImage(int textureId, const BufferedImage& image);
    static void uploadTexture(int textureId, std::span<const uint32_t> data, int width, int height);
    static std::vector<std::vector<uint32_t>> generateMipmapData(int levels, int width, const std::vector<std::vector<uint32_t>>& data);
    static void uploadTextureMipmap(const std::vector<std::vector<uint32_t>>& data, int width, int height, int x, int y, bool blur, bool clamp);
    static int uploadTextureImageAllocate(int textureId, const BufferedImage& image, bool blur, bool clamp);
    static void allocateTexture(int textureId, int width, int height);
    static void allocateTextureImpl(int textureId, int mipmapLevels, int width, int height);
    static int uploadTextureImageSub(int textureId, const BufferedImage& image, int x, int y, bool blur, bool clamp);
    static void bindTexture(int textureId);
    static std::vector<uint32_t> readImageData(const IResourceManager& resourceManager, const ResourceLocation& location);

    // Java: ImageIO.read. Throws std::runtime_error on undecodable data.
    // Known difference: ImageIO gamma-shifts greyscale PNGs through a linear grey colour space in getRGB();
    // vanilla 1.8.9 ships none (only RGB, RGBA and palette PNGs), resource packs might. Revisit in stage 3.
    static BufferedImage readBufferedImage(std::span<const uint8_t> data);

    static uint32_t anaglyphColor(uint32_t color);
    static void processPixelValues(std::span<uint32_t> pixels, int width, int height);

private:
    static void uploadTextureSub(int level, std::span<const uint32_t> data, int width, int height, int x, int y, bool blur, bool clamp, bool mipmap);
    static void setTextureClamped(bool clamp);
    static void setTextureBlurred(bool blur);
    static void setTextureBlurMipmap(bool blur, bool mipmap);
    static std::span<const uint32_t> applyAnaglyph(std::span<const uint32_t> data, std::vector<uint32_t>& scratch);
    static uint32_t blendColors(uint32_t c0, uint32_t c1, uint32_t c2, uint32_t c3, bool alphaAware);
    static int blendColorComponent(uint32_t c0, uint32_t c1, uint32_t c2, uint32_t c3, int shift);
};

} // namespace mc

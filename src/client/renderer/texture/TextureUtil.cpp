#include "client/renderer/texture/TextureUtil.h"

#include "client/Minecraft.h"
#include "client/renderer/DxStateManager.h"
#include "client/renderer/texture/SimpleTexture.h"
#include "client/resources/IResourceManager.h"
#include "core/Java.h"
#include "render/gl/GL.h"

#pragma warning(push, 0)
#define STB_IMAGE_IMPLEMENTATION
#define STBI_ONLY_PNG
#define STBI_NO_STDIO
#include <stb/stb_image.h>
#pragma warning(pop)

#include <cmath>
#include <limits>
#include <stdexcept>
#include <string>

namespace mc {

DynamicTexture& TextureUtil::missingTexture()
{
    static DynamicTexture* texture = [] {
        auto* t = new DynamicTexture(16, 16); // lives as long as the GL context; never destroyed, like Java's static
        std::vector<uint32_t>& data = t->getTextureData();
        const uint32_t black = 0xFF000000u;
        const uint32_t magenta = 0xFFF800F8u; // -524040
        for (int row = 0; row < 16; ++row) {
            for (int column = 0; column < 16; ++column)
                data[static_cast<size_t>(row * 16 + column)] = ((row < 8) == (column < 8)) ? magenta : black;
        }
        t->updateDynamicTexture();
        return t;
    }();
    return *texture;
}

int TextureUtil::glGenTextures()
{
    return DxStateManager::generateTexture();
}

void TextureUtil::deleteTexture(int textureId)
{
    DxStateManager::deleteTexture(textureId);
}

int TextureUtil::uploadTextureImage(int textureId, const BufferedImage& image)
{
    return uploadTextureImageAllocate(textureId, image, false, false);
}

void TextureUtil::uploadTexture(int textureId, std::span<const uint32_t> data, int width, int height)
{
    bindTexture(textureId);
    uploadTextureSub(0, data, width, height, 0, 0, false, false, false);
}

std::vector<std::vector<uint32_t>> TextureUtil::generateMipmapData(int levels, int width, const std::vector<std::vector<uint32_t>>& data)
{
    std::vector<std::vector<uint32_t>> result(static_cast<size_t>(levels) + 1);
    result[0] = data[0];

    if (levels > 0) {
        // Java's loop runs to the number of levels, not of pixels: only the first levels + 1 pixels are looked at.
        bool hasTransparency = false;
        for (size_t i = 0; i < data.size(); ++i) {
            if ((static_cast<int32_t>(data[0].at(i)) >> 24) == 0) {
                hasTransparency = true;
                break;
            }
        }

        for (int level = 1; level <= levels; ++level) {
            if (static_cast<size_t>(level) < data.size() && !data[static_cast<size_t>(level)].empty()) {
                result[static_cast<size_t>(level)] = data[static_cast<size_t>(level)];
                continue;
            }
            const std::vector<uint32_t>& previous = result[static_cast<size_t>(level) - 1];
            std::vector<uint32_t> next(previous.size() >> 2);
            const int levelWidth = width >> level;
            const int levelHeight = levelWidth > 0 ? static_cast<int>(next.size()) / levelWidth : 0;
            const int previousWidth = levelWidth << 1;
            for (int x = 0; x < levelWidth; ++x) {
                for (int y = 0; y < levelHeight; ++y) {
                    const int k1 = 2 * (x + y * previousWidth);
                    next[static_cast<size_t>(x + y * levelWidth)] = blendColors(previous[static_cast<size_t>(k1)], previous[static_cast<size_t>(k1 + 1)],
                        previous[static_cast<size_t>(k1 + previousWidth)], previous[static_cast<size_t>(k1 + 1 + previousWidth)], hasTransparency);
                }
            }
            result[static_cast<size_t>(level)] = std::move(next);
        }
    }
    return result;
}

uint32_t TextureUtil::blendColors(uint32_t c0, uint32_t c1, uint32_t c2, uint32_t c3, bool alphaAware)
{
    if (!alphaAware) {
        const int a = blendColorComponent(c0, c1, c2, c3, 24);
        const int r = blendColorComponent(c0, c1, c2, c3, 16);
        const int g = blendColorComponent(c0, c1, c2, c3, 8);
        const int b = blendColorComponent(c0, c1, c2, c3, 0);
        return static_cast<uint32_t>(java::shl(a, 24) | (r << 16) | (g << 8) | b);
    }

    const uint32_t colors[4] = { c0, c1, c2, c3 };
    float fa = 0.0f;
    float fr = 0.0f;
    float fg = 0.0f;
    float fb = 0.0f;
    for (const uint32_t c : colors) {
        if ((static_cast<int32_t>(c) >> 24) != 0) {
            fa += static_cast<float>(std::pow(static_cast<double>(static_cast<float>((c >> 24) & 255) / 255.0f), 2.2));
            fr += static_cast<float>(std::pow(static_cast<double>(static_cast<float>((c >> 16) & 255) / 255.0f), 2.2));
            fg += static_cast<float>(std::pow(static_cast<double>(static_cast<float>((c >> 8) & 255) / 255.0f), 2.2));
            fb += static_cast<float>(std::pow(static_cast<double>(static_cast<float>(c & 255) / 255.0f), 2.2));
        }
    }
    fa /= 4.0f;
    fr /= 4.0f;
    fg /= 4.0f;
    fb /= 4.0f;
    int a = java::d2i(std::pow(static_cast<double>(fa), 0.45454545454545453) * 255.0);
    const int r = java::d2i(std::pow(static_cast<double>(fr), 0.45454545454545453) * 255.0);
    const int g = java::d2i(std::pow(static_cast<double>(fg), 0.45454545454545453) * 255.0);
    const int b = java::d2i(std::pow(static_cast<double>(fb), 0.45454545454545453) * 255.0);
    if (a < 96)
        a = 0;
    return static_cast<uint32_t>(java::shl(a, 24) | (r << 16) | (g << 8) | b);
}

int TextureUtil::blendColorComponent(uint32_t c0, uint32_t c1, uint32_t c2, uint32_t c3, int shift)
{
    const auto channel = [&](uint32_t c) {
        return static_cast<float>(std::pow(static_cast<double>(static_cast<float>((c >> shift) & 255) / 255.0f), 2.2));
    };
    const float f = channel(c0);
    const float f1 = channel(c1);
    const float f2 = channel(c2);
    const float f3 = channel(c3);
    const float f4 = static_cast<float>(std::pow(static_cast<double>(f + f1 + f2 + f3) * 0.25, 0.45454545454545453));
    return java::d2i(static_cast<double>(f4) * 255.0);
}

void TextureUtil::uploadTextureMipmap(const std::vector<std::vector<uint32_t>>& data, int width, int height, int x, int y, bool blur, bool clamp)
{
    for (size_t i = 0; i < data.size(); ++i) {
        const int level = static_cast<int>(i);
        uploadTextureSub(level, data[i], width >> level, height >> level, x >> level, y >> level, blur, clamp, data.size() > 1);
    }
}

std::span<const uint32_t> TextureUtil::applyAnaglyph(std::span<const uint32_t> data, std::vector<uint32_t>& scratch)
{
    const Minecraft* minecraft = Minecraft::getMinecraft();
    if (!minecraft || !minecraft->gameSettings || !minecraft->gameSettings->anaglyph)
        return data;
    scratch.resize(data.size());
    for (size_t i = 0; i < data.size(); ++i)
        scratch[i] = anaglyphColor(data[i]);
    return scratch;
}

void TextureUtil::uploadTextureSub(int level, std::span<const uint32_t> data, int width, int height, int x, int y, bool blur, bool clamp, bool mipmap)
{
    setTextureBlurMipmap(blur, mipmap);
    setTextureClamped(clamp);
    std::vector<uint32_t> scratch;
    const std::span<const uint32_t> pixels = applyAnaglyph(data.first(static_cast<size_t>(width) * static_cast<size_t>(height)), scratch);
    GL::glTexSubImage2D(GL::GL_TEXTURE_2D, level, x, y, width, height, pixels.data());
}

int TextureUtil::uploadTextureImageAllocate(int textureId, const BufferedImage& image, bool blur, bool clamp)
{
    allocateTexture(textureId, image.width, image.height);
    return uploadTextureImageSub(textureId, image, 0, 0, blur, clamp);
}

void TextureUtil::allocateTexture(int textureId, int width, int height)
{
    allocateTextureImpl(textureId, 0, width, height);
}

void TextureUtil::allocateTextureImpl(int textureId, int mipmapLevels, int width, int height)
{
    deleteTexture(textureId);
    bindTexture(textureId);
    if (mipmapLevels >= 0) {
        GL::glTexParameteri(GL::GL_TEXTURE_2D, GL::GL_TEXTURE_MAX_LEVEL, mipmapLevels);
        GL::glTexParameterf(GL::GL_TEXTURE_2D, GL::GL_TEXTURE_MIN_LOD, 0.0f);
        GL::glTexParameterf(GL::GL_TEXTURE_2D, GL::GL_TEXTURE_MAX_LOD, static_cast<float>(mipmapLevels));
        GL::glTexParameterf(GL::GL_TEXTURE_2D, GL::GL_TEXTURE_LOD_BIAS, 0.0f);
    }
    for (int i = 0; i <= mipmapLevels; ++i)
        GL::glTexImage2D(GL::GL_TEXTURE_2D, i, width >> i, height >> i, nullptr);
}

int TextureUtil::uploadTextureImageSub(int textureId, const BufferedImage& image, int x, int y, bool blur, bool clamp)
{
    bindTexture(textureId);
    setTextureBlurred(blur);
    setTextureClamped(clamp);
    std::vector<uint32_t> scratch;
    const std::span<const uint32_t> pixels = applyAnaglyph(image.pixels, scratch);
    GL::glTexSubImage2D(GL::GL_TEXTURE_2D, 0, x, y, image.width, image.height, pixels.data());
    return textureId;
}

void TextureUtil::setTextureClamped(bool clamp)
{
    const int mode = clamp ? GL::GL_CLAMP : GL::GL_REPEAT;
    GL::glTexParameteri(GL::GL_TEXTURE_2D, GL::GL_TEXTURE_WRAP_S, mode);
    GL::glTexParameteri(GL::GL_TEXTURE_2D, GL::GL_TEXTURE_WRAP_T, mode);
}

void TextureUtil::setTextureBlurred(bool blur)
{
    setTextureBlurMipmap(blur, false);
}

void TextureUtil::setTextureBlurMipmap(bool blur, bool mipmap)
{
    if (blur) {
        GL::glTexParameteri(GL::GL_TEXTURE_2D, GL::GL_TEXTURE_MIN_FILTER, mipmap ? GL::GL_LINEAR_MIPMAP_LINEAR : GL::GL_LINEAR);
        GL::glTexParameteri(GL::GL_TEXTURE_2D, GL::GL_TEXTURE_MAG_FILTER, GL::GL_LINEAR);
    } else {
        GL::glTexParameteri(GL::GL_TEXTURE_2D, GL::GL_TEXTURE_MIN_FILTER, mipmap ? GL::GL_NEAREST_MIPMAP_LINEAR : GL::GL_NEAREST);
        GL::glTexParameteri(GL::GL_TEXTURE_2D, GL::GL_TEXTURE_MAG_FILTER, GL::GL_NEAREST);
    }
}

void TextureUtil::bindTexture(int textureId)
{
    DxStateManager::bindTexture(textureId);
}

std::vector<uint32_t> TextureUtil::readImageData(const IResourceManager& resourceManager, const ResourceLocation& location)
{
    return readBufferedImage(resourceManager.getResource(location)->getInputStream()).pixels;
}

BufferedImage TextureUtil::readBufferedImage(std::span<const uint8_t> data)
{
    if (data.size() > static_cast<size_t>(std::numeric_limits<int>::max()))
        throw std::runtime_error("Image too large");

    int width = 0;
    int height = 0;
    int channels = 0;
    stbi_uc* rgba = stbi_load_from_memory(data.data(), static_cast<int>(data.size()), &width, &height, &channels, 4);
    if (!rgba)
        throw std::runtime_error(std::string("Couldn't decode image: ") + stbi_failure_reason());

    BufferedImage image;
    image.width = width;
    image.height = height;
    image.pixels.resize(static_cast<size_t>(width) * static_cast<size_t>(height));
    for (size_t i = 0; i < image.pixels.size(); ++i) {
        const stbi_uc* p = rgba + i * 4;
        image.pixels[i] = (static_cast<uint32_t>(p[3]) << 24) | (static_cast<uint32_t>(p[0]) << 16)
            | (static_cast<uint32_t>(p[1]) << 8) | static_cast<uint32_t>(p[2]);
    }
    stbi_image_free(rgba);
    return image;
}

uint32_t TextureUtil::anaglyphColor(uint32_t color)
{
    const int a = (color >> 24) & 255;
    const int r = (color >> 16) & 255;
    const int g = (color >> 8) & 255;
    const int b = color & 255;
    const int r2 = (r * 30 + g * 59 + b * 11) / 100;
    const int g2 = (r * 30 + g * 70) / 100;
    const int b2 = (r * 30 + b * 70) / 100;
    return static_cast<uint32_t>((a << 24) | (r2 << 16) | (g2 << 8) | b2);
}

void TextureUtil::processPixelValues(std::span<uint32_t> pixels, int width, int height)
{
    // Flips rows vertically.
    for (int j = 0; j < height / 2; ++j) {
        std::swap_ranges(pixels.begin() + j * width, pixels.begin() + (j + 1) * width, pixels.begin() + (height - 1 - j) * width);
    }
}

} // namespace mc

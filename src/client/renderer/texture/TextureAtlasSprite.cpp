#include "client/renderer/texture/TextureAtlasSprite.h"

#include "client/renderer/texture/TextureUtil.h"
#include "core/Java.h"
#include "core/JavaString.h"
#include "util/MathHelper.h"
#include "util/Random.h"

#include <cmath>
#include <numbers>
#include <stdexcept>

namespace mc {

std::function<std::optional<SpriteWorldView>()> spriteWorldView;

namespace {

std::string locationNameClock = "builtin/clock";
std::string locationNameCompass = "builtin/compass";

// Math.random()
double mathRandom()
{
    static Random random;
    return random.nextDouble();
}

std::optional<SpriteWorldView> currentWorldView()
{
    return spriteWorldView ? spriteWorldView() : std::nullopt;
}

SpriteFrame extractFrame(const SpriteFrame& data, int rows, int columns, int frame)
{
    SpriteFrame result(data.size());
    for (size_t i = 0; i < data.size(); ++i) {
        const std::vector<uint32_t>& level = data[i];
        if (!level.empty()) {
            const size_t length = static_cast<size_t>((rows >> i) * (columns >> i));
            const size_t start = static_cast<size_t>(frame) * length;
            if (start + length > level.size())
                throw std::out_of_range("ArrayIndexOutOfBoundsException");
            result[i].assign(level.begin() + static_cast<std::ptrdiff_t>(start), level.begin() + static_cast<std::ptrdiff_t>(start + length));
        }
    }
    return result;
}

} // namespace

std::shared_ptr<TextureAtlasSprite> TextureAtlasSprite::makeAtlasSprite(const ResourceLocation& spriteResourceLocation)
{
    const std::string s = spriteResourceLocation.toString();
    if (locationNameClock == s)
        return std::make_shared<TextureClock>(s);
    if (locationNameCompass == s)
        return std::make_shared<TextureCompass>(s);
    return std::make_shared<TextureAtlasSprite>(s);
}

void TextureAtlasSprite::setLocationNameClock(std::string clockName)
{
    locationNameClock = std::move(clockName);
}

void TextureAtlasSprite::setLocationNameCompass(std::string compassName)
{
    locationNameCompass = std::move(compassName);
}

void TextureAtlasSprite::initSprite(int inX, int inY, int originInX, int originInY, bool rotatedIn)
{
    originX = originInX;
    originY = originInY;
    rotated = rotatedIn;
    const float f = static_cast<float>(0.009999999776482582 / static_cast<double>(inX));
    const float f1 = static_cast<float>(0.009999999776482582 / static_cast<double>(inY));
    minU = static_cast<float>(originInX) / static_cast<float>(inX) + f;
    maxU = static_cast<float>(originInX + width) / static_cast<float>(inX) - f;
    minV = static_cast<float>(originInY) / static_cast<float>(inY) + f1;
    maxV = static_cast<float>(originInY + height) / static_cast<float>(inY) - f1;
}

void TextureAtlasSprite::copyFrom(const TextureAtlasSprite& atlasSprite)
{
    originX = atlasSprite.originX;
    originY = atlasSprite.originY;
    width = atlasSprite.width;
    height = atlasSprite.height;
    rotated = atlasSprite.rotated;
    minU = atlasSprite.minU;
    maxU = atlasSprite.maxU;
    minV = atlasSprite.minV;
    maxV = atlasSprite.maxV;
}

float TextureAtlasSprite::getInterpolatedU(double u) const
{
    const float f = maxU - minU;
    return minU + f * static_cast<float>(u) / 16.0f;
}

float TextureAtlasSprite::getInterpolatedV(double v) const
{
    const float f = maxV - minV;
    return minV + f * (static_cast<float>(v) / 16.0f);
}

void TextureAtlasSprite::updateAnimation()
{
    ++tickCounter;
    const AnimationMetadataSection& meta = *animationMetadata;
    if (tickCounter >= meta.getFrameTimeSingle(frameCounter)) {
        const int i = meta.getFrameIndex(frameCounter);
        const int j = meta.getFrameCount() == 0 ? getFrameCount() : meta.getFrameCount();
        frameCounter = (frameCounter + 1) % j;
        tickCounter = 0;
        const int k = meta.getFrameIndex(frameCounter);
        if (i != k && k >= 0 && k < getFrameCount())
            TextureUtil::uploadTextureMipmap(framesTextureData[static_cast<size_t>(k)], width, height, originX, originY, false, false);
    } else if (meta.isInterpolate()) {
        updateAnimationInterpolated();
    }
}

void TextureAtlasSprite::updateAnimationInterpolated()
{
    const AnimationMetadataSection& meta = *animationMetadata;
    const double d0 = 1.0 - static_cast<double>(tickCounter) / static_cast<double>(meta.getFrameTimeSingle(frameCounter));
    const int i = meta.getFrameIndex(frameCounter);
    const int j = meta.getFrameCount() == 0 ? getFrameCount() : meta.getFrameCount();
    const int k = meta.getFrameIndex((frameCounter + 1) % j);
    if (i != k && k >= 0 && k < getFrameCount()) {
        const SpriteFrame& from = framesTextureData.at(static_cast<size_t>(i));
        const SpriteFrame& to = framesTextureData[static_cast<size_t>(k)];
        if (interpolatedFrameData.size() != from.size())
            interpolatedFrameData.assign(from.size(), {});
        for (size_t l = 0; l < from.size(); ++l) {
            if (interpolatedFrameData[l].empty())
                interpolatedFrameData[l].resize(from[l].size());
            if (l < to.size() && to[l].size() == from[l].size()) {
                for (size_t i1 = 0; i1 < from[l].size(); ++i1) {
                    const uint32_t j1 = from[l][i1];
                    const uint32_t k1 = to[l][i1];
                    const int l1 = java::d2i(static_cast<double>((j1 & 16711680u) >> 16) * d0 + static_cast<double>((k1 & 16711680u) >> 16) * (1.0 - d0));
                    const int i2 = java::d2i(static_cast<double>((j1 & 65280u) >> 8) * d0 + static_cast<double>((k1 & 65280u) >> 8) * (1.0 - d0));
                    const int j2 = java::d2i(static_cast<double>(j1 & 255u) * d0 + static_cast<double>(k1 & 255u) * (1.0 - d0));
                    interpolatedFrameData[l][i1] = (j1 & 0xFF000000u) | static_cast<uint32_t>(java::shl(l1, 16) | java::shl(i2, 8) | j2);
                }
            }
        }
        TextureUtil::uploadTextureMipmap(interpolatedFrameData, width, height, originX, originY, false, false);
    }
}

void TextureAtlasSprite::loadSprite(const std::vector<BufferedImage>& images, const std::optional<AnimationMetadataSection>& meta)
{
    resetSprite();
    const int i = images[0].width;
    const int j = images[0].height;
    width = i;
    height = j;
    SpriteFrame aint(images.size());
    for (size_t k = 0; k < images.size(); ++k) {
        const BufferedImage& image = images[k];
        if (image.width > 0) {
            if (k > 0 && (image.width != i >> k || image.height != j >> k)) {
                throw std::runtime_error("Unable to load miplevel: " + std::to_string(k) + ", image is size: " + std::to_string(image.width) + "x"
                    + std::to_string(image.height) + ", expected " + std::to_string(i >> k) + "x" + std::to_string(j >> k));
            }
            aint[k] = image.pixels;
        }
    }

    if (!meta) {
        if (j != i)
            throw std::runtime_error("broken aspect ratio and not an animation");
        framesTextureData.push_back(std::move(aint));
    } else {
        const int j1 = j / i;
        const int k1 = i;
        const int l = i;
        height = width;
        if (meta->getFrameCount() > 0) {
            for (const int i1 : meta->getFrameIndexSet()) {
                if (i1 >= j1)
                    throw std::runtime_error("invalid frameindex " + std::to_string(i1));
                allocateFrameTextureData(i1);
                framesTextureData[static_cast<size_t>(i1)] = extractFrame(aint, k1, l, i1);
            }
            animationMetadata = meta;
        } else {
            std::vector<AnimationFrame> list;
            for (int l1 = 0; l1 < j1; ++l1) {
                framesTextureData.push_back(extractFrame(aint, k1, l, l1));
                list.push_back({ l1, -1 });
            }
            animationMetadata.emplace(std::move(list), width, height, meta->getFrameTime(), meta->isInterpolate());
        }
    }
}

void TextureAtlasSprite::generateMipmaps(int level)
{
    // Null frames (gaps in an animation's frame list) are dropped, so later frames move down, as in Java.
    std::vector<SpriteFrame> list;
    for (const SpriteFrame& frame : framesTextureData) {
        if (!frame.empty())
            list.push_back(TextureUtil::generateMipmapData(level, width, frame));
    }
    setFramesTextureData(std::move(list));
}

void TextureAtlasSprite::allocateFrameTextureData(int index)
{
    if (framesTextureData.size() <= static_cast<size_t>(index))
        framesTextureData.resize(static_cast<size_t>(index) + 1);
}

void TextureAtlasSprite::resetSprite()
{
    animationMetadata.reset();
    setFramesTextureData({});
    frameCounter = 0;
    tickCounter = 0;
}

std::string TextureAtlasSprite::toString() const
{
    return "TextureAtlasSprite{name='" + iconName + "', frameCount=" + std::to_string(framesTextureData.size()) + ", rotated=" + java::toString(rotated)
        + ", x=" + std::to_string(originX) + ", y=" + std::to_string(originY) + ", height=" + std::to_string(height) + ", width=" + std::to_string(width)
        + ", u0=" + java::toString(minU) + ", u1=" + java::toString(maxU) + ", v0=" + java::toString(minV) + ", v1=" + java::toString(maxV) + "}";
}

void TextureClock::updateAnimation()
{
    if (framesTextureData.empty())
        return;
    double d0 = 0.0;
    if (const std::optional<SpriteWorldView> world = currentWorldView()) {
        d0 = static_cast<double>(world->celestialAngle);
        if (!world->surfaceWorld)
            d0 = mathRandom();
    }
    double d1 = d0 - currentAngle;
    while (d1 < -0.5)
        ++d1;
    while (d1 >= 0.5)
        --d1;
    d1 = MathHelper::clamp_double(d1, -1.0, 1.0);
    angleDelta += d1 * 0.1;
    angleDelta *= 0.8;
    currentAngle += angleDelta;
    const int frames = getFrameCount();
    int i = java::d2i((currentAngle + 1.0) * static_cast<double>(frames)) % frames;
    while (i < 0)
        i = (i + frames) % frames;
    if (i != frameCounter) {
        frameCounter = i;
        TextureUtil::uploadTextureMipmap(framesTextureData[static_cast<size_t>(frameCounter)], width, height, originX, originY, false, false);
    }
}

std::string TextureCompass::locationSprite;

TextureCompass::TextureCompass(std::string iconName)
    : TextureAtlasSprite(iconName)
{
    locationSprite = std::move(iconName);
}

void TextureCompass::updateAnimation()
{
    const std::optional<SpriteWorldView> world = currentWorldView();
    if (world)
        updateCompass(&*world, world->playerX, world->playerZ, world->playerYaw, false, false);
    else
        updateCompass(nullptr, 0.0, 0.0, 0.0, true, false);
}

void TextureCompass::updateCompass(const SpriteWorldView* world, double x, double z, double yaw, bool spin, bool snap)
{
    if (framesTextureData.empty())
        return;
    double d0 = 0.0;
    if (world && !spin) {
        const double d1 = static_cast<double>(world->spawnX) - x;
        const double d2 = static_cast<double>(world->spawnZ) - z;
        yaw = std::fmod(yaw, 360.0);
        d0 = -((yaw - 90.0) * std::numbers::pi / 180.0 - std::atan2(d2, d1));
        if (!world->surfaceWorld)
            d0 = mathRandom() * std::numbers::pi * 2.0;
    }
    if (snap) {
        currentAngle = d0;
    } else {
        double d3 = d0 - currentAngle;
        while (d3 < -std::numbers::pi)
            d3 += std::numbers::pi * 2.0;
        while (d3 >= std::numbers::pi)
            d3 -= std::numbers::pi * 2.0;
        d3 = MathHelper::clamp_double(d3, -1.0, 1.0);
        angleDelta += d3 * 0.1;
        angleDelta *= 0.8;
        currentAngle += angleDelta;
    }
    const int frames = getFrameCount();
    int i = java::d2i((currentAngle / (std::numbers::pi * 2.0) + 1.0) * static_cast<double>(frames)) % frames;
    while (i < 0)
        i = (i + frames) % frames;
    if (i != frameCounter) {
        frameCounter = i;
        TextureUtil::uploadTextureMipmap(framesTextureData[static_cast<size_t>(frameCounter)], width, height, originX, originY, false, false);
    }
}

} // namespace mc

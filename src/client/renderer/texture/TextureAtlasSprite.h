#pragma once

// Original: net/minecraft/client/renderer/texture/TextureAtlasSprite.java, TextureClock.java and
// TextureCompass.java
// A frame is one int[][] of Java: the pixels of each mip level (0xAARRGGBB). An empty level vector stands for a
// null level and an empty frame for a null frame.

#include "client/renderer/texture/BufferedImage.h"
#include "client/resources/data/MetadataSections.h"
#include "util/ResourceLocation.h"

#include <cstdint>
#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <vector>

namespace mc {

using SpriteFrame = std::vector<std::vector<uint32_t>>;

class TextureAtlasSprite {
public:
    explicit TextureAtlasSprite(std::string spriteName)
        : iconName(std::move(spriteName))
    {
    }
    virtual ~TextureAtlasSprite() = default;

    // The clock and compass sprites get their own classes, picked by name.
    static std::shared_ptr<TextureAtlasSprite> makeAtlasSprite(const ResourceLocation& spriteResourceLocation);
    static void setLocationNameClock(std::string clockName);
    static void setLocationNameCompass(std::string compassName);

    void initSprite(int inX, int inY, int originInX, int originInY, bool rotatedIn);
    void copyFrom(const TextureAtlasSprite& atlasSprite);
    int getOriginX() const { return originX; }
    int getOriginY() const { return originY; }
    int getIconWidth() const { return width; }
    int getIconHeight() const { return height; }
    float getMinU() const { return minU; }
    float getMaxU() const { return maxU; }
    float getInterpolatedU(double u) const;
    float getMinV() const { return minV; }
    float getMaxV() const { return maxV; }
    float getInterpolatedV(double v) const;
    const std::string& getIconName() const { return iconName; }
    // Uploads the next frame into the bound atlas texture when its time has come.
    virtual void updateAnimation();
    const SpriteFrame& getFrameTextureData(int index) const { return framesTextureData.at(static_cast<size_t>(index)); }
    int getFrameCount() const { return static_cast<int>(framesTextureData.size()); }
    void setIconWidth(int newWidth) { width = newWidth; }
    void setIconHeight(int newHeight) { height = newHeight; }
    // images[0] is the texture, images[n] an optional pre-made mip level (width 0 when absent). Throws
    // std::runtime_error like Java's RuntimeExceptions.
    void loadSprite(const std::vector<BufferedImage>& images, const std::optional<AnimationMetadataSection>& meta);
    void generateMipmaps(int level);
    void clearFramesTextureData() { framesTextureData.clear(); }
    bool hasAnimationMetadata() const { return animationMetadata.has_value(); }
    void setFramesTextureData(std::vector<SpriteFrame> newFramesTextureData) { framesTextureData = std::move(newFramesTextureData); }
    bool isRotated() const { return rotated; }
    std::string toString() const;

protected:
    std::vector<SpriteFrame> framesTextureData;
    SpriteFrame interpolatedFrameData;
    std::optional<AnimationMetadataSection> animationMetadata;
    bool rotated = false;
    int originX = 0;
    int originY = 0;
    int width = 0;
    int height = 0;
    int frameCounter = 0;
    int tickCounter = 0;

private:
    void updateAnimationInterpolated();
    void allocateFrameTextureData(int index);
    void resetSprite();

    std::string iconName;
    float minU = 0.0f;
    float maxU = 0.0f;
    float minV = 0.0f;
    float maxV = 0.0f;
};

// What the clock and compass need from the client (Java reads Minecraft.theWorld and thePlayer). Unset or
// returning nullopt means "no world": the clock points at 0, the compass spins.
struct SpriteWorldView {
    float celestialAngle;
    bool surfaceWorld;
    int spawnX;
    int spawnZ;
    double playerX;
    double playerZ;
    double playerYaw;
};

class TextureClock : public TextureAtlasSprite {
public:
    using TextureAtlasSprite::TextureAtlasSprite;
    void updateAnimation() override;

private:
    double currentAngle = 0.0;
    double angleDelta = 0.0;
};

class TextureCompass : public TextureAtlasSprite {
public:
    explicit TextureCompass(std::string iconName);
    void updateAnimation() override;
    void updateCompass(const SpriteWorldView* world, double x, double z, double yaw, bool spin, bool snap);

    double currentAngle = 0.0;
    double angleDelta = 0.0;
    static std::string locationSprite;
};

// Set by the client once worlds exist (stage 8).
extern std::function<std::optional<SpriteWorldView>()> spriteWorldView;

} // namespace mc

#pragma once

// Original: net/minecraft/client/resources/data/AnimationFrame.java, AnimationMetadataSection.java,
// AnimationMetadataSectionSerializer.java, TextureMetadataSection.java and TextureMetadataSectionSerializer.java.
// The deserializers throw std::runtime_error where Java throws JsonParseException / IllegalArgumentException
// (both RuntimeExceptions, which TextureMap catches per sprite).

#include <nlohmann/json_fwd.hpp>

#include <optional>
#include <set>
#include <vector>

namespace mc {

class IResource;

struct AnimationFrame {
    int frameIndex;
    int frameTime = -1;

    bool hasNoTime() const { return frameTime == -1; }
};

class AnimationMetadataSection {
public:
    AnimationMetadataSection(std::vector<AnimationFrame> frames, int frameWidthIn, int frameHeightIn, int frameTimeIn, bool interpolateIn)
        : animationFrames(std::move(frames))
        , frameWidth(frameWidthIn)
        , frameHeight(frameHeightIn)
        , frameTime(frameTimeIn)
        , interpolate(interpolateIn)
    {
    }

    // The "animation" section of an .mcmeta.
    static AnimationMetadataSection deserialize(const nlohmann::json& json);
    // The resource's "animation" section, if any.
    static std::optional<AnimationMetadataSection> fromResource(const IResource& resource);

    int getFrameHeight() const { return frameHeight; }
    int getFrameWidth() const { return frameWidth; }
    int getFrameCount() const { return static_cast<int>(animationFrames.size()); }
    int getFrameTime() const { return frameTime; }
    bool isInterpolate() const { return interpolate; }
    int getFrameTimeSingle(int frame) const
    {
        const AnimationFrame& f = animationFrames.at(static_cast<size_t>(frame));
        return f.hasNoTime() ? frameTime : f.frameTime;
    }
    bool frameHasTime(int frame) const { return !animationFrames.at(static_cast<size_t>(frame)).hasNoTime(); }
    int getFrameIndex(int frame) const { return animationFrames.at(static_cast<size_t>(frame)).frameIndex; }
    // Java: a HashSet<Integer>, which iterates small non-negative ints in ascending order.
    std::set<int> getFrameIndexSet() const;

private:
    std::vector<AnimationFrame> animationFrames;
    int frameWidth;
    int frameHeight;
    int frameTime;
    bool interpolate;
};

struct TextureMetadataSection {
    bool textureBlur = false;
    bool textureClamp = false;
    std::vector<int> listMipmaps;

    static TextureMetadataSection deserialize(const nlohmann::json& json);
    static std::optional<TextureMetadataSection> fromResource(const IResource& resource);
};

} // namespace mc

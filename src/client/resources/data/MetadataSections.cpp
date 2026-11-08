#include "client/resources/data/MetadataSections.h"

#include "client/resources/IResource.h"
#include "core/Java.h"

#include <nlohmann/json.hpp>

#include <stdexcept>
#include <string>

namespace mc {

namespace {

// JsonUtils.getInt(element, name): a JSON number, converted like LazilyParsedNumber.intValue.
int getInt(const nlohmann::json& element, const std::string& name)
{
    if (element.is_number_integer())
        return static_cast<int32_t>(element.get<int64_t>());
    if (element.is_number_float())
        return java::d2i(element.get<double>());
    throw std::runtime_error("Expected " + name + " to be a Int, was " + element.dump());
}

int getInt(const nlohmann::json& object, const std::string& key, int fallback)
{
    const auto it = object.find(key);
    return it == object.end() ? fallback : getInt(*it, key);
}

// JsonUtils.getBoolean(object, key, fallback): JsonPrimitive.getAsBoolean (strings parse, numbers are false).
bool getBoolean(const nlohmann::json& object, const std::string& key, bool fallback)
{
    const auto it = object.find(key);
    if (it == object.end())
        return fallback;
    if (it->is_boolean())
        return it->get<bool>();
    if (it->is_string()) {
        // Boolean.parseBoolean
        const std::string text = it->get<std::string>();
        return text.size() == 4 && (text[0] | 32) == 't' && (text[1] | 32) == 'r' && (text[2] | 32) == 'u' && (text[3] | 32) == 'e';
    }
    if (it->is_number())
        return false;
    throw std::runtime_error("Expected " + key + " to be a Boolean, was " + it->dump());
}

void inclusiveBetween(int64_t start, int64_t end, int64_t value, const char* message)
{
    if (value < start || value > end)
        throw std::invalid_argument(message);
}

} // namespace

AnimationMetadataSection AnimationMetadataSection::deserialize(const nlohmann::json& json)
{
    if (!json.is_object())
        throw std::runtime_error("Expected metadata section to be a JsonObject, was " + json.dump());
    std::vector<AnimationFrame> list;
    const int frameTime = getInt(json, "frametime", 1);
    if (frameTime != 1)
        inclusiveBetween(1, 2147483647, frameTime, "Invalid default frame time");

    if (const auto frames = json.find("frames"); frames != json.end()) {
        if (!frames->is_array())
            throw std::runtime_error("Invalid animation->frames: expected array, was " + frames->dump());
        for (size_t j = 0; j < frames->size(); ++j) {
            const nlohmann::json& element = (*frames)[j];
            const std::string name = "frames[" + std::to_string(j) + "]";
            if (element.is_primitive() && !element.is_null()) {
                list.push_back({ getInt(element, name), -1 });
            } else if (element.is_object()) {
                const int time = getInt(element, "time", -1);
                if (element.contains("time"))
                    inclusiveBetween(1, 2147483647, time, "Invalid frame time");
                const auto index = element.find("index");
                if (index == element.end())
                    throw std::runtime_error("Missing index, expected to find a Int");
                const int frameIndex = getInt(*index, "index");
                inclusiveBetween(0, 2147483647, frameIndex, "Invalid frame index");
                list.push_back({ frameIndex, time });
            }
        }
    }

    const int width = getInt(json, "width", -1);
    const int height = getInt(json, "height", -1);
    if (width != -1)
        inclusiveBetween(1, 2147483647, width, "Invalid width");
    if (height != -1)
        inclusiveBetween(1, 2147483647, height, "Invalid height");
    const bool interpolate = getBoolean(json, "interpolate", false);
    return AnimationMetadataSection(std::move(list), width, height, frameTime, interpolate);
}

std::optional<AnimationMetadataSection> AnimationMetadataSection::fromResource(const IResource& resource)
{
    const nlohmann::json* section = resource.getMetadata("animation");
    if (!section)
        return std::nullopt;
    return deserialize(*section);
}

std::set<int> AnimationMetadataSection::getFrameIndexSet() const
{
    std::set<int> set;
    for (const AnimationFrame& frame : animationFrames)
        set.insert(frame.frameIndex);
    return set;
}

TextureMetadataSection TextureMetadataSection::deserialize(const nlohmann::json& json)
{
    if (!json.is_object())
        throw std::runtime_error("Not a JSON Object: " + json.dump());
    TextureMetadataSection section;
    section.textureBlur = getBoolean(json, "blur", false);
    section.textureClamp = getBoolean(json, "clamp", false);
    if (const auto mipmaps = json.find("mipmaps"); mipmaps != json.end()) {
        if (!mipmaps->is_array())
            throw std::runtime_error("Invalid texture->mipmaps: expected array, was " + mipmaps->dump());
        for (size_t i = 0; i < mipmaps->size(); ++i) {
            const nlohmann::json& element = (*mipmaps)[i];
            if (element.is_object())
                throw std::runtime_error("Invalid texture->mipmap->" + std::to_string(i) + ": expected number, was " + element.dump());
            if (element.is_number())
                section.listMipmaps.push_back(getInt(element, "mipmap"));
            else if (element.is_primitive() && !element.is_null())
                throw std::runtime_error("Invalid texture->mipmap->" + std::to_string(i) + ": expected number, was " + element.dump());
        }
    }
    return section;
}

std::optional<TextureMetadataSection> TextureMetadataSection::fromResource(const IResource& resource)
{
    const nlohmann::json* section = resource.getMetadata("texture");
    if (!section)
        return std::nullopt;
    return deserialize(*section);
}

} // namespace mc

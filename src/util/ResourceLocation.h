#pragma once

// Original: net/minecraft/util/ResourceLocation.java

#include <cstdint>
#include <functional>
#include <string>
#include <string_view>

namespace mc {

class ResourceLocation {
public:
    // "domain:path" or "path" (domain "minecraft"). Quirk kept from the original: a domain of one character
    // ("a:path") is dropped and becomes "minecraft".
    explicit ResourceLocation(std::string_view resourceName);
    ResourceLocation(std::string_view resourceDomain, std::string_view resourcePath);

    const std::string& getResourceDomain() const { return resourceDomain; }
    const std::string& getResourcePath() const { return resourcePath; }
    std::string toString() const { return resourceDomain + ':' + resourcePath; }

    // Java hashCode(), for code that orders or seeds things by it.
    int32_t hashCode() const;

    bool operator==(const ResourceLocation& other) const = default;

private:
    std::string resourceDomain;
    std::string resourcePath;
};

} // namespace mc

template <>
struct std::hash<mc::ResourceLocation> {
    size_t operator()(const mc::ResourceLocation& location) const noexcept
    {
        const size_t h = std::hash<std::string>()(location.getResourceDomain());
        return h ^ (std::hash<std::string>()(location.getResourcePath()) + 0x9e3779b97f4a7c15ULL + (h << 6) + (h >> 2));
    }
};

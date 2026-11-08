#pragma once

// Original: net/minecraft/client/resources/model/ModelResourceLocation.java
// "domain:path#variant"; the variant defaults to "normal" and is lower-cased.

#include "util/ResourceLocation.h"

#include <string>
#include <string_view>

namespace mc {

class ModelResourceLocation : public ResourceLocation {
public:
    explicit ModelResourceLocation(std::string_view pathString);
    ModelResourceLocation(const ResourceLocation& location, std::string_view variantIn);
    ModelResourceLocation(std::string_view location, std::string_view variantIn);

    const std::string& getVariant() const { return variant; }
    std::string toString() const { return ResourceLocation::toString() + '#' + variant; }
    int32_t hashCode() const;

    bool operator==(const ModelResourceLocation& other) const
    {
        return static_cast<const ResourceLocation&>(*this) == static_cast<const ResourceLocation&>(other) && variant == other.variant;
    }

private:
    struct Parts {
        std::string location;
        std::string variant;
    };
    explicit ModelResourceLocation(Parts parts);
    static Parts parsePathString(std::string_view pathString);

    std::string variant;
};

} // namespace mc

template <>
struct std::hash<mc::ModelResourceLocation> {
    size_t operator()(const mc::ModelResourceLocation& location) const noexcept
    {
        const size_t h = std::hash<mc::ResourceLocation>()(location);
        return h ^ (std::hash<std::string>()(location.getVariant()) + 0x9e3779b97f4a7c15ULL + (h << 6) + (h >> 2));
    }
};

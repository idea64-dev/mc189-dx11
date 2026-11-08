#include "client/resources/model/ModelResourceLocation.h"

#include "core/Java.h"

namespace mc {

ModelResourceLocation::Parts ModelResourceLocation::parsePathString(std::string_view pathString)
{
    // Java keeps the '#' in the path when it is at index 0 or 1.
    Parts parts { std::string(pathString), "" };
    const size_t i = pathString.find('#');
    if (i != std::string_view::npos) {
        parts.variant = std::string(pathString.substr(i + 1));
        if (i > 1)
            parts.location = std::string(pathString.substr(0, i));
    }
    return parts;
}

ModelResourceLocation::ModelResourceLocation(Parts parts)
    : ResourceLocation(parts.location)
{
    variant = parts.variant.empty() ? "normal" : parts.variant;
    for (char& c : variant) {
        if (c >= 'A' && c <= 'Z')
            c = static_cast<char>(c - 'A' + 'a');
    }
}

ModelResourceLocation::ModelResourceLocation(std::string_view pathString)
    : ModelResourceLocation(parsePathString(pathString))
{
}

ModelResourceLocation::ModelResourceLocation(const ResourceLocation& location, std::string_view variantIn)
    : ModelResourceLocation(location.toString(), variantIn)
{
}

ModelResourceLocation::ModelResourceLocation(std::string_view location, std::string_view variantIn)
    : ModelResourceLocation(parsePathString(std::string(location) + '#' + std::string(variantIn)))
{
}

int32_t ModelResourceLocation::hashCode() const
{
    return java::add(java::mul(31, ResourceLocation::hashCode()), java::hashCode(variant));
}

} // namespace mc

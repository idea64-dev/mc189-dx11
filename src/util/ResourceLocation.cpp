#include "util/ResourceLocation.h"

#include "core/Java.h"

namespace mc {

namespace {

std::string toLowerAscii(std::string_view text)
{
    std::string result(text);
    for (char& c : result) {
        if (c >= 'A' && c <= 'Z')
            c = static_cast<char>(c - 'A' + 'a');
    }
    return result;
}

} // namespace

ResourceLocation::ResourceLocation(std::string_view resourceName)
{
    std::string_view domain;
    std::string_view path = resourceName;

    const size_t i = resourceName.find(':');
    if (i != std::string_view::npos) {
        path = resourceName.substr(i + 1);
        if (i > 1)
            domain = resourceName.substr(0, i);
    }

    resourceDomain = domain.empty() ? "minecraft" : toLowerAscii(domain);
    resourcePath = path;
}

ResourceLocation::ResourceLocation(std::string_view domain, std::string_view path)
    : resourceDomain(domain.empty() ? "minecraft" : toLowerAscii(domain))
    , resourcePath(path)
{
}

int32_t ResourceLocation::hashCode() const
{
    return java::add(java::mul(31, java::hashCode(resourceDomain)), java::hashCode(resourcePath));
}

} // namespace mc

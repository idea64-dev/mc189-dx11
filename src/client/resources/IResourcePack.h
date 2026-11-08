#pragma once

// Original: net/minecraft/client/resources/IResourcePack.java

#include "client/renderer/texture/BufferedImage.h"
#include "util/ResourceLocation.h"

#include <nlohmann/json.hpp>

#include <cstdint>
#include <optional>
#include <set>
#include <stdexcept>
#include <string>
#include <vector>

namespace mc {

// Java: java.io.FileNotFoundException(resourcePath)
class ResourceNotFoundException : public std::runtime_error {
public:
    explicit ResourceNotFoundException(const std::string& resourcePath)
        : std::runtime_error(resourcePath)
    {
    }
};

class IResourcePack {
public:
    virtual ~IResourcePack() = default;

    // Whole resource contents; throws ResourceNotFoundException when absent.
    virtual std::vector<uint8_t> getInputStream(const ResourceLocation& location) const = 0;
    virtual bool resourceExists(const ResourceLocation& location) const = 0;
    virtual std::set<std::string> getResourceDomains() const = 0;
    // The "<section>" object of the pack's pack.mcmeta, or nullopt. Throws on malformed JSON or a section that
    // isn't an object.
    virtual std::optional<nlohmann::json> getPackMetadata(const std::string& section) const = 0;
    // pack.png; throws when it is missing or unreadable.
    virtual BufferedImage getPackImage() const = 0;
    virtual std::string getPackName() const = 0;
};

} // namespace mc

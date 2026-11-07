#pragma once

// Original: net/minecraft/client/resources/IResourcePack.java
// Pack metadata (pack.mcmeta) and pack images arrive with the resource pack screen (ROADMAP stage 3).

#include "util/ResourceLocation.h"

#include <cstdint>
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
    virtual std::string getPackName() const = 0;
};

} // namespace mc

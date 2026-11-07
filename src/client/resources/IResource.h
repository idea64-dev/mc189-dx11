#pragma once

// Original: net/minecraft/client/resources/IResource.java and SimpleResource.java
// Java's IMetadataSerializer registry is replaced by direct access to the parsed .mcmeta sections; each
// section type (texture, animation, font, language, pack) parses its own JSON where it is used.

#include "util/ResourceLocation.h"

#include <nlohmann/json_fwd.hpp>

#include <cstdint>
#include <memory>
#include <optional>
#include <string>
#include <vector>

namespace mc {

class IResource {
public:
    virtual ~IResource() = default;

    virtual const ResourceLocation& getResourceLocation() const = 0;
    virtual const std::vector<uint8_t>& getInputStream() const = 0;
    virtual bool hasMetadata() const = 0;
    // The "<section>" object of the resource's .mcmeta, or null if absent. Throws on malformed JSON, like Gson.
    virtual const nlohmann::json* getMetadata(const std::string& section) const = 0;
    virtual const std::string& getResourcePackName() const = 0;
};

class SimpleResource : public IResource {
public:
    SimpleResource(std::string resourcePackName, ResourceLocation location, std::vector<uint8_t> data,
        std::optional<std::vector<uint8_t>> mcmeta);
    ~SimpleResource() override;

    const ResourceLocation& getResourceLocation() const override { return location; }
    const std::vector<uint8_t>& getInputStream() const override { return data; }
    bool hasMetadata() const override { return mcmeta.has_value(); }
    const nlohmann::json* getMetadata(const std::string& section) const override;
    const std::string& getResourcePackName() const override { return resourcePackName; }

private:
    std::string resourcePackName;
    ResourceLocation location;
    std::vector<uint8_t> data;
    std::optional<std::vector<uint8_t>> mcmeta;
    mutable std::unique_ptr<nlohmann::json> mcmetaJson;
    mutable bool mcmetaJsonChecked = false;
};

} // namespace mc

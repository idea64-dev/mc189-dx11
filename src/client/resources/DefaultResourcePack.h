#pragma once

// Original: net/minecraft/client/resources/DefaultResourcePack.java
// Java read "/assets/<domain>/<path>" from the classpath, i.e. the client jar; here the jar is opened directly.

#include "client/resources/IResourcePack.h"

#include <filesystem>
#include <memory>
#include <optional>
#include <string>
#include <unordered_map>

namespace mc {

class ZipArchive;

class DefaultResourcePack : public IResourcePack {
public:
    DefaultResourcePack(std::shared_ptr<const ZipArchive> clientJar,
        std::unordered_map<std::string, std::filesystem::path> mapAssets);

    // The jar first, then the asset index.
    std::vector<uint8_t> getInputStream(const ResourceLocation& location) const override;
    // Asset index only (window icons, sounds, languages other than en_US).
    std::optional<std::vector<uint8_t>> getInputStreamAssets(const ResourceLocation& location) const;
    bool resourceExists(const ResourceLocation& location) const override;
    std::set<std::string> getResourceDomains() const override { return { "minecraft", "realms" }; }
    // Java reads pack.mcmeta from the asset index (it lists the available languages); any error gives nullopt.
    std::optional<nlohmann::json> getPackMetadata(const std::string& section) const override;
    // pack.png from the root of the jar.
    BufferedImage getPackImage() const override;
    std::string getPackName() const override { return "Default"; }

private:
    std::optional<std::vector<uint8_t>> getResourceStream(const ResourceLocation& location) const;

    std::shared_ptr<const ZipArchive> clientJar;
    std::unordered_map<std::string, std::filesystem::path> mapAssets;
};

} // namespace mc

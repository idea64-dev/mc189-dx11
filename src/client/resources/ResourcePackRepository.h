#pragma once

// Original: net/minecraft/client/resources/ResourcePackRepository.java and data/PackMetadataSection.java
// Server resource packs (download, server-resource-packs/) arrive with multiplayer (stage 14).

#include "client/renderer/texture/BufferedImage.h"
#include "util/ResourceLocation.h"

#include <nlohmann/json_fwd.hpp>

#include <filesystem>
#include <memory>
#include <optional>
#include <string>
#include <vector>

namespace mc {

class GameSettings;
class IChatComponent;
class IResourcePack;
class TextureManager;

// Java: PackMetadataSection, the "pack" section of pack.mcmeta.
struct PackMetadataSection {
    std::shared_ptr<IChatComponent> packDescription;
    int packFormat = 0;

    // Throws (JsonParseException) on a missing description or pack_format.
    static PackMetadataSection deserialize(const nlohmann::json& element);
};

class ResourcePackRepository {
public:
    class Entry {
    public:
        explicit Entry(std::filesystem::path resourcePackFile);

        void updateResourcePack(const ResourcePackRepository& repository);
        void bindTexturePackIcon(TextureManager& textureManager);
        void closeResourcePack();
        std::shared_ptr<IResourcePack> getResourcePack() const { return reResourcePack; }
        std::string getResourcePackName() const;
        std::string getTexturePackDescription() const;
        // Java: func_183027_f, the pack_format. Java throws NullPointerException without a "pack" section.
        int getPackFormat() const;
        // "<file name>:<zip|folder>:<last modified ms>"; entries with the same string are the same pack.
        std::string toString() const;

    private:
        std::filesystem::path resourcePackFile;
        std::shared_ptr<IResourcePack> reResourcePack;
        std::optional<PackMetadataSection> rePackMetadataSection;
        BufferedImage texturePackIcon;
        std::optional<ResourceLocation> locationTexturePackIcon;
    };

    ResourcePackRepository(std::filesystem::path dirResourcepacks, std::filesystem::path dirServerResourcepacks,
        std::shared_ptr<IResourcePack> defaultResourcePack, GameSettings& settings);

    void updateRepositoryEntriesAll();
    std::vector<std::shared_ptr<Entry>> getRepositoryEntriesAll() const { return repositoryEntriesAll; }
    std::vector<std::shared_ptr<Entry>> getRepositoryEntries() const { return repositoryEntries; }
    void setRepositories(std::vector<std::shared_ptr<Entry>> repositories) { repositoryEntries = std::move(repositories); }
    const std::filesystem::path& getDirResourcepacks() const { return dirResourcepacks; }
    // The server-provided pack (stage 14); always null for now.
    std::shared_ptr<IResourcePack> getResourcePackInstance() const { return nullptr; }

    const std::shared_ptr<IResourcePack> rprDefaultResourcePack;

private:
    void fixDirResourcepacks();
    std::vector<std::filesystem::path> getResourcePackFiles() const;

    std::filesystem::path dirResourcepacks;
    std::filesystem::path dirServerResourcepacks;
    std::vector<std::shared_ptr<Entry>> repositoryEntriesAll;
    std::vector<std::shared_ptr<Entry>> repositoryEntries;
};

} // namespace mc

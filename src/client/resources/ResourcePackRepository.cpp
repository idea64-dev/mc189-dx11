#include "client/resources/ResourcePackRepository.h"

#include "client/renderer/texture/SimpleTexture.h"
#include "client/renderer/texture/TextureManager.h"
#include "client/resources/AbstractResourcePack.h"
#include "client/settings/GameSettings.h"
#include "core/Log.h"
#include "core/StringUtil.h"
#include "util/ChatComponent.h"

#include <nlohmann/json.hpp>

#include <algorithm>
#include <chrono>
#include <format>

namespace mc {

PackMetadataSection PackMetadataSection::deserialize(const nlohmann::json& element)
{
    PackMetadataSection section;
    const auto description = element.find("description");
    if (description != element.end())
        section.packDescription = IChatComponent::deserialize(*description);
    if (section.packDescription == nullptr)
        throw JsonParseException("Invalid/missing description!");

    // JsonUtils.getInt
    const auto format = element.find("pack_format");
    if (format == element.end())
        throw JsonParseException("Missing pack_format, expected to find a Int");
    if (!format->is_number())
        throw JsonParseException("Expected pack_format to be a Int, was " + format->dump());
    section.packFormat = format->is_number_float() ? static_cast<int>(format->get<double>()) : format->get<int>();
    return section;
}

// --- Entry ---

ResourcePackRepository::Entry::Entry(std::filesystem::path resourcePackFileIn)
    : resourcePackFile(std::move(resourcePackFileIn))
{
}

void ResourcePackRepository::Entry::updateResourcePack(const ResourcePackRepository& repository)
{
    std::error_code ec;
    if (std::filesystem::is_directory(resourcePackFile, ec))
        reResourcePack = std::make_shared<FolderResourcePack>(resourcePackFile);
    else
        reResourcePack = std::make_shared<FileResourcePack>(resourcePackFile);

    if (const auto section = reResourcePack->getPackMetadata("pack"))
        rePackMetadataSection = PackMetadataSection::deserialize(*section);
    else
        rePackMetadataSection.reset();

    try {
        texturePackIcon = reResourcePack->getPackImage();
    } catch (const std::exception&) {
        texturePackIcon = {};
    }
    if (texturePackIcon.pixels.empty())
        texturePackIcon = repository.rprDefaultResourcePack->getPackImage();

    closeResourcePack();
}

void ResourcePackRepository::Entry::bindTexturePackIcon(TextureManager& textureManager)
{
    if (!locationTexturePackIcon)
        locationTexturePackIcon = textureManager.getDynamicTextureLocation("texturepackicon", std::make_shared<DynamicTexture>(texturePackIcon));
    textureManager.bindTexture(*locationTexturePackIcon);
}

void ResourcePackRepository::Entry::closeResourcePack()
{
    if (auto* file = dynamic_cast<FileResourcePack*>(reResourcePack.get()))
        file->close();
}

std::string ResourcePackRepository::Entry::getResourcePackName() const
{
    return reResourcePack->getPackName();
}

std::string ResourcePackRepository::Entry::getTexturePackDescription() const
{
    return rePackMetadataSection ? rePackMetadataSection->packDescription->getFormattedText()
                                 : EnumChatFormatting::toString('c') + "Invalid pack.mcmeta (or missing 'pack' section)";
}

int ResourcePackRepository::Entry::getPackFormat() const
{
    if (!rePackMetadataSection)
        throw std::runtime_error("java.lang.NullPointerException: resource pack " + getResourcePackName() + " has no 'pack' section");
    return rePackMetadataSection->packFormat;
}

std::string ResourcePackRepository::Entry::toString() const
{
    std::error_code ec;
    const bool directory = std::filesystem::is_directory(resourcePackFile, ec);
    int64_t lastModified = 0;
    const auto time = std::filesystem::last_write_time(resourcePackFile, ec);
    if (!ec)
        lastModified = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::clock_cast<std::chrono::system_clock>(time).time_since_epoch()).count();
    return std::format("{}:{}:{}", toUtf8(resourcePackFile.filename().wstring()), directory ? "folder" : "zip", lastModified);
}

// --- ResourcePackRepository ---

ResourcePackRepository::ResourcePackRepository(std::filesystem::path dirResourcepacksIn, std::filesystem::path dirServerResourcepacksIn,
    std::shared_ptr<IResourcePack> defaultResourcePack, GameSettings& settings)
    : rprDefaultResourcePack(std::move(defaultResourcePack))
    , dirResourcepacks(std::move(dirResourcepacksIn))
    , dirServerResourcepacks(std::move(dirServerResourcepacksIn))
{
    fixDirResourcepacks();
    updateRepositoryEntriesAll();

    for (auto it = settings.resourcePacks.begin(); it != settings.resourcePacks.end();) {
        const std::string& s = *it;
        bool removed = false;
        for (const auto& entry : repositoryEntriesAll) {
            if (entry->getResourcePackName() != s)
                continue;
            const auto& incompatible = settings.incompatibleResourcePacks;
            if (entry->getPackFormat() == 1 || std::find(incompatible.begin(), incompatible.end(), entry->getResourcePackName()) != incompatible.end()) {
                repositoryEntries.push_back(entry);
                break;
            }
            Log::warn("Removed selected resource pack {} because it's no longer compatible", entry->getResourcePackName());
            it = settings.resourcePacks.erase(it);
            removed = true;
            break;
        }
        if (!removed)
            ++it;
    }
}

void ResourcePackRepository::fixDirResourcepacks()
{
    std::error_code ec;
    if (std::filesystem::exists(dirResourcepacks, ec)) {
        if (!std::filesystem::is_directory(dirResourcepacks, ec)
            && (!std::filesystem::remove(dirResourcepacks, ec) || !std::filesystem::create_directories(dirResourcepacks, ec)))
            Log::warn("Unable to recreate resourcepack folder, it exists but is not a directory: {}", toUtf8(dirResourcepacks.wstring()));
    } else if (!std::filesystem::create_directories(dirResourcepacks, ec)) {
        Log::warn("Unable to create resourcepack folder: {}", toUtf8(dirResourcepacks.wstring()));
    }
}

std::vector<std::filesystem::path> ResourcePackRepository::getResourcePackFiles() const
{
    std::vector<std::filesystem::path> files;
    std::error_code ec;
    if (!std::filesystem::is_directory(dirResourcepacks, ec))
        return files;
    // Directory order, like File.listFiles on Windows.
    for (const auto& entry : std::filesystem::directory_iterator(dirResourcepacks, ec)) {
        const std::filesystem::path& file = entry.path();
        const std::wstring name = file.filename().wstring();
        const bool zip = entry.is_regular_file(ec) && name.size() >= 4 && name.ends_with(L".zip");
        const bool folder = entry.is_directory(ec) && std::filesystem::is_regular_file(file / "pack.mcmeta", ec);
        if (zip || folder)
            files.push_back(file);
    }
    return files;
}

void ResourcePackRepository::updateRepositoryEntriesAll()
{
    std::vector<std::shared_ptr<Entry>> list;
    const auto sameAs = [](const std::shared_ptr<Entry>& entry) {
        return [key = entry->toString()](const std::shared_ptr<Entry>& other) { return other->toString() == key; };
    };

    for (const std::filesystem::path& file : getResourcePackFiles()) {
        auto entry = std::make_shared<Entry>(file);
        const auto existing = std::find_if(repositoryEntriesAll.begin(), repositoryEntriesAll.end(), sameAs(entry));
        if (existing == repositoryEntriesAll.end()) {
            try {
                entry->updateResourcePack(*this);
                list.push_back(entry);
            } catch (const std::exception&) {
                std::erase_if(list, sameAs(entry));
            }
        } else {
            list.push_back(*existing);
        }
    }

    std::erase_if(repositoryEntriesAll, [&](const std::shared_ptr<Entry>& entry) {
        return std::any_of(list.begin(), list.end(), sameAs(entry));
    });
    for (const auto& entry : repositoryEntriesAll)
        entry->closeResourcePack();
    repositoryEntriesAll = std::move(list);
}

} // namespace mc

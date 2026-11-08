#pragma once

// Original: net/minecraft/client/resources/AbstractResourcePack.java, FileResourcePack.java,
// FolderResourcePack.java and ResourcePackFileNotFoundException.java

#include "client/resources/IResourcePack.h"

#include <filesystem>
#include <memory>
#include <mutex>

namespace mc {

class ZipArchive;

class AbstractResourcePack : public IResourcePack {
public:
    explicit AbstractResourcePack(std::filesystem::path resourcePackFile);

    std::vector<uint8_t> getInputStream(const ResourceLocation& location) const override;
    bool resourceExists(const ResourceLocation& location) const override;
    std::optional<nlohmann::json> getPackMetadata(const std::string& section) const override;
    BufferedImage getPackImage() const override;
    std::string getPackName() const override;

protected:
    // Throws ResourceNotFoundException when the entry is missing.
    virtual std::vector<uint8_t> getInputStreamByName(const std::string& name) const = 0;
    virtual bool hasResourceName(const std::string& name) const = 0;
    void logNameNotLowercase(const std::string& name) const;

    std::filesystem::path resourcePackFile;
};

class FileResourcePack : public AbstractResourcePack {
public:
    using AbstractResourcePack::AbstractResourcePack;

    std::set<std::string> getResourceDomains() const override;
    // Java: Closeable. The zip reopens on the next access.
    void close();

protected:
    std::vector<uint8_t> getInputStreamByName(const std::string& name) const override;
    bool hasResourceName(const std::string& name) const override;

private:
    std::shared_ptr<const ZipArchive> getResourcePackZipFile() const;

    mutable std::mutex mutex;
    mutable std::shared_ptr<const ZipArchive> resourcePackZipFile;
};

class FolderResourcePack : public AbstractResourcePack {
public:
    using AbstractResourcePack::AbstractResourcePack;

    std::set<std::string> getResourceDomains() const override;

protected:
    std::vector<uint8_t> getInputStreamByName(const std::string& name) const override;
    bool hasResourceName(const std::string& name) const override;
};

} // namespace mc

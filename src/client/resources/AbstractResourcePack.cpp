#include "client/resources/AbstractResourcePack.h"

#include "client/renderer/texture/TextureUtil.h"
#include "core/Log.h"
#include "core/StringUtil.h"
#include "core/ZipArchive.h"

#include <nlohmann/json.hpp>

#include <algorithm>
#include <format>
#include <fstream>
#include <iterator>

namespace mc {

namespace {

// Java: !s.equals(s.toLowerCase())
bool isLowerCase(const std::string& text)
{
    return std::none_of(text.begin(), text.end(), [](char c) { return c >= 'A' && c <= 'Z'; });
}

std::string locationToName(const ResourceLocation& location)
{
    return "assets/" + location.getResourceDomain() + "/" + location.getResourcePath();
}

} // namespace

AbstractResourcePack::AbstractResourcePack(std::filesystem::path resourcePackFileIn)
    : resourcePackFile(std::move(resourcePackFileIn))
{
}

std::vector<uint8_t> AbstractResourcePack::getInputStream(const ResourceLocation& location) const
{
    return getInputStreamByName(locationToName(location));
}

bool AbstractResourcePack::resourceExists(const ResourceLocation& location) const
{
    return hasResourceName(locationToName(location));
}

void AbstractResourcePack::logNameNotLowercase(const std::string& /*name*/) const
{
    // Java hands printf placeholders to log4j, which prints them unchanged.
    Log::warn("ResourcePack: ignored non-lowercase namespace: %s in %s");
}

std::optional<nlohmann::json> AbstractResourcePack::getPackMetadata(const std::string& section) const
{
    const std::vector<uint8_t> data = getInputStreamByName("pack.mcmeta");
    nlohmann::json root;
    try {
        // Gson's JsonParser is lenient; comments are the leniency packs rely on in practice.
        root = nlohmann::json::parse(data.begin(), data.end(), nullptr, true, true);
    } catch (const nlohmann::json::exception& e) {
        throw std::runtime_error(std::string("JsonParseException: ") + e.what());
    }
    if (!root.is_object())
        throw std::runtime_error("JsonParseException: Not a JSON Object");
    const auto found = root.find(section);
    if (found == root.end())
        return std::nullopt;
    if (!found->is_object())
        throw std::invalid_argument("Invalid metadata for '" + section + "' - expected object, found " + found->dump());
    return *found;
}

BufferedImage AbstractResourcePack::getPackImage() const
{
    return TextureUtil::readBufferedImage(getInputStreamByName("pack.png"));
}

std::string AbstractResourcePack::getPackName() const
{
    return toUtf8(resourcePackFile.filename().wstring());
}

// --- FileResourcePack ---

std::shared_ptr<const ZipArchive> FileResourcePack::getResourcePackZipFile() const
{
    const std::lock_guard lock(mutex);
    if (resourcePackZipFile == nullptr)
        resourcePackZipFile = std::make_shared<const ZipArchive>(resourcePackFile);
    return resourcePackZipFile;
}

std::vector<uint8_t> FileResourcePack::getInputStreamByName(const std::string& name) const
{
    auto data = getResourcePackZipFile()->read(name);
    if (!data)
        throw ResourceNotFoundException(std::format("'{}' in ResourcePack '{}'", name, toUtf8(resourcePackFile.wstring())));
    return std::move(*data);
}

bool FileResourcePack::hasResourceName(const std::string& name) const
{
    try {
        return getResourcePackZipFile()->contains(name);
    } catch (const std::exception&) {
        return false;
    }
}

std::set<std::string> FileResourcePack::getResourceDomains() const
{
    std::shared_ptr<const ZipArchive> zipfile;
    try {
        zipfile = getResourcePackZipFile();
    } catch (const std::exception&) {
        return {};
    }

    std::set<std::string> set;
    for (const std::string& s : zipfile->list("assets/")) {
        // Splitter.on('/').omitEmptyStrings().limit(3): the second piece is the namespace.
        std::vector<std::string> pieces;
        size_t start = 0;
        while (pieces.size() < 2) {
            const size_t slash = s.find('/', start);
            const size_t end = slash == std::string::npos ? s.size() : slash;
            if (end > start)
                pieces.push_back(s.substr(start, end - start));
            if (slash == std::string::npos)
                break;
            start = slash + 1;
        }
        if (pieces.size() > 1) {
            const std::string& s1 = pieces[1];
            if (!isLowerCase(s1))
                logNameNotLowercase(s1);
            else
                set.insert(s1);
        }
    }
    return set;
}

void FileResourcePack::close()
{
    const std::lock_guard lock(mutex);
    resourcePackZipFile.reset();
}

// --- FolderResourcePack ---

std::vector<uint8_t> FolderResourcePack::getInputStreamByName(const std::string& name) const
{
    const std::filesystem::path file = resourcePackFile / toWide(name);
    std::ifstream stream(file, std::ios::binary);
    if (!stream)
        throw ResourceNotFoundException(toUtf8(file.wstring()));
    return std::vector<uint8_t>(std::istreambuf_iterator<char>(stream), std::istreambuf_iterator<char>());
}

bool FolderResourcePack::hasResourceName(const std::string& name) const
{
    std::error_code ec;
    return std::filesystem::is_regular_file(resourcePackFile / toWide(name), ec);
}

std::set<std::string> FolderResourcePack::getResourceDomains() const
{
    std::set<std::string> set;
    const std::filesystem::path assets = resourcePackFile / "assets";
    std::error_code ec;
    if (!std::filesystem::is_directory(assets, ec))
        return set;
    for (const auto& entry : std::filesystem::directory_iterator(assets, ec)) {
        if (!entry.is_directory(ec))
            continue;
        const std::string s = toUtf8(entry.path().filename().wstring());
        if (!isLowerCase(s))
            logNameNotLowercase(s);
        else
            set.insert(s);
    }
    return set;
}

} // namespace mc

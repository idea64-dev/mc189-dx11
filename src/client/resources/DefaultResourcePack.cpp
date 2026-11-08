#include "client/resources/DefaultResourcePack.h"

#include "client/renderer/texture/TextureUtil.h"
#include "core/ZipArchive.h"

#include <fstream>
#include <iterator>

namespace mc {

DefaultResourcePack::DefaultResourcePack(std::shared_ptr<const ZipArchive> jar,
    std::unordered_map<std::string, std::filesystem::path> assets)
    : clientJar(std::move(jar))
    , mapAssets(std::move(assets))
{
}

std::vector<uint8_t> DefaultResourcePack::getInputStream(const ResourceLocation& location) const
{
    if (auto data = getResourceStream(location))
        return std::move(*data);
    if (auto data = getInputStreamAssets(location))
        return std::move(*data);
    throw ResourceNotFoundException(location.getResourcePath());
}

std::optional<std::vector<uint8_t>> DefaultResourcePack::getInputStreamAssets(const ResourceLocation& location) const
{
    const auto it = mapAssets.find(location.toString());
    if (it == mapAssets.end())
        return std::nullopt;

    std::error_code ec;
    if (!std::filesystem::is_regular_file(it->second, ec))
        return std::nullopt;

    std::ifstream stream(it->second, std::ios::binary);
    if (!stream)
        return std::nullopt;
    return std::vector<uint8_t>(std::istreambuf_iterator<char>(stream), std::istreambuf_iterator<char>());
}

std::optional<std::vector<uint8_t>> DefaultResourcePack::getResourceStream(const ResourceLocation& location) const
{
    return clientJar->read("assets/" + location.getResourceDomain() + "/" + location.getResourcePath());
}

std::optional<nlohmann::json> DefaultResourcePack::getPackMetadata(const std::string& section) const
{
    const auto it = mapAssets.find("pack.mcmeta");
    if (it == mapAssets.end())
        return std::nullopt;
    std::ifstream stream(it->second, std::ios::binary);
    if (!stream)
        return std::nullopt;
    try {
        const nlohmann::json root = nlohmann::json::parse(stream, nullptr, true, true);
        if (!root.is_object())
            return std::nullopt;
        const auto found = root.find(section);
        if (found == root.end() || !found->is_object())
            return std::nullopt;
        return *found;
    } catch (const nlohmann::json::exception&) {
        return std::nullopt; // Java: RuntimeException -> null
    }
}

BufferedImage DefaultResourcePack::getPackImage() const
{
    const auto data = clientJar->read("pack.png");
    if (!data)
        throw ResourceNotFoundException("pack.png");
    return TextureUtil::readBufferedImage(*data);
}

bool DefaultResourcePack::resourceExists(const ResourceLocation& location) const
{
    return clientJar->contains("assets/" + location.getResourceDomain() + "/" + location.getResourcePath())
        || mapAssets.contains(location.toString());
}

} // namespace mc

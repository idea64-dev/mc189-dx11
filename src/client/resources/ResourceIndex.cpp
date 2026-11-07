#include "client/resources/ResourceIndex.h"

#include "core/Log.h"
#include "core/StringUtil.h"

#include <nlohmann/json.hpp>

#include <fstream>

namespace mc {

ResourceIndex::ResourceIndex(const std::filesystem::path& assetsDir, const std::string& indexName)
{
    if (indexName.empty())
        return;

    const std::filesystem::path objectsDir = assetsDir / "objects";
    const std::filesystem::path indexFile = assetsDir / "indexes" / (indexName + ".json");

    std::ifstream stream(indexFile, std::ios::binary);
    if (!stream) {
        Log::error("Can't find the resource index file: {}", toUtf8(indexFile.wstring()));
        return;
    }

    try {
        const nlohmann::json root = nlohmann::json::parse(stream);
        const auto objects = root.find("objects");
        if (objects == root.end() || !objects->is_object())
            return;

        for (const auto& [key, value] : objects->items()) {
            // "minecraft/sounds/x.ogg" -> "minecraft:sounds/x.ogg"; keys without '/' stay as they are.
            const size_t slash = key.find('/');
            std::string name = slash == std::string::npos ? key : key.substr(0, slash) + ':' + key.substr(slash + 1);

            const std::string hash = value.at("hash").get<std::string>();
            resourceMap.emplace(std::move(name), objectsDir / hash.substr(0, 2) / hash);
        }
    } catch (const nlohmann::json::exception&) {
        Log::error("Unable to parse resource index file: {}", toUtf8(indexFile.wstring()));
    }
}

} // namespace mc

#pragma once

// Original: net/minecraft/client/resources/ResourceIndex.java
// Maps "domain:path" to files in assets/objects using the launcher's asset index (sounds, languages, icons).

#include <filesystem>
#include <string>
#include <unordered_map>

namespace mc {

class ResourceIndex {
public:
    ResourceIndex() = default;
    // Reads <assetsDir>/indexes/<indexName>.json. Errors are logged, leaving the map empty, as in Java.
    ResourceIndex(const std::filesystem::path& assetsDir, const std::string& indexName);

    const std::unordered_map<std::string, std::filesystem::path>& getResourceMap() const { return resourceMap; }

private:
    std::unordered_map<std::string, std::filesystem::path> resourceMap;
};

} // namespace mc

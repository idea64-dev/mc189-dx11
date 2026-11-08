#include "world/WorldType.h"

#include <cctype>

namespace mc {

std::array<const WorldType*, 16> WorldType::worldTypes {};

WorldType::WorldType(int id, std::string name, int version, bool versioned, bool notification, bool canBeCreatedIn)
    : worldTypeId(id)
    , worldType(std::move(name))
    , generatorVersion(version)
    , canBeCreated(canBeCreatedIn)
    , isWorldTypeVersioned(versioned)
    , hasNotificationData(notification)
{
    worldTypes[static_cast<size_t>(id)] = this;
}

// In the order of the Java static fields; worldTypes is constant-initialized before them.
const WorldType WorldType::DEFAULT(0, "default", 1, true, false, true);
const WorldType WorldType::FLAT(1, "flat", 0, false, false, true);
const WorldType WorldType::LARGE_BIOMES(2, "largeBiomes", 0, false, false, true);
const WorldType WorldType::AMPLIFIED(3, "amplified", 0, false, true, true);
const WorldType WorldType::CUSTOMIZED(4, "customized", 0, false, false, true);
const WorldType WorldType::DEBUG_WORLD(5, "debug_all_block_states", 0, false, false, true);
const WorldType WorldType::DEFAULT_1_1(8, "default_1_1", 0, false, false, false);

const WorldType* WorldType::parseWorldType(const std::string& type)
{
    // String.equalsIgnoreCase; the names are ASCII.
    const auto equalsIgnoreCase = [](const std::string& a, const std::string& b) {
        if (a.size() != b.size())
            return false;
        for (size_t i = 0; i < a.size(); ++i) {
            if (std::tolower(static_cast<unsigned char>(a[i])) != std::tolower(static_cast<unsigned char>(b[i])))
                return false;
        }
        return true;
    };
    for (const WorldType* worldType : worldTypes) {
        if (worldType && equalsIgnoreCase(worldType->worldType, type))
            return worldType;
    }
    return nullptr;
}

} // namespace mc

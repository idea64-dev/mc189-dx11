#pragma once

// Original: net/minecraft/world/WorldType.java

#include <array>
#include <string>

namespace mc {

class WorldType {
public:
    static std::array<const WorldType*, 16> worldTypes;
    static const WorldType DEFAULT, FLAT, LARGE_BIOMES, AMPLIFIED, CUSTOMIZED, DEBUG_WORLD, DEFAULT_1_1;

    const std::string& getWorldTypeName() const { return worldType; }
    std::string getTranslateName() const { return "generator." + worldType; }
    std::string getTranslatedInfo() const { return getTranslateName() + ".info"; }
    int getGeneratorVersion() const { return generatorVersion; }
    const WorldType* getWorldTypeForGeneratorVersion(int version) const { return this == &DEFAULT && version == 0 ? &DEFAULT_1_1 : this; }
    bool getCanBeCreated() const { return canBeCreated; }
    bool isVersioned() const { return isWorldTypeVersioned; }
    // Case-insensitive; null when unknown.
    static const WorldType* parseWorldType(const std::string& type);
    int getWorldTypeID() const { return worldTypeId; }
    bool showWorldInfoNotice() const { return hasNotificationData; }

    WorldType(const WorldType&) = delete;
    WorldType& operator=(const WorldType&) = delete;

private:
    WorldType(int id, std::string name, int version, bool versioned, bool notification, bool canBeCreatedIn);

    int worldTypeId;
    std::string worldType;
    int generatorVersion;
    bool canBeCreated;
    bool isWorldTypeVersioned;
    bool hasNotificationData;
};

} // namespace mc

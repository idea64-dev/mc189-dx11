#pragma once

// Original: net/minecraft/world/WorldSettings.java (with WorldSettings.GameType)
// GameType.configurePlayerCapabilities comes with the player (stage 8).

#include "world/WorldType.h"

#include <cstdint>
#include <string>
#include <string_view>

namespace mc {

class WorldInfo;

enum class GameType { NOT_SET, SURVIVAL, CREATIVE, ADVENTURE, SPECTATOR };

constexpr int getGameTypeID(GameType type)
{
    return static_cast<int>(type) - 1;
}

constexpr std::string_view getGameTypeName(GameType type)
{
    constexpr std::string_view names[5] { "", "survival", "creative", "adventure", "spectator" };
    return names[static_cast<int>(type)];
}

constexpr bool isAdventure(GameType type)
{
    return type == GameType::ADVENTURE || type == GameType::SPECTATOR;
}

constexpr bool isCreative(GameType type)
{
    return type == GameType::CREATIVE;
}

constexpr bool isSurvivalOrAdventure(GameType type)
{
    return type == GameType::SURVIVAL || type == GameType::ADVENTURE;
}

// SURVIVAL for unknown ids and names.
constexpr GameType getGameTypeByID(int id)
{
    return id >= -1 && id <= 3 ? static_cast<GameType>(id + 1) : GameType::SURVIVAL;
}

constexpr GameType getGameTypeByName(std::string_view name)
{
    for (int i = 0; i < 5; ++i) {
        if (getGameTypeName(static_cast<GameType>(i)) == name)
            return static_cast<GameType>(i);
    }
    return GameType::SURVIVAL;
}

class WorldSettings {
public:
    WorldSettings(int64_t seedIn, GameType gameType, bool enableMapFeatures, bool hardcoreMode, const WorldType* worldTypeIn)
        : seed(seedIn)
        , theGameType(gameType)
        , mapFeaturesEnabled(enableMapFeatures)
        , hardcoreEnabled(hardcoreMode)
        , terrainType(worldTypeIn)
    {
    }
    explicit WorldSettings(const WorldInfo& info);

    WorldSettings& enableBonusChest()
    {
        bonusChestEnabled = true;
        return *this;
    }
    WorldSettings& enableCommands()
    {
        commandsAllowed = true;
        return *this;
    }
    // Java: setWorldName -- despite the name, the generator options.
    WorldSettings& setWorldName(std::string name)
    {
        worldName = std::move(name);
        return *this;
    }
    bool isBonusChestEnabled() const { return bonusChestEnabled; }
    int64_t getSeed() const { return seed; }
    GameType getGameType() const { return theGameType; }
    bool getHardcoreEnabled() const { return hardcoreEnabled; }
    bool isMapFeaturesEnabled() const { return mapFeaturesEnabled; }
    const WorldType* getTerrainType() const { return terrainType; }
    bool areCommandsAllowed() const { return commandsAllowed; }
    const std::string& getWorldName() const { return worldName; }

private:
    int64_t seed;
    GameType theGameType;
    bool mapFeaturesEnabled;
    bool hardcoreEnabled;
    const WorldType* terrainType;
    bool commandsAllowed = false;
    bool bonusChestEnabled = false;
    std::string worldName;
};

} // namespace mc

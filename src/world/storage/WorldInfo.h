#pragma once

// Original: net/minecraft/world/storage/WorldInfo.java -- what level.dat holds.
// addToCrashReport comes with crash reports (stage 15).

#include "util/BlockPos.h"
#include "world/EnumDifficulty.h"
#include "world/GameRules.h"
#include "world/WorldSettings.h"

#include <cstdint>
#include <memory>
#include <optional>
#include <string>

namespace mc {

class NBTTagCompound;

class WorldInfo {
public:
    static constexpr EnumDifficulty DEFAULT_DIFFICULTY = EnumDifficulty::NORMAL;

    explicit WorldInfo(const NBTTagCompound& nbt);
    WorldInfo(const WorldSettings& settings, const std::string& name);
    // Java: WorldInfo(WorldInfo) shares the game rules and skips clearWeatherTime -- with the derived worlds of
    // other dimensions (stage 9).
    WorldInfo(const WorldInfo&) = delete;
    WorldInfo& operator=(const WorldInfo&) = delete;

    void populateFromWorldSettings(const WorldSettings& settings);
    std::shared_ptr<NBTTagCompound> getNBTTagCompound() const;
    std::shared_ptr<NBTTagCompound> cloneNBTCompound(const std::shared_ptr<NBTTagCompound>& nbt) const;

    int64_t getSeed() const { return randomSeed; }
    int getSpawnX() const { return spawnX; }
    int getSpawnY() const { return spawnY; }
    int getSpawnZ() const { return spawnZ; }
    int64_t getWorldTotalTime() const { return totalTime; }
    int64_t getWorldTime() const { return worldTime; }
    int64_t getSizeOnDisk() const { return sizeOnDisk; }
    const std::shared_ptr<NBTTagCompound>& getPlayerNBTTagCompound() const { return playerTag; }
    void setSpawnX(int x) { spawnX = x; }
    void setSpawnY(int y) { spawnY = y; }
    void setSpawnZ(int z) { spawnZ = z; }
    void setWorldTotalTime(int64_t time) { totalTime = time; }
    void setWorldTime(int64_t time) { worldTime = time; }
    void setSpawn(const BlockPos& spawnPoint)
    {
        spawnX = spawnPoint.getX();
        spawnY = spawnPoint.getY();
        spawnZ = spawnPoint.getZ();
    }
    const std::string& getWorldName() const { return levelName; }
    void setWorldName(const std::string& worldName) { levelName = worldName; }
    int getSaveVersion() const { return saveVersion; }
    void setSaveVersion(int version) { saveVersion = version; }
    int64_t getLastTimePlayed() const { return lastTimePlayed; }
    int getCleanWeatherTime() const { return cleanWeatherTime; }
    void setCleanWeatherTime(int time) { cleanWeatherTime = time; }
    bool isThundering() const { return thundering; }
    void setThundering(bool value) { thundering = value; }
    int getThunderTime() const { return thunderTime; }
    void setThunderTime(int time) { thunderTime = time; }
    bool isRaining() const { return raining; }
    void setRaining(bool value) { raining = value; }
    int getRainTime() const { return rainTime; }
    void setRainTime(int time) { rainTime = time; }
    GameType getGameType() const { return theGameType; }
    bool isMapFeaturesEnabled() const { return mapFeaturesEnabled; }
    void setMapFeaturesEnabled(bool enabled) { mapFeaturesEnabled = enabled; }
    void setGameType(GameType type) { theGameType = type; }
    bool isHardcoreModeEnabled() const { return hardcore; }
    void setHardcore(bool value) { hardcore = value; }
    const WorldType* getTerrainType() const { return terrainType; }
    void setTerrainType(const WorldType* type) { terrainType = type; }
    const std::string& getGeneratorOptions() const { return generatorOptions; }
    bool areCommandsAllowed() const { return allowCommands; }
    void setAllowCommands(bool allow) { allowCommands = allow; }
    bool isInitialized() const { return initialized; }
    void setServerInitialized(bool value) { initialized = value; }
    GameRules& getGameRulesInstance() { return theGameRules; }
    const GameRules& getGameRulesInstance() const { return theGameRules; }
    double getBorderCenterX() const { return borderCenterX; }
    double getBorderCenterZ() const { return borderCenterZ; }
    double getBorderSize() const { return borderSize; }
    void setBorderSize(double size) { borderSize = size; }
    int64_t getBorderLerpTime() const { return borderSizeLerpTime; }
    void setBorderLerpTime(int64_t time) { borderSizeLerpTime = time; }
    double getBorderLerpTarget() const { return borderSizeLerpTarget; }
    void setBorderLerpTarget(double lerpSize) { borderSizeLerpTarget = lerpSize; }
    // Java: the setters are named getBorderCenterZ(double) / getBorderCenterX(double).
    void setBorderCenterZ(double posZ) { borderCenterZ = posZ; }
    void setBorderCenterX(double posX) { borderCenterX = posX; }
    double getBorderSafeZone() const { return borderSafeZone; }
    void setBorderSafeZone(double amount) { borderSafeZone = amount; }
    double getBorderDamagePerBlock() const { return borderDamagePerBlock; }
    void setBorderDamagePerBlock(double damage) { borderDamagePerBlock = damage; }
    int getBorderWarningDistance() const { return borderWarningDistance; }
    int getBorderWarningTime() const { return borderWarningTime; }
    void setBorderWarningDistance(int blocks) { borderWarningDistance = blocks; }
    void setBorderWarningTime(int ticks) { borderWarningTime = ticks; }
    // Java's null is nullopt (old level.dat files have no difficulty).
    std::optional<EnumDifficulty> getDifficulty() const { return difficulty; }
    void setDifficulty(EnumDifficulty newDifficulty) { difficulty = newDifficulty; }
    bool isDifficultyLocked() const { return difficultyLocked; }
    void setDifficultyLocked(bool locked) { difficultyLocked = locked; }

private:
    void updateTagCompound(NBTTagCompound& nbt, const std::shared_ptr<NBTTagCompound>& playerNbt) const;

    int64_t randomSeed = 0;
    const WorldType* terrainType = &WorldType::DEFAULT;
    std::string generatorOptions;
    int spawnX = 0;
    int spawnY = 0;
    int spawnZ = 0;
    int64_t totalTime = 0;
    int64_t worldTime = 0;
    int64_t lastTimePlayed = 0;
    int64_t sizeOnDisk = 0;
    std::shared_ptr<NBTTagCompound> playerTag;
    int dimension = 0;
    std::string levelName;
    int saveVersion = 0;
    int cleanWeatherTime = 0;
    bool raining = false;
    int rainTime = 0;
    bool thundering = false;
    int thunderTime = 0;
    GameType theGameType = GameType::NOT_SET;
    bool mapFeaturesEnabled = false;
    bool hardcore = false;
    bool allowCommands = false;
    bool initialized = false;
    std::optional<EnumDifficulty> difficulty;
    bool difficultyLocked = false;
    double borderCenterX = 0.0;
    double borderCenterZ = 0.0;
    double borderSize = 6.0E7;
    int64_t borderSizeLerpTime = 0;
    double borderSizeLerpTarget = 0.0;
    double borderSafeZone = 5.0;
    double borderDamagePerBlock = 0.2;
    int borderWarningDistance = 5;
    int borderWarningTime = 15;
    GameRules theGameRules;
};

} // namespace mc

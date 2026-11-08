#include "world/storage/WorldInfo.h"

#include "core/JavaTime.h"
#include "nbt/NBTTagCompound.h"

namespace mc {

WorldInfo::WorldInfo(const NBTTagCompound& nbt)
{
    randomSeed = nbt.getLong("RandomSeed");
    if (nbt.hasKey("generatorName", 8)) {
        const std::string s = nbt.getString("generatorName");
        terrainType = WorldType::parseWorldType(s);
        if (!terrainType) {
            terrainType = &WorldType::DEFAULT;
        } else if (terrainType->isVersioned()) {
            int i = 0;
            if (nbt.hasKey("generatorVersion", 99))
                i = nbt.getInteger("generatorVersion");
            terrainType = terrainType->getWorldTypeForGeneratorVersion(i);
        }
        if (nbt.hasKey("generatorOptions", 8))
            generatorOptions = nbt.getString("generatorOptions");
    }
    theGameType = getGameTypeByID(nbt.getInteger("GameType"));
    mapFeaturesEnabled = nbt.hasKey("MapFeatures", 99) ? nbt.getBoolean("MapFeatures") : true;
    spawnX = nbt.getInteger("SpawnX");
    spawnY = nbt.getInteger("SpawnY");
    spawnZ = nbt.getInteger("SpawnZ");
    totalTime = nbt.getLong("Time");
    worldTime = nbt.hasKey("DayTime", 99) ? nbt.getLong("DayTime") : totalTime;
    lastTimePlayed = nbt.getLong("LastPlayed");
    sizeOnDisk = nbt.getLong("SizeOnDisk");
    levelName = nbt.getString("LevelName");
    saveVersion = nbt.getInteger("version");
    cleanWeatherTime = nbt.getInteger("clearWeatherTime");
    rainTime = nbt.getInteger("rainTime");
    raining = nbt.getBoolean("raining");
    thunderTime = nbt.getInteger("thunderTime");
    thundering = nbt.getBoolean("thundering");
    hardcore = nbt.getBoolean("hardcore");
    initialized = nbt.hasKey("initialized", 99) ? nbt.getBoolean("initialized") : true;
    allowCommands = nbt.hasKey("allowCommands", 99) ? nbt.getBoolean("allowCommands") : theGameType == GameType::CREATIVE;
    if (nbt.hasKey("Player", 10)) {
        playerTag = nbt.getCompoundTag("Player");
        dimension = playerTag->getInteger("Dimension");
    }
    if (nbt.hasKey("GameRules", 10))
        theGameRules.readFromNBT(*nbt.getCompoundTag("GameRules"));
    if (nbt.hasKey("Difficulty", 99))
        difficulty = getDifficultyEnum(nbt.getByte("Difficulty"));
    if (nbt.hasKey("DifficultyLocked", 1))
        difficultyLocked = nbt.getBoolean("DifficultyLocked");
    if (nbt.hasKey("BorderCenterX", 99))
        borderCenterX = nbt.getDouble("BorderCenterX");
    if (nbt.hasKey("BorderCenterZ", 99))
        borderCenterZ = nbt.getDouble("BorderCenterZ");
    if (nbt.hasKey("BorderSize", 99))
        borderSize = nbt.getDouble("BorderSize");
    if (nbt.hasKey("BorderSizeLerpTime", 99))
        borderSizeLerpTime = nbt.getLong("BorderSizeLerpTime");
    if (nbt.hasKey("BorderSizeLerpTarget", 99))
        borderSizeLerpTarget = nbt.getDouble("BorderSizeLerpTarget");
    if (nbt.hasKey("BorderSafeZone", 99))
        borderSafeZone = nbt.getDouble("BorderSafeZone");
    if (nbt.hasKey("BorderDamagePerBlock", 99))
        borderDamagePerBlock = nbt.getDouble("BorderDamagePerBlock");
    if (nbt.hasKey("BorderWarningBlocks", 99))
        borderWarningDistance = nbt.getInteger("BorderWarningBlocks");
    if (nbt.hasKey("BorderWarningTime", 99))
        borderWarningTime = nbt.getInteger("BorderWarningTime");
}

WorldInfo::WorldInfo(const WorldSettings& settings, const std::string& name)
{
    populateFromWorldSettings(settings);
    levelName = name;
    difficulty = DEFAULT_DIFFICULTY;
    initialized = false;
}

void WorldInfo::populateFromWorldSettings(const WorldSettings& settings)
{
    randomSeed = settings.getSeed();
    theGameType = settings.getGameType();
    mapFeaturesEnabled = settings.isMapFeaturesEnabled();
    hardcore = settings.getHardcoreEnabled();
    terrainType = settings.getTerrainType();
    generatorOptions = settings.getWorldName();
    allowCommands = settings.areCommandsAllowed();
}

std::shared_ptr<NBTTagCompound> WorldInfo::getNBTTagCompound() const
{
    auto nbttagcompound = std::make_shared<NBTTagCompound>();
    updateTagCompound(*nbttagcompound, playerTag);
    return nbttagcompound;
}

std::shared_ptr<NBTTagCompound> WorldInfo::cloneNBTCompound(const std::shared_ptr<NBTTagCompound>& nbt) const
{
    auto nbttagcompound = std::make_shared<NBTTagCompound>();
    updateTagCompound(*nbttagcompound, nbt);
    return nbttagcompound;
}

void WorldInfo::updateTagCompound(NBTTagCompound& nbt, const std::shared_ptr<NBTTagCompound>& playerNbt) const
{
    nbt.setLong("RandomSeed", randomSeed);
    nbt.setString("generatorName", terrainType->getWorldTypeName());
    nbt.setInteger("generatorVersion", terrainType->getGeneratorVersion());
    nbt.setString("generatorOptions", generatorOptions);
    nbt.setInteger("GameType", getGameTypeID(theGameType));
    nbt.setBoolean("MapFeatures", mapFeaturesEnabled);
    nbt.setInteger("SpawnX", spawnX);
    nbt.setInteger("SpawnY", spawnY);
    nbt.setInteger("SpawnZ", spawnZ);
    nbt.setLong("Time", totalTime);
    nbt.setLong("DayTime", worldTime);
    nbt.setLong("SizeOnDisk", sizeOnDisk);
    nbt.setLong("LastPlayed", java::currentTimeMillis());
    nbt.setString("LevelName", levelName);
    nbt.setInteger("version", saveVersion);
    nbt.setInteger("clearWeatherTime", cleanWeatherTime);
    nbt.setInteger("rainTime", rainTime);
    nbt.setBoolean("raining", raining);
    nbt.setInteger("thunderTime", thunderTime);
    nbt.setBoolean("thundering", thundering);
    nbt.setBoolean("hardcore", hardcore);
    nbt.setBoolean("allowCommands", allowCommands);
    nbt.setBoolean("initialized", initialized);
    nbt.setDouble("BorderCenterX", borderCenterX);
    nbt.setDouble("BorderCenterZ", borderCenterZ);
    nbt.setDouble("BorderSize", borderSize);
    nbt.setLong("BorderSizeLerpTime", borderSizeLerpTime);
    nbt.setDouble("BorderSafeZone", borderSafeZone);
    nbt.setDouble("BorderDamagePerBlock", borderDamagePerBlock);
    nbt.setDouble("BorderSizeLerpTarget", borderSizeLerpTarget);
    nbt.setDouble("BorderWarningBlocks", static_cast<double>(borderWarningDistance));
    nbt.setDouble("BorderWarningTime", static_cast<double>(borderWarningTime));
    if (difficulty)
        nbt.setByte("Difficulty", static_cast<int8_t>(getDifficultyId(*difficulty)));
    nbt.setBoolean("DifficultyLocked", difficultyLocked);
    nbt.setTag("GameRules", theGameRules.writeToNBT());
    if (playerNbt)
        nbt.setTag("Player", playerNbt);
}

} // namespace mc

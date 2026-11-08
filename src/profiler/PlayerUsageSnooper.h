#pragma once

// Original: net/minecraft/profiler/PlayerUsageSnooper.java and IPlayerUsage.java
// Collects the statistics shown on the Snooper Settings screen. Nothing is sent: snoop.minecraft.net is gone,
// so the 15-minute upload timer is dropped. Values are stored as the strings Java's toString() would give.

#include <cstdint>
#include <map>
#include <string>

namespace mc {

class PlayerUsageSnooper;

class IPlayerUsage {
public:
    virtual ~IPlayerUsage() = default;
    virtual void addServerStatsToSnooper(PlayerUsageSnooper& playerSnooper) = 0;
    virtual void addServerTypeToSnooper(PlayerUsageSnooper& playerSnooper) = 0;
    virtual bool isSnooperEnabled() const = 0;
};

class PlayerUsageSnooper {
public:
    PlayerUsageSnooper(std::string side, IPlayerUsage& playerStatCollector, int64_t startTime);

    void startSnooper();
    void addMemoryStatsToSnooper();
    void addClientStat(const std::string& statName, std::string statValue);
    void addStatToSnooper(const std::string& statName, std::string statValue);
    // Java returns a LinkedHashMap; its only reader (GuiSnooper) sorts it, so a sorted map is equivalent.
    std::map<std::string, std::string> getCurrentStats();
    bool isSnooperRunning() const { return isRunning; }
    void stopSnooper() { }
    const std::string& getUniqueID() const { return uniqueID; }
    int64_t getMinecraftStartTimeMillis() const { return minecraftStartTimeMilis; }

private:
    void addOSData();

    std::map<std::string, std::string> snooperStats;
    std::map<std::string, std::string> clientStats;
    std::string uniqueID;
    std::string side;
    IPlayerUsage& playerStatsCollector;
    int64_t minecraftStartTimeMilis;
    bool isRunning = false;
};

} // namespace mc

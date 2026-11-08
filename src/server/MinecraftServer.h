#pragma once

// Original: net/minecraft/server/MinecraftServer.java and network/NetworkSystem.java (the local endpoint)
// The server thread: loads the worlds, prepares the spawn area, then ticks 20 times a second.
// Not ported yet: the commands (stage 12), the status response, LAN and remote endpoints (stage 14), the
// server's usage snooper and crash reports.

#include "profiler/Profiler.h"
#include "util/Random.h"
#include "world/EnumDifficulty.h"
#include "world/WorldSettings.h"

#include <array>
#include <atomic>
#include <cstdint>
#include <deque>
#include <filesystem>
#include <functional>
#include <memory>
#include <mutex>
#include <optional>
#include <string>
#include <thread>
#include <vector>

namespace mc {

class EntityPlayer;
class IChatComponent;
class ISaveFormat;
class NetworkManager;
class ServerConfigurationManager;
class World;
class WorldServer;
class WorldType;
struct BlockPos;

class MinecraftServer {
public:
    explicit MinecraftServer(const std::filesystem::path& savesDirectory);
    virtual ~MinecraftServer();

    MinecraftServer(const MinecraftServer&) = delete;
    MinecraftServer& operator=(const MinecraftServer&) = delete;

    static int64_t getCurrentTimeMillis();

    // Java: startServerThread -- starts the "Server thread".
    void startServerThread();
    // Waits for the server thread to end.
    void joinServerThread();
    virtual void initiateShutdown() { serverRunning = false; }
    bool isServerRunning() const { return serverRunning; }
    bool serverIsInRunLoop() const { return serverIsRunning; }
    bool isServerStopped() const { return serverStopped; }
    // Java: getUserMessage -- the translation key of the loading step, null when none.
    std::optional<std::string> getUserMessage();
    std::optional<std::string> getCurrentTask() const { return currentTask; }
    int getPercentDone() const { return percentDone; }

    // Runs the task on the server thread (at once when called from it).
    void addScheduledTask(std::function<void()> task);
    // Java: Futures.getUnchecked(addScheduledTask(task)) -- waits until the server thread ran it.
    void runOnServerThreadAndWait(std::function<void()> task);
    bool isCallingFromMinecraftThread() const { return std::this_thread::get_id() == serverThreadId; }

    // Java: NetworkSystem.addLocalEndpoint + NetworkManager.provideLocalClient -- the client end of a new local
    // connection.
    std::shared_ptr<NetworkManager> addLocalEndpoint();

    WorldServer* worldServerForDimension(int dimension) const;
    ServerConfigurationManager* getConfigurationManager() const { return serverConfigManager.get(); }
    void setConfigManager(std::unique_ptr<ServerConfigurationManager> configManager);

    virtual void setDifficultyForAllWorlds(EnumDifficulty difficulty);
    virtual GameType getGameType() const = 0;
    virtual bool canStructuresSpawn() const = 0;
    virtual bool isHardcore() const = 0;
    virtual bool isDedicatedServer() const = 0;
    virtual int getOpPermissionLevel() const { return 4; }
    virtual bool isCommandBlockEnabled() const { return false; }
    bool isSinglePlayer() const { return serverOwner.has_value(); }
    const std::string& getServerOwner() const { return *serverOwner; }
    void setServerOwner(std::string owner) { serverOwner = std::move(owner); }
    const std::string& getFolderName() const { return folderName; }
    void setFolderName(std::string name) { folderName = std::move(name); }
    const std::string& getWorldName() const { return worldName; }
    void setWorldName(std::string name) { worldName = std::move(name); }
    std::string getServerModName() const { return "vanilla"; }
    bool isBlockProtected(World*, const BlockPos&, EntityPlayer*) { return false; }
    int getMaxWorldSize() const { return 29999984; }
    virtual int getSpawnProtectionSize() const { return 16; }
    // Java: getForceGamemode -- server.properties' force-gamemode; off for the integrated server.
    virtual bool getForceGamemode() const { return false; }
    int getBuildLimit() const { return buildLimit; }
    void setBuildLimit(int limit) { buildLimit = limit; }
    bool getCanSpawnAnimals() const { return canSpawnAnimals; }
    void setCanSpawnAnimals(bool value) { canSpawnAnimals = value; }
    bool getCanSpawnNPCs() const { return canSpawnNPCs; }
    void setCanSpawnNPCs(bool value) { canSpawnNPCs = value; }
    bool isPVPEnabled() const { return pvpEnabled; }
    void setAllowPvp(bool value) { pvpEnabled = value; }
    bool isFlightAllowed() const { return allowFlight; }
    void setAllowFlight(bool value) { allowFlight = value; }
    void setOnlineMode(bool value) { onlineMode = value; }
    bool isServerInOnlineMode() const { return onlineMode; }
    const std::string& getMOTD() const { return motd; }
    void setMOTD(std::string value) { motd = std::move(value); }
    const std::string& getResourcePackUrl() const { return resourcePackUrl; }
    const std::string& getResourcePackHash() const { return resourcePackHash; }
    int getMaxPlayerIdleMinutes() const { return maxPlayerIdleMinutes; }
    void canCreateBonusChest(bool enable) { enableBonusChest = enable; }
    bool getAllowNether() const { return true; }
    int getTickCounter() const { return tickCounter; }
    // Java: logs the chat line (the server console).
    void addChatMessage(const std::shared_ptr<IChatComponent>& message);
    // Java: deleteWorldAndStopServer -- the hardcore game over.
    void deleteWorldAndStopServer();

    std::vector<std::unique_ptr<WorldServer>> worldServers;
    Profiler theProfiler;
    // The tick times of the last 100 ticks, in nanoseconds.
    std::array<int64_t, 100> tickTimeArray {};

protected:
    virtual bool startServer() = 0;
    virtual void tick();
    virtual void stopServer();
    virtual void finalTick() { }
    void convertMapIfNeeded(const std::string& worldNameIn);
    void setUserMessage(std::string message);
    void initialWorldChunkLoad();
    void outputPercentRemaining(const std::string& message, int percent);
    void clearCurrentTask();
    void saveAllWorlds(bool dontLog);
    void updateTimeLightAndEntities();
    void runScheduledTasks();
    ISaveFormat& getActiveAnvilConverter() { return *anvilConverterForAnvilFile; }

    std::atomic<bool> serverRunning { true };
    std::atomic<bool> serverIsRunning { false };
    std::atomic<bool> serverStopped { false };
    int tickCounter = 0;
    bool enableBonusChest = false;
    bool worldIsBeingDeleted = false;

private:
    void run();
    // Java: NetworkSystem.networkTick
    void networkTick();
    void terminateEndpoints();

    std::unique_ptr<ISaveFormat> anvilConverterForAnvilFile;
    std::unique_ptr<ServerConfigurationManager> serverConfigManager;
    std::thread serverThread;
    std::thread::id serverThreadId;
    std::mutex futureTaskMutex;
    std::deque<std::function<void()>> futureTaskQueue;
    std::mutex networkMutex;
    std::vector<std::shared_ptr<NetworkManager>> networkManagers;
    std::mutex userMessageMutex;
    std::optional<std::string> userMessage;
    std::optional<std::string> currentTask;
    int percentDone = 0;
    std::optional<std::string> serverOwner;
    std::string folderName;
    std::string worldName;
    int buildLimit = 256;
    bool canSpawnAnimals = false;
    bool canSpawnNPCs = false;
    bool pvpEnabled = false;
    bool allowFlight = false;
    bool onlineMode = false;
    std::string motd;
    std::string resourcePackUrl;
    std::string resourcePackHash;
    int maxPlayerIdleMinutes = 0;
    int64_t currentTime = 0;
    int64_t timeOfLastWarning = 0;
    Random random;
};

} // namespace mc

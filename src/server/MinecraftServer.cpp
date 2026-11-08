#include "server/MinecraftServer.h"

#include "core/JavaTime.h"
#include "core/Log.h"
#include "entity/EntityTracker.h"
#include "network/NetworkManager.h"
#include "network/play/ServerPlayPackets.h"
#include "server/management/ServerConfigurationManager.h"
#include "server/network/NetHandlerLoginServer.h"
#include "util/ChatComponent.h"
#include "util/IProgressUpdate.h"
#include "world/GameRules.h"
#include "world/WorldProvider.h"
#include "world/WorldServer.h"
#include "world/gen/ChunkProviderServer.h"
#include "world/storage/ISaveHandler.h"
#include "world/storage/SaveFormat.h"
#include "world/storage/WorldInfo.h"

#include <algorithm>
#include <chrono>
#include <future>

namespace mc {

MinecraftServer::MinecraftServer(const std::filesystem::path& savesDirectory)
    : anvilConverterForAnvilFile(std::make_unique<AnvilSaveConverter>(savesDirectory))
{
}

MinecraftServer::~MinecraftServer()
{
    joinServerThread();
}

int64_t MinecraftServer::getCurrentTimeMillis()
{
    return java::currentTimeMillis();
}

void MinecraftServer::startServerThread()
{
    serverThread = std::thread([this] { run(); });
}

void MinecraftServer::joinServerThread()
{
    if (serverThread.joinable() && std::this_thread::get_id() != serverThread.get_id())
        serverThread.join();
}

std::optional<std::string> MinecraftServer::getUserMessage()
{
    std::lock_guard lock(userMessageMutex);
    return userMessage;
}

void MinecraftServer::setUserMessage(std::string message)
{
    std::lock_guard lock(userMessageMutex);
    userMessage = std::move(message);
}

void MinecraftServer::addScheduledTask(std::function<void()> task)
{
    // Java: callFromMainThread runs the task at once on the server thread (or once the server stopped).
    if (isCallingFromMinecraftThread() || isServerStopped()) {
        task();
        return;
    }
    std::lock_guard lock(futureTaskMutex);
    futureTaskQueue.push_back(std::move(task));
}

void MinecraftServer::runOnServerThreadAndWait(std::function<void()> task)
{
    if (isCallingFromMinecraftThread() || isServerStopped() || !serverThread.joinable()) {
        task();
        return;
    }
    auto done = std::make_shared<std::promise<void>>();
    std::future<void> finished = done->get_future();
    addScheduledTask([task = std::move(task), done] {
        task();
        done->set_value();
    });
    // The server may stop before it gets to the task; it then runs the queue no more.
    while (finished.wait_for(std::chrono::milliseconds(10)) != std::future_status::ready) {
        if (isServerStopped())
            return;
    }
}

void MinecraftServer::runScheduledTasks()
{
    for (;;) {
        std::function<void()> task;
        {
            std::lock_guard lock(futureTaskMutex);
            if (futureTaskQueue.empty())
                break;
            task = std::move(futureTaskQueue.front());
            futureTaskQueue.pop_front();
        }
        try {
            task();
        } catch (const std::exception& e) {
            Log::fatal("Error executing task: {}", e.what());
        }
    }
    // The packets of the connections (Java: the play handlers' tasks queued from the Netty threads).
    std::vector<std::shared_ptr<NetworkManager>> managers;
    {
        std::lock_guard lock(networkMutex);
        managers = networkManagers;
    }
    for (const std::shared_ptr<NetworkManager>& manager : managers)
        manager->processInboundPackets();
}

std::shared_ptr<NetworkManager> MinecraftServer::addLocalEndpoint()
{
    auto [client, server] = NetworkManager::createLocalPair();
    server->setNetHandler(std::make_shared<NetHandlerHandshakeMemory>(this, server));
    std::lock_guard lock(networkMutex);
    networkManagers.push_back(server);
    return client;
}

void MinecraftServer::networkTick()
{
    std::vector<std::shared_ptr<NetworkManager>> managers;
    {
        std::lock_guard lock(networkMutex);
        managers = networkManagers;
    }

    for (const std::shared_ptr<NetworkManager>& networkmanager : managers) {
        if (networkmanager->hasNoChannel())
            continue;
        if (!networkmanager->isChannelOpen()) {
            {
                std::lock_guard lock(networkMutex);
                networkManagers.erase(std::remove(networkManagers.begin(), networkManagers.end(), networkmanager), networkManagers.end());
            }
            networkmanager->checkDisconnected();
        } else {
            // Java rethrows for local channels: the crash takes the server down.
            networkmanager->processReceivedPackets();
        }
    }
}

void MinecraftServer::terminateEndpoints()
{
    std::lock_guard lock(networkMutex);
    for (const std::shared_ptr<NetworkManager>& manager : networkManagers) {
        manager->closeChannel(std::make_shared<ChatComponentText>("Server closed"));
        // Breaks the manager <-> handler <-> player cycle.
        manager->setNetHandler(nullptr);
    }
    networkManagers.clear();
}

void MinecraftServer::setConfigManager(std::unique_ptr<ServerConfigurationManager> configManager)
{
    serverConfigManager = std::move(configManager);
}

void MinecraftServer::convertMapIfNeeded(const std::string& worldNameIn)
{
    if (getActiveAnvilConverter().isOldMapFormat(worldNameIn)) {
        Log::info("Converting map!");
        setUserMessage("menu.convertingLevel");

        class Progress : public IProgressUpdate {
        public:
            void displaySavingString(const std::string&) override { }
            void resetProgressAndMessage(const std::string&) override { }
            void setLoadingProgress(int progress) override
            {
                if (java::currentTimeMillis() - startTime >= 1000) {
                    startTime = java::currentTimeMillis();
                    Log::info("Converting... {}%", progress);
                }
            }
            void setDoneWorking() override { }
            void displayLoadingString(const std::string&) override { }

        private:
            int64_t startTime = java::currentTimeMillis();
        } progress;

        getActiveAnvilConverter().convertMapFormat(worldNameIn, progress);
    }
}

void MinecraftServer::initialWorldChunkLoad()
{
    int i1 = 0;
    setUserMessage("menu.generatingTerrain");
    const int j1 = 0;
    Log::info("Preparing start region for level {}", j1);
    WorldServer* worldserver = worldServers[static_cast<size_t>(j1)].get();
    const BlockPos blockpos = worldserver->getSpawnPoint();
    int64_t k1 = getCurrentTimeMillis();

    for (int l1 = -192; l1 <= 192 && isServerRunning(); l1 += 16) {
        for (int i2 = -192; i2 <= 192 && isServerRunning(); i2 += 16) {
            const int64_t j2 = getCurrentTimeMillis();
            if (j2 - k1 > 1000) {
                outputPercentRemaining("Preparing spawn area", i1 * 100 / 625);
                k1 = j2;
            }
            ++i1;
            worldserver->theChunkProviderServer->loadChunk(blockpos.getX() + l1 >> 4, blockpos.getZ() + i2 >> 4);
        }
    }

    clearCurrentTask();
}

void MinecraftServer::outputPercentRemaining(const std::string& message, int percent)
{
    currentTask = message;
    percentDone = percent;
    Log::info("{}: {}%", message, percent);
}

void MinecraftServer::clearCurrentTask()
{
    currentTask.reset();
    percentDone = 0;
}

void MinecraftServer::saveAllWorlds(bool dontLog)
{
    if (worldIsBeingDeleted)
        return;

    for (const auto& worldserver : worldServers) {
        if (!worldserver)
            continue;
        if (!dontLog)
            Log::info("Saving chunks for level '{}'/{}", worldserver->getWorldInfo().getWorldName(), worldserver->provider->getDimensionName());
        try {
            worldserver->saveAllChunks(true, nullptr);
        } catch (const MinecraftException& e) {
            Log::warn("{}", e.what());
        }
    }
}

void MinecraftServer::stopServer()
{
    if (worldIsBeingDeleted)
        return;

    Log::info("Stopping server");

    if (serverConfigManager) {
        Log::info("Saving players");
        serverConfigManager->saveAllPlayerData();
        serverConfigManager->removeAllPlayers();
    }

    terminateEndpoints();

    if (!worldServers.empty()) {
        Log::info("Saving worlds");
        saveAllWorlds(false);
        for (const auto& worldserver : worldServers)
            worldserver->flush();
    }
}

void MinecraftServer::run()
{
    serverThreadId = std::this_thread::get_id();

    try {
        if (startServer()) {
            currentTime = getCurrentTimeMillis();
            int64_t i = 0;

            while (serverRunning) {
                const int64_t k = getCurrentTimeMillis();
                int64_t j = k - currentTime;

                if (j > 2000 && currentTime - timeOfLastWarning >= 15000) {
                    Log::warn("Can't keep up! Did the system time change, or is the server overloaded? Running {}ms behind, skipping {} tick(s)", j, j / 50);
                    j = 2000;
                    timeOfLastWarning = currentTime;
                }

                if (j < 0) {
                    Log::warn("Time ran backwards! Did the system time change?");
                    j = 0;
                }

                i += j;
                currentTime = k;

                if (worldServers[0]->areAllPlayersAsleep()) {
                    tick();
                    i = 0;
                } else {
                    while (i > 50) {
                        i -= 50;
                        tick();
                    }
                }

                std::this_thread::sleep_for(std::chrono::milliseconds(std::max<int64_t>(1, 50 - i)));
                serverIsRunning = true;
            }
        } else {
            finalTick();
        }
    } catch (const std::exception& e) {
        Log::error("Encountered an unexpected exception: {}", e.what());
        finalTick();
    }

    try {
        serverStopped = true;
        stopServer();
    } catch (const std::exception& e) {
        Log::error("Exception stopping the server: {}", e.what());
    }
}

void MinecraftServer::tick()
{
    const int64_t i = java::nanoTime();
    ++tickCounter;
    theProfiler.startSection("root");
    updateTimeLightAndEntities();
    // Java: refreshes the status response (player sample) every 5 seconds -- the status protocol, stage 14.

    if (tickCounter % 900 == 0) {
        theProfiler.startSection("save");
        serverConfigManager->saveAllPlayerData();
        saveAllWorlds(true);
        theProfiler.endSection();
    }

    theProfiler.startSection("tallying");
    tickTimeArray[static_cast<size_t>(tickCounter % 100)] = java::nanoTime() - i;
    theProfiler.endSection();
    theProfiler.startSection("snooper");
    theProfiler.endSection();
    theProfiler.endSection();
}

void MinecraftServer::updateTimeLightAndEntities()
{
    theProfiler.startSection("jobs");
    runScheduledTasks();
    theProfiler.endStartSection("levels");

    for (size_t j = 0; j < worldServers.size(); ++j) {
        if (j == 0 || getAllowNether()) {
            WorldServer* worldserver = worldServers[j].get();
            theProfiler.startSection(worldserver->getWorldInfo().getWorldName());

            if (tickCounter % 20 == 0) {
                theProfiler.startSection("timeSync");
                serverConfigManager->sendPacketToAllPlayersInDimension(std::make_shared<S03PacketTimeUpdate>(worldserver->getTotalWorldTime(),
                                                                           worldserver->getWorldTime(), worldserver->getGameRules().getBoolean("doDaylightCycle")),
                    worldserver->provider->getDimensionId());
                theProfiler.endSection();
            }

            theProfiler.startSection("tick");
            worldserver->tick();
            worldserver->updateEntities();
            theProfiler.endSection();
            theProfiler.startSection("tracker");
            worldserver->getEntityTracker()->updateTrackedEntities();
            theProfiler.endSection();
            theProfiler.endSection();
        }
    }

    theProfiler.endStartSection("connection");
    networkTick();
    theProfiler.endStartSection("players");
    serverConfigManager->onTick();
    theProfiler.endStartSection("tickables");
    theProfiler.endSection();
}

WorldServer* MinecraftServer::worldServerForDimension(int dimension) const
{
    // Java: worldServers[1] is the Nether, [2] the End. Only the overworld exists so far (see WorldServer.h).
    const size_t index = dimension == -1 ? 1 : (dimension == 1 ? 2 : 0);
    return index < worldServers.size() ? worldServers[index].get() : worldServers[0].get();
}

void MinecraftServer::setDifficultyForAllWorlds(EnumDifficulty difficulty)
{
    for (const auto& world : worldServers) {
        if (!world)
            continue;
        if (world->getWorldInfo().isHardcoreModeEnabled()) {
            world->getWorldInfo().setDifficulty(EnumDifficulty::HARD);
            world->setAllowedSpawnTypes(true, true);
        } else if (isSinglePlayer()) {
            world->getWorldInfo().setDifficulty(difficulty);
            world->setAllowedSpawnTypes(world->getDifficulty() != EnumDifficulty::PEACEFUL, true);
        } else {
            world->getWorldInfo().setDifficulty(difficulty);
            world->setAllowedSpawnTypes(difficulty != EnumDifficulty::PEACEFUL, canSpawnAnimals);
        }
    }
}

void MinecraftServer::addChatMessage(const std::shared_ptr<IChatComponent>& message)
{
    Log::info("{}", message->getUnformattedText());
}

void MinecraftServer::deleteWorldAndStopServer()
{
    worldIsBeingDeleted = true;
    getActiveAnvilConverter().flushCache();
    for (const auto& worldserver : worldServers) {
        if (worldserver)
            worldserver->flush();
    }
    getActiveAnvilConverter().deleteWorldDirectory(worldServers[0]->getSaveHandler()->getWorldDirectoryName());
    initiateShutdown();
}

} // namespace mc

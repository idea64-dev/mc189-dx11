#include "server/integrated/IntegratedServer.h"

#include "client/Minecraft.h"
#include "client/multiplayer/WorldClient.h"
#include "client/settings/GameSettings.h"
#include "core/Log.h"
#include "entity/player/EntityPlayerMP.h"
#include "server/management/ServerConfigurationManager.h"
#include "world/WorldServer.h"
#include "world/storage/ISaveHandler.h"
#include "world/storage/SaveFormat.h"
#include "world/storage/WorldInfo.h"

namespace mc {

IntegratedServer::IntegratedServer(Minecraft* mcIn, const std::string& folderName, const std::string& worldName, const WorldSettings& settings)
    : MinecraftServer(mcIn->getDataDirectory() / "saves")
    , mc(mcIn)
    , theWorldSettings(settings)
{
    setServerOwner(mcIn->getSession().getUsername());
    setFolderName(folderName);
    setWorldName(worldName);
    canCreateBonusChest(settings.isBonusChestEnabled());
    setBuildLimit(256);
    setConfigManager(std::make_unique<IntegratedPlayerList>(this));
}

void IntegratedServer::loadAllWorlds(const std::string& saveName, const std::string& worldNameIn)
{
    convertMapIfNeeded(saveName);
    const std::shared_ptr<ISaveHandler> isavehandler = getActiveAnvilConverter().getSaveLoader(saveName, true);
    // Java: setResourcePackFromWorld -- a resources.zip inside the save, with the server resource packs (stage 14).
    std::shared_ptr<WorldInfo> worldinfo = isavehandler->loadWorldInfo();

    if (!worldinfo)
        worldinfo = std::make_shared<WorldInfo>(theWorldSettings, worldNameIn);
    else
        worldinfo->setWorldName(worldNameIn);

    // Java: also the Nether (-1) and the End (1) as WorldServerMulti -- see WorldServer.h.
    auto worldserver = std::make_unique<WorldServer>(this, isavehandler, worldinfo, 0, theProfiler);
    worldserver->init();
    worldserver->initialize(theWorldSettings);
    WorldServer* overworld = worldserver.get();
    worldServers.push_back(std::move(worldserver));
    overworld->setWorldManager(std::make_unique<WorldManager>(this, overworld));

    std::vector<WorldServer*> worlds;
    for (const auto& world : worldServers)
        worlds.push_back(world.get());
    getConfigurationManager()->setPlayerManager(worlds);

    if (!worldServers[0]->getWorldInfo().getDifficulty())
        setDifficultyForAllWorlds(mc->gameSettings->difficulty);

    initialWorldChunkLoad();
}

bool IntegratedServer::startServer()
{
    Log::info("Starting integrated minecraft server version 1.8.9");
    setOnlineMode(true);
    setCanSpawnAnimals(true);
    setCanSpawnNPCs(true);
    setAllowPvp(true);
    setAllowFlight(true);
    // Java: generates the key pair for remote logins -- multiplayer.
    loadAllWorlds(getFolderName(), getWorldName());
    setMOTD(getServerOwner() + " - " + worldServers[0]->getWorldInfo().getWorldName());
    return true;
}

void IntegratedServer::tick()
{
    const bool flag = isGamePaused;
    isGamePaused = mc->getNetHandler() != nullptr && mc->isGamePaused();

    if (!flag && isGamePaused) {
        Log::info("Saving and pausing game...");
        getConfigurationManager()->saveAllPlayerData();
        saveAllWorlds(false);
    }

    if (isGamePaused) {
        runScheduledTasks();
        return;
    }

    MinecraftServer::tick();

    const int renderDistance = mc->gameSettings->renderDistanceChunks;
    if (renderDistance != getConfigurationManager()->getViewDistance()) {
        Log::info("Changing view distance to {}, from {}", renderDistance, getConfigurationManager()->getViewDistance());
        getConfigurationManager()->setViewDistance(renderDistance);
    }

    // Java reads the client world's info from the server thread, unsynchronised; so does this.
    if (WorldClient* clientWorld = mc->theWorld) {
        WorldInfo& worldinfo1 = worldServers[0]->getWorldInfo();
        WorldInfo& worldinfo = clientWorld->getWorldInfo();

        if (!worldinfo1.isDifficultyLocked() && worldinfo.getDifficulty() != worldinfo1.getDifficulty()) {
            Log::info("Changing difficulty to {}, from {}", static_cast<int>(worldinfo.getDifficulty().value_or(EnumDifficulty::NORMAL)),
                static_cast<int>(worldinfo1.getDifficulty().value_or(EnumDifficulty::NORMAL)));
            setDifficultyForAllWorlds(worldinfo.getDifficulty().value_or(EnumDifficulty::NORMAL));
        } else if (worldinfo.isDifficultyLocked() && !worldinfo1.isDifficultyLocked()) {
            Log::info("Locking difficulty to {}", static_cast<int>(worldinfo.getDifficulty().value_or(EnumDifficulty::NORMAL)));
            for (const auto& worldserver : worldServers) {
                if (worldserver)
                    worldserver->getWorldInfo().setDifficultyLocked(true);
            }
        }
    }
}

void IntegratedServer::setDifficultyForAllWorlds(EnumDifficulty difficulty)
{
    MinecraftServer::setDifficultyForAllWorlds(difficulty);
    if (WorldClient* clientWorld = mc->theWorld)
        clientWorld->getWorldInfo().setDifficulty(difficulty);
}

void IntegratedServer::finalTick()
{
    // Java: mc.crashed(report) -- the client shows the crash.
    Log::error("The integrated server crashed");
}

void IntegratedServer::stopServer()
{
    MinecraftServer::stopServer();
}

void IntegratedServer::initiateShutdown()
{
    runOnServerThreadAndWait([this] {
        const std::vector<std::shared_ptr<EntityPlayerMP>> players = getConfigurationManager()->getPlayerList();
        for (const std::shared_ptr<EntityPlayerMP>& entityplayermp : players)
            getConfigurationManager()->playerLoggedOut(entityplayermp.get());
    });
    MinecraftServer::initiateShutdown();
}

void IntegratedServer::setGameType(GameType gameMode)
{
    getConfigurationManager()->setGameType(gameMode);
}

} // namespace mc

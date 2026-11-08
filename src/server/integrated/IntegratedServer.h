#pragma once

// Original: net/minecraft/server/integrated/IntegratedServer.java -- the server a singleplayer game runs in,
// on its own thread next to the client.
// Not ported yet: opening to LAN (stage 14) and the Nether / End worlds (WorldServerMulti, stage 12).

#include "server/MinecraftServer.h"

namespace mc {

class Minecraft;

class IntegratedServer : public MinecraftServer {
public:
    IntegratedServer(Minecraft* mcIn, const std::string& folderName, const std::string& worldName, const WorldSettings& settings);

    bool canStructuresSpawn() const override { return false; }
    GameType getGameType() const override { return theWorldSettings.getGameType(); }
    bool isHardcore() const override { return theWorldSettings.getHardcoreEnabled(); }
    bool isDedicatedServer() const override { return false; }
    bool isCommandBlockEnabled() const override { return true; }
    int getOpPermissionLevel() const override { return 4; }
    void setDifficultyForAllWorlds(EnumDifficulty difficulty) override;
    void initiateShutdown() override;
    bool getPublic() const { return isPublic; }
    void setGameType(GameType gameMode);

protected:
    bool startServer() override;
    void tick() override;
    void stopServer() override;
    void finalTick() override;

private:
    void loadAllWorlds(const std::string& saveName, const std::string& worldNameIn);

    Minecraft* const mc;
    const WorldSettings theWorldSettings;
    bool isGamePaused = false;
    bool isPublic = false;
};

} // namespace mc

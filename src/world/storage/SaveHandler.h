#pragma once

// Original: net/minecraft/world/storage/SaveHandler.java and net/minecraft/world/chunk/storage/AnvilSaveHandler.java
// -- a world directory: session.lock, level.dat (with level.dat_old as backup), playerdata/ and data/.

#include "world/storage/ISaveHandler.h"

#include <cstdint>
#include <filesystem>
#include <string>

namespace mc {

class SaveHandler : public ISaveHandler, public IPlayerFileData {
public:
    // Throws std::runtime_error when the session lock can't be written ("Failed to check session lock, aborting").
    SaveHandler(const std::filesystem::path& savesDirectory, const std::string& directoryName, bool playersDirectoryIn);

    std::filesystem::path getWorldDirectory() const override { return worldDirectory; }
    void checkSessionLock() override;
    // Java throws "Old Chunk Storage is no longer supported."
    std::unique_ptr<IChunkLoader> getChunkLoader(const WorldProvider& provider) override;
    std::unique_ptr<WorldInfo> loadWorldInfo() override;
    void saveWorldInfoWithPlayer(WorldInfo& worldInformation, const std::shared_ptr<NBTTagCompound>& tagCompound) override;
    void saveWorldInfo(WorldInfo& worldInformation) override;
    void flush() override { }
    std::filesystem::path getMapFileFromName(const std::string& mapName) const override { return mapDataDir / (mapName + ".dat"); }
    std::string getWorldDirectoryName() const override { return saveDirectoryName; }
    IPlayerFileData* getPlayerNBTManager() override { return this; }
    void writePlayerData(EntityPlayer* player) override;
    std::shared_ptr<NBTTagCompound> readPlayerData(EntityPlayer* player) override;
    std::vector<std::string> getAvailablePlayerDat() override;

protected:
    void writeLevel(const std::shared_ptr<NBTTagCompound>& data);

private:
    void setSessionLock();

    const std::filesystem::path worldDirectory;
    const std::filesystem::path playersDirectory;
    const std::filesystem::path mapDataDir;
    const int64_t initializationTime;
    const std::string saveDirectoryName;
};

class AnvilSaveHandler : public SaveHandler {
public:
    using SaveHandler::SaveHandler;

    // DIM-1 for the Nether, DIM1 for the End, the world directory otherwise.
    std::unique_ptr<IChunkLoader> getChunkLoader(const WorldProvider& provider) override;
    void saveWorldInfoWithPlayer(WorldInfo& worldInformation, const std::shared_ptr<NBTTagCompound>& tagCompound) override;
    // Waits for the File IO thread and closes the region files.
    void flush() override;
};

} // namespace mc

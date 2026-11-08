#include "world/storage/SaveHandler.h"

#include "core/DataStream.h"
#include "core/JavaTime.h"
#include "core/Log.h"
#include "core/StringUtil.h"
#include "nbt/CompressedStreamTools.h"
#include "world/WorldProvider.h"
#include "world/chunk/storage/AnvilChunkLoader.h"
#include "world/chunk/storage/RegionFile.h"
#include "world/storage/ThreadedFileIOBase.h"
#include "world/storage/WorldInfo.h"

#include <fstream>

namespace mc {

SaveHandler::SaveHandler(const std::filesystem::path& savesDirectory, const std::string& directoryName, bool playersDirectoryIn)
    : worldDirectory(savesDirectory / toWide(directoryName))
    , playersDirectory(worldDirectory / "playerdata")
    , mapDataDir(worldDirectory / "data")
    , initializationTime(java::currentTimeMillis())
    , saveDirectoryName(directoryName)
{
    std::error_code ec;
    std::filesystem::create_directories(worldDirectory, ec);
    std::filesystem::create_directories(mapDataDir, ec);
    if (playersDirectoryIn)
        std::filesystem::create_directories(playersDirectory, ec);
    setSessionLock();
}

void SaveHandler::setSessionLock()
{
    std::ofstream file(worldDirectory / "session.lock", std::ios::binary | std::ios::trunc);
    DataOutput output;
    output.writeLong(initializationTime);
    file.write(reinterpret_cast<const char*>(output.getBytes().data()), static_cast<std::streamsize>(output.getBytes().size()));
    if (!file) {
        Log::error("Couldn't write {}", toUtf8((worldDirectory / "session.lock").wstring()));
        throw std::runtime_error("Failed to check session lock, aborting");
    }
}

void SaveHandler::checkSessionLock()
{
    std::ifstream file(worldDirectory / "session.lock", std::ios::binary);
    uint8_t bytes[8] {};
    file.read(reinterpret_cast<char*>(bytes), 8);
    if (!file || file.gcount() != 8)
        throw MinecraftException("Failed to check session lock, aborting");
    DataInput input(bytes);
    if (input.readLong() != initializationTime)
        throw MinecraftException("The save is being accessed from another location, aborting");
}

std::unique_ptr<IChunkLoader> SaveHandler::getChunkLoader(const WorldProvider&)
{
    throw std::runtime_error("Old Chunk Storage is no longer supported.");
}

std::unique_ptr<WorldInfo> SaveHandler::loadWorldInfo()
{
    for (const char* name : { "level.dat", "level.dat_old" }) {
        const std::filesystem::path file1 = worldDirectory / name;
        std::error_code ec;
        if (!std::filesystem::exists(file1, ec))
            continue;
        try {
            const auto nbttagcompound = CompressedStreamTools::readCompressed(file1);
            return std::make_unique<WorldInfo>(*nbttagcompound->getCompoundTag("Data"));
        } catch (const std::exception& e) {
            Log::error("{}", e.what()); // Java: printStackTrace
        }
    }
    return nullptr;
}

void SaveHandler::writeLevel(const std::shared_ptr<NBTTagCompound>& data)
{
    NBTTagCompound nbttagcompound1;
    nbttagcompound1.setTag("Data", data);
    try {
        const std::filesystem::path file1 = worldDirectory / "level.dat_new";
        const std::filesystem::path file2 = worldDirectory / "level.dat_old";
        const std::filesystem::path file3 = worldDirectory / "level.dat";
        CompressedStreamTools::writeCompressed(nbttagcompound1, file1);
        std::error_code ec;
        if (std::filesystem::exists(file2, ec))
            std::filesystem::remove(file2, ec);
        std::filesystem::rename(file3, file2, ec);
        if (std::filesystem::exists(file3, ec))
            std::filesystem::remove(file3, ec);
        std::filesystem::rename(file1, file3, ec);
        if (std::filesystem::exists(file1, ec))
            std::filesystem::remove(file1, ec);
    } catch (const std::exception& e) {
        Log::error("{}", e.what());
    }
}

void SaveHandler::saveWorldInfoWithPlayer(WorldInfo& worldInformation, const std::shared_ptr<NBTTagCompound>& tagCompound)
{
    writeLevel(worldInformation.cloneNBTCompound(tagCompound));
}

void SaveHandler::saveWorldInfo(WorldInfo& worldInformation)
{
    writeLevel(worldInformation.getNBTTagCompound());
}

// --- AnvilSaveHandler ---

std::unique_ptr<IChunkLoader> AnvilSaveHandler::getChunkLoader(const WorldProvider& provider)
{
    const std::filesystem::path file1 = getWorldDirectory();
    std::error_code ec;
    if (dynamic_cast<const WorldProviderHell*>(&provider)) {
        const std::filesystem::path file3 = file1 / "DIM-1";
        std::filesystem::create_directories(file3, ec);
        return std::make_unique<AnvilChunkLoader>(file3);
    }
    if (dynamic_cast<const WorldProviderEnd*>(&provider)) {
        const std::filesystem::path file2 = file1 / "DIM1";
        std::filesystem::create_directories(file2, ec);
        return std::make_unique<AnvilChunkLoader>(file2);
    }
    return std::make_unique<AnvilChunkLoader>(file1);
}

void AnvilSaveHandler::saveWorldInfoWithPlayer(WorldInfo& worldInformation, const std::shared_ptr<NBTTagCompound>& tagCompound)
{
    worldInformation.setSaveVersion(19133);
    SaveHandler::saveWorldInfoWithPlayer(worldInformation, tagCompound);
}

void AnvilSaveHandler::flush()
{
    ThreadedFileIOBase::getThreadedIOInstance().waitForFinish();
    RegionFileCache::clearRegionFileReferences();
}

} // namespace mc

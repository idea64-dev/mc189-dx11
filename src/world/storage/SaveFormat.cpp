#include "world/storage/SaveFormat.h"

#include "core/JavaString.h"
#include "core/Log.h"
#include "core/StringUtil.h"
#include "nbt/CompressedStreamTools.h"
#include "world/chunk/storage/RegionFile.h"
#include "world/storage/SaveHandler.h"
#include "world/storage/WorldInfo.h"

#include <thread>

namespace mc {

int SaveFormatComparator::compareTo(const SaveFormatComparator& other) const
{
    if (lastTimePlayed < other.lastTimePlayed)
        return 1;
    if (lastTimePlayed > other.lastTimePlayed)
        return -1;
    return java::compareTo(fileName, other.fileName);
}

// --- SaveFormatOld ---

SaveFormatOld::SaveFormatOld(const std::filesystem::path& savesDirectoryIn)
    : savesDirectory(savesDirectoryIn)
{
    std::error_code ec;
    if (!std::filesystem::exists(savesDirectoryIn, ec))
        std::filesystem::create_directories(savesDirectoryIn, ec);
}

std::filesystem::path SaveFormatOld::worldPath(const std::string& saveName) const
{
    return savesDirectory / toWide(saveName);
}

std::vector<SaveFormatComparator> SaveFormatOld::getSaveList()
{
    std::vector<SaveFormatComparator> list;
    for (int i = 0; i < 5; ++i) {
        const std::string s = "World" + std::to_string(i + 1);
        if (const auto worldinfo = getWorldInfo(s))
            list.emplace_back(s, "", worldinfo->getLastTimePlayed(), worldinfo->getSizeOnDisk(), worldinfo->getGameType(), false,
                worldinfo->isHardcoreModeEnabled(), worldinfo->areCommandsAllowed());
    }
    return list;
}

std::unique_ptr<WorldInfo> SaveFormatOld::getWorldInfo(const std::string& saveName)
{
    const std::filesystem::path file1 = worldPath(saveName);
    std::error_code ec;
    if (!std::filesystem::exists(file1, ec))
        return nullptr;
    for (const char* name : { "level.dat", "level.dat_old" }) {
        const std::filesystem::path file2 = file1 / name;
        if (!std::filesystem::exists(file2, ec))
            continue;
        try {
            const auto nbttagcompound = CompressedStreamTools::readCompressed(file2);
            return std::make_unique<WorldInfo>(*nbttagcompound->getCompoundTag("Data"));
        } catch (const std::exception& e) {
            Log::error("Exception reading {}: {}", toUtf8(file2.wstring()), e.what());
        }
    }
    return nullptr;
}

void SaveFormatOld::renameWorld(const std::string& dirName, const std::string& newName)
{
    const std::filesystem::path file1 = worldPath(dirName);
    std::error_code ec;
    if (!std::filesystem::exists(file1, ec))
        return;
    const std::filesystem::path file2 = file1 / "level.dat";
    if (!std::filesystem::exists(file2, ec))
        return;
    try {
        const auto nbttagcompound = CompressedStreamTools::readCompressed(file2);
        nbttagcompound->getCompoundTag("Data")->setString("LevelName", newName);
        CompressedStreamTools::writeCompressed(*nbttagcompound, file2);
    } catch (const std::exception& e) {
        Log::error("{}", e.what());
    }
}

bool SaveFormatOld::isNewLevelIdAcceptable(const std::string& saveName)
{
    const std::filesystem::path file1 = worldPath(saveName);
    std::error_code ec;
    if (std::filesystem::exists(file1, ec))
        return false;
    // Java: mkdir + delete to see whether the name is a valid directory.
    if (!std::filesystem::create_directory(file1, ec)) {
        Log::warn("Couldn't make new level");
        return false;
    }
    std::filesystem::remove(file1, ec);
    return true;
}

bool SaveFormatOld::deleteWorldDirectory(const std::string& saveName)
{
    const std::filesystem::path file1 = worldPath(saveName);
    std::error_code ec;
    if (!std::filesystem::exists(file1, ec))
        return true;
    Log::info("Deleting level {}", saveName);
    for (int i = 1; i <= 5; ++i) {
        Log::info("Attempt {}...", i);
        if (deleteFiles(file1))
            break;
        Log::warn("Unsuccessful in deleting contents.");
        if (i < 5)
            std::this_thread::sleep_for(std::chrono::milliseconds(500));
    }
    return std::filesystem::remove(file1, ec);
}

bool SaveFormatOld::deleteFiles(const std::filesystem::path& directory)
{
    std::error_code ec;
    std::vector<std::filesystem::path> files;
    for (const auto& entry : std::filesystem::directory_iterator(directory, ec))
        files.push_back(entry.path());
    for (const auto& file1 : files) {
        Log::debug("Deleting {}", toUtf8(file1.wstring()));
        if (std::filesystem::is_directory(file1, ec) && !deleteFiles(file1)) {
            Log::warn("Couldn't delete directory {}", toUtf8(file1.wstring()));
            return false;
        }
        if (!std::filesystem::remove(file1, ec)) {
            Log::warn("Couldn't delete file {}", toUtf8(file1.wstring()));
            return false;
        }
    }
    return true;
}

std::shared_ptr<ISaveHandler> SaveFormatOld::getSaveLoader(const std::string& saveName, bool storePlayerdata)
{
    return std::make_shared<SaveHandler>(savesDirectory, saveName, storePlayerdata);
}

bool SaveFormatOld::canLoadWorld(const std::string& saveName)
{
    std::error_code ec;
    return std::filesystem::is_directory(worldPath(saveName), ec);
}

// --- AnvilSaveConverter ---

std::vector<SaveFormatComparator> AnvilSaveConverter::getSaveList()
{
    std::error_code ec;
    if (savesDirectory.empty() || !std::filesystem::exists(savesDirectory, ec) || !std::filesystem::is_directory(savesDirectory, ec))
        throw AnvilConverterException("Unable to read or access folder where game worlds are saved!");
    std::vector<SaveFormatComparator> list;
    for (const auto& entry : std::filesystem::directory_iterator(savesDirectory, ec)) {
        if (!entry.is_directory(ec))
            continue;
        const std::string s = toUtf8(entry.path().filename().wstring());
        const auto worldinfo = getWorldInfo(s);
        if (worldinfo && (worldinfo->getSaveVersion() == 19132 || worldinfo->getSaveVersion() == 19133)) {
            const bool flag = worldinfo->getSaveVersion() != getSaveVersion();
            std::string s1 = worldinfo->getWorldName();
            if (s1.empty())
                s1 = s;
            list.emplace_back(s, s1, worldinfo->getLastTimePlayed(), 0, worldinfo->getGameType(), flag, worldinfo->isHardcoreModeEnabled(),
                worldinfo->areCommandsAllowed());
        }
    }
    return list;
}

void AnvilSaveConverter::flushCache()
{
    RegionFileCache::clearRegionFileReferences();
}

std::shared_ptr<ISaveHandler> AnvilSaveConverter::getSaveLoader(const std::string& saveName, bool storePlayerdata)
{
    return std::make_shared<AnvilSaveHandler>(savesDirectory, saveName, storePlayerdata);
}

bool AnvilSaveConverter::isConvertible(const std::string& saveName)
{
    const auto worldinfo = getWorldInfo(saveName);
    return worldinfo && worldinfo->getSaveVersion() == 19132;
}

bool AnvilSaveConverter::isOldMapFormat(const std::string& saveName)
{
    const auto worldinfo = getWorldInfo(saveName);
    return worldinfo && worldinfo->getSaveVersion() != getSaveVersion();
}

bool AnvilSaveConverter::convertMapFormat(const std::string& filename, IProgressUpdate&)
{
    // Java converts McRegion (.mcr) worlds chunk by chunk, filling in biomes from the generator -- stage 6.
    Log::warn("Converting {} from McRegion needs the biome generator (stage 6); not converted", filename);
    return false;
}

} // namespace mc

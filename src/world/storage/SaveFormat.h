#pragma once

// Original: net/minecraft/world/storage/ISaveFormat.java, SaveFormatOld.java, SaveFormatComparator.java,
// net/minecraft/world/chunk/storage/AnvilSaveConverter.java and net/minecraft/client/AnvilConverterException.java
// -- the saves directory: listing, renaming, deleting and opening worlds.
// Not ported yet: convertMapFormat's McRegion-to-Anvil conversion (ChunkLoader), which needs the biome source
// (stage 6); it reports failure until then.

#include "world/WorldSettings.h"

#include <cstdint>
#include <filesystem>
#include <memory>
#include <stdexcept>
#include <string>
#include <vector>

namespace mc {

class IProgressUpdate;
class ISaveHandler;
class WorldInfo;

class AnvilConverterException : public std::runtime_error {
public:
    using std::runtime_error::runtime_error;
};

class SaveFormatComparator {
public:
    SaveFormatComparator(std::string fileNameIn, std::string displayNameIn, int64_t lastTimePlayedIn, int64_t sizeOnDiskIn, GameType gameTypeIn,
        bool requiresConversionIn, bool hardcoreIn, bool cheatsEnabledIn)
        : fileName(std::move(fileNameIn))
        , displayName(std::move(displayNameIn))
        , lastTimePlayed(lastTimePlayedIn)
        , sizeOnDisk(sizeOnDiskIn)
        , requiresConversionFlag(requiresConversionIn)
        , theEnumGameType(gameTypeIn)
        , hardcore(hardcoreIn)
        , cheatsEnabled(cheatsEnabledIn)
    {
    }

    const std::string& getFileName() const { return fileName; }
    const std::string& getDisplayName() const { return displayName; }
    int64_t getSizeOnDisk() const { return sizeOnDisk; }
    bool requiresConversion() const { return requiresConversionFlag; }
    int64_t getLastTimePlayed() const { return lastTimePlayed; }
    // Most recently played first, then by directory name.
    int compareTo(const SaveFormatComparator& other) const;
    GameType getEnumGameType() const { return theEnumGameType; }
    bool isHardcoreModeEnabled() const { return hardcore; }
    bool getCheatsEnabled() const { return cheatsEnabled; }

private:
    std::string fileName;
    std::string displayName;
    int64_t lastTimePlayed;
    int64_t sizeOnDisk;
    bool requiresConversionFlag;
    GameType theEnumGameType;
    bool hardcore;
    bool cheatsEnabled;
};

class ISaveFormat {
public:
    virtual ~ISaveFormat() = default;
    virtual std::string getName() const = 0;
    virtual std::shared_ptr<ISaveHandler> getSaveLoader(const std::string& saveName, bool storePlayerdata) = 0;
    // Throws AnvilConverterException when the saves directory can't be read.
    virtual std::vector<SaveFormatComparator> getSaveList() = 0;
    virtual void flushCache() = 0;
    // Null when the world or its level.dat doesn't exist or can't be read.
    virtual std::unique_ptr<WorldInfo> getWorldInfo(const std::string& saveName) = 0;
    virtual bool isNewLevelIdAcceptable(const std::string& saveName) = 0;
    virtual bool deleteWorldDirectory(const std::string& saveName) = 0;
    virtual void renameWorld(const std::string& dirName, const std::string& newName) = 0;
    virtual bool isConvertible(const std::string& saveName) = 0;
    virtual bool isOldMapFormat(const std::string& saveName) = 0;
    virtual bool convertMapFormat(const std::string& filename, IProgressUpdate& progressCallback) = 0;
    virtual bool canLoadWorld(const std::string& saveName) = 0;
};

class SaveFormatOld : public ISaveFormat {
public:
    explicit SaveFormatOld(const std::filesystem::path& savesDirectoryIn);

    std::string getName() const override { return "Old Format"; }
    std::vector<SaveFormatComparator> getSaveList() override;
    void flushCache() override { }
    std::unique_ptr<WorldInfo> getWorldInfo(const std::string& saveName) override;
    void renameWorld(const std::string& dirName, const std::string& newName) override;
    bool isNewLevelIdAcceptable(const std::string& saveName) override;
    bool deleteWorldDirectory(const std::string& saveName) override;
    std::shared_ptr<ISaveHandler> getSaveLoader(const std::string& saveName, bool storePlayerdata) override;
    bool isConvertible(const std::string&) override { return false; }
    bool isOldMapFormat(const std::string&) override { return false; }
    bool convertMapFormat(const std::string&, IProgressUpdate&) override { return false; }
    bool canLoadWorld(const std::string& saveName) override;

protected:
    static bool deleteFiles(const std::filesystem::path& directory);
    std::filesystem::path worldPath(const std::string& saveName) const;

    const std::filesystem::path savesDirectory;
};

class AnvilSaveConverter : public SaveFormatOld {
public:
    using SaveFormatOld::SaveFormatOld;

    std::string getName() const override { return "Anvil"; }
    std::vector<SaveFormatComparator> getSaveList() override;
    void flushCache() override;
    std::shared_ptr<ISaveHandler> getSaveLoader(const std::string& saveName, bool storePlayerdata) override;
    bool isConvertible(const std::string& saveName) override;
    bool isOldMapFormat(const std::string& saveName) override;
    bool convertMapFormat(const std::string& filename, IProgressUpdate& progressCallback) override;

protected:
    int getSaveVersion() const { return 19133; }
};

} // namespace mc

#pragma once

// Original: net/minecraft/world/storage/MapStorage.java and net/minecraft/world/WorldSavedData.java -- the
// data/*.dat files of a save (structures, villages, maps, the scoreboard) and data/idcounts.dat.

#include "core/JavaHashMap.h"

#include <functional>
#include <memory>
#include <string>
#include <vector>

namespace mc {

class ISaveHandler;
class NBTTagCompound;

class WorldSavedData {
public:
    explicit WorldSavedData(std::string name)
        : mapName(std::move(name))
    {
    }
    virtual ~WorldSavedData() = default;

    virtual void readFromNBT(const NBTTagCompound& nbt) = 0;
    virtual void writeToNBT(NBTTagCompound& nbt) const = 0;
    void markDirty() { setDirty(true); }
    void setDirty(bool isDirty) { dirty = isDirty; }
    bool isDirty() const { return dirty; }

    const std::string mapName;

private:
    bool dirty = false;
};

class MapStorage {
public:
    // saveHandlerIn may be null (the data then lives only in memory).
    explicit MapStorage(ISaveHandler* saveHandlerIn);

    // Java: loadData(Class, id) -- the loaded instance, or one read from data/<id>.dat, or null.
    template <class T>
    std::shared_ptr<T> loadData(const std::string& dataIdentifier)
    {
        return std::dynamic_pointer_cast<T>(loadData(dataIdentifier, [](const std::string& id) -> std::shared_ptr<WorldSavedData> { return std::make_shared<T>(id); }));
    }
    std::shared_ptr<WorldSavedData> loadData(const std::string& dataIdentifier, const std::function<std::shared_ptr<WorldSavedData>(const std::string&)>& factory);
    void setData(const std::string& dataIdentifier, std::shared_ptr<WorldSavedData> data);
    void saveAllData();
    int getUniqueDataId(const std::string& key);

private:
    void saveData(WorldSavedData& data);
    void loadIdCounts();

    ISaveHandler* saveHandler;
    JavaHashMap<std::string, std::shared_ptr<WorldSavedData>> loadedDataMap;
    std::vector<std::shared_ptr<WorldSavedData>> loadedDataList;
    JavaHashMap<std::string, int16_t> idCounts;
};

} // namespace mc

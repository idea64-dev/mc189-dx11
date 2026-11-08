#include "world/storage/MapStorage.h"

#include "core/Log.h"
#include "nbt/CompressedStreamTools.h"
#include "nbt/NBTTagCompound.h"
#include "world/storage/ISaveHandler.h"

#include <algorithm>
#include <exception>
#include <filesystem>

namespace mc {

MapStorage::MapStorage(ISaveHandler* saveHandlerIn)
    : saveHandler(saveHandlerIn)
{
    loadIdCounts();
}

std::shared_ptr<WorldSavedData> MapStorage::loadData(const std::string& dataIdentifier,
    const std::function<std::shared_ptr<WorldSavedData>(const std::string&)>& factory)
{
    if (const std::shared_ptr<WorldSavedData>* found = loadedDataMap.find(dataIdentifier))
        return *found;
    std::shared_ptr<WorldSavedData> worldsaveddata;
    if (saveHandler) {
        try {
            const std::filesystem::path file1 = saveHandler->getMapFileFromName(dataIdentifier);
            if (!file1.empty() && std::filesystem::exists(file1)) {
                worldsaveddata = factory(dataIdentifier);
                const auto nbttagcompound = CompressedStreamTools::readCompressed(file1);
                worldsaveddata->readFromNBT(*nbttagcompound->getCompoundTag("data"));
            }
        } catch (const std::exception& exception) {
            // Java prints the stack trace and keeps the instance it created, if any.
            Log::warn("Failed to load data {}: {}", dataIdentifier, exception.what());
        }
    }
    if (worldsaveddata) {
        loadedDataMap.put(dataIdentifier, worldsaveddata);
        loadedDataList.push_back(worldsaveddata);
    }
    return worldsaveddata;
}

void MapStorage::setData(const std::string& dataIdentifier, std::shared_ptr<WorldSavedData> data)
{
    if (const std::shared_ptr<WorldSavedData>* found = loadedDataMap.find(dataIdentifier)) {
        const std::shared_ptr<WorldSavedData> old = *found;
        loadedDataMap.erase(dataIdentifier);
        const auto it = std::find(loadedDataList.begin(), loadedDataList.end(), old);
        if (it != loadedDataList.end())
            loadedDataList.erase(it);
    }
    loadedDataMap.put(dataIdentifier, data);
    loadedDataList.push_back(std::move(data));
}

void MapStorage::saveAllData()
{
    for (size_t i = 0; i < loadedDataList.size(); ++i) {
        WorldSavedData& worldsaveddata = *loadedDataList[i];
        if (worldsaveddata.isDirty()) {
            saveData(worldsaveddata);
            worldsaveddata.setDirty(false);
        }
    }
}

void MapStorage::saveData(WorldSavedData& data)
{
    if (!saveHandler)
        return;
    try {
        const std::filesystem::path file1 = saveHandler->getMapFileFromName(data.mapName);
        if (!file1.empty()) {
            auto nbttagcompound = std::make_shared<NBTTagCompound>();
            data.writeToNBT(*nbttagcompound);
            NBTTagCompound nbttagcompound1;
            nbttagcompound1.setTag("data", nbttagcompound);
            CompressedStreamTools::writeCompressed(nbttagcompound1, file1);
        }
    } catch (const std::exception& exception) {
        Log::warn("Failed to save data {}: {}", data.mapName, exception.what());
    }
}

void MapStorage::loadIdCounts()
{
    try {
        idCounts.clear();
        if (!saveHandler)
            return;
        const std::filesystem::path file1 = saveHandler->getMapFileFromName("idcounts");
        if (!file1.empty() && std::filesystem::exists(file1)) {
            const auto nbttagcompound = CompressedStreamTools::read(file1);
            for (const std::string& s : nbttagcompound->getKeySet()) {
                const auto nbtbase = nbttagcompound->getTag(s);
                if (nbtbase && nbtbase->getId() == 2)
                    idCounts.put(s, nbttagcompound->getShort(s));
            }
        }
    } catch (const std::exception& exception) {
        Log::warn("Failed to load idcounts: {}", exception.what());
    }
}

int MapStorage::getUniqueDataId(const std::string& key)
{
    int16_t oshort = 0;
    if (const int16_t* found = idCounts.find(key))
        oshort = static_cast<int16_t>(*found + 1);
    idCounts.put(key, oshort);
    if (!saveHandler)
        return oshort;
    try {
        const std::filesystem::path file1 = saveHandler->getMapFileFromName("idcounts");
        if (!file1.empty()) {
            NBTTagCompound nbttagcompound;
            for (const auto& entry : idCounts)
                nbttagcompound.setShort(entry.first, entry.second);
            CompressedStreamTools::write(nbttagcompound, file1);
        }
    } catch (const std::exception& exception) {
        Log::warn("Failed to save idcounts: {}", exception.what());
    }
    return oshort;
}

} // namespace mc

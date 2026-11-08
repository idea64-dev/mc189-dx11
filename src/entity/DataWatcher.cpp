#include "entity/DataWatcher.h"

#include "entity/Entity.h"
#include "item/ItemStack.h"

#include <stdexcept>

namespace mc {

void DataWatcher::addObject(int id, Value object)
{
    const int type = static_cast<int>(object.index());
    if (id > 31)
        throw std::invalid_argument("Data value id is too big with " + std::to_string(id) + "! (Max is 31)");
    if (watchedObjects.contains(id))
        throw std::invalid_argument("Duplicate id value for " + std::to_string(id) + "!");
    watchedObjects.put(id, WatchableObject(type, id, std::move(object)));
    isBlank = false;
}

void DataWatcher::addObjectByDataType(int id, int type)
{
    // Java stores a null object; the default of the type stands in for it.
    Value value;
    switch (type) {
    case 0: value = int8_t(0); break;
    case 1: value = int16_t(0); break;
    case 2: value = int32_t(0); break;
    case 3: value = 0.0f; break;
    case 4: value = std::string(); break;
    case 5: value = std::shared_ptr<ItemStack>(); break;
    case 6: value = BlockPos(); break;
    default: value = Rotations(); break;
    }
    watchedObjects.put(id, WatchableObject(type, id, std::move(value)));
    isBlank = false;
}

const DataWatcher::WatchableObject& DataWatcher::get(int id) const
{
    const WatchableObject* object = watchedObjects.find(id);
    if (!object)
        throw std::out_of_range("Getting synched entity data: no data value " + std::to_string(id));
    return *object;
}

DataWatcher::WatchableObject& DataWatcher::get(int id)
{
    return const_cast<WatchableObject&>(static_cast<const DataWatcher*>(this)->get(id));
}

std::shared_ptr<ItemStack> DataWatcher::getWatchableObjectItemStack(int id) const
{
    return std::get<std::shared_ptr<ItemStack>>(get(id).getObject());
}

void DataWatcher::updateObject(int id, Value newData)
{
    WatchableObject& object = get(id);
    if (newData != object.getObject()) {
        object.setObject(std::move(newData));
        owner->onDataWatcherUpdate(id);
        object.setWatched(true);
        objectChanged = true;
    }
}

void DataWatcher::setObjectWatched(int id)
{
    get(id).watched = true;
    objectChanged = true;
}

std::vector<DataWatcher::WatchableObject> DataWatcher::getChanged()
{
    std::vector<WatchableObject> list;
    if (objectChanged) {
        for (auto& entry : watchedObjects) {
            WatchableObject& object = const_cast<WatchableObject&>(entry.second);
            if (object.isWatched()) {
                object.setWatched(false);
                list.push_back(object);
            }
        }
    }
    objectChanged = false;
    return list;
}

std::vector<DataWatcher::WatchableObject> DataWatcher::getAllWatched() const
{
    std::vector<WatchableObject> list;
    for (const auto& entry : watchedObjects)
        list.push_back(entry.second);
    return list;
}

void DataWatcher::updateWatchedObjectsFromList(const std::vector<WatchableObject>& list)
{
    for (const WatchableObject& object : list) {
        if (WatchableObject* existing = watchedObjects.find(object.getDataValueId())) {
            existing->setObject(object.getObject());
            owner->onDataWatcherUpdate(object.getDataValueId());
        }
    }
    objectChanged = true;
}

} // namespace mc

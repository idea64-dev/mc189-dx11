#pragma once

// Original: net/minecraft/entity/DataWatcher.java (and util/Rotations.java) -- the entity values synchronised
// to clients. Packets carry the WatchableObjects themselves (the local connection doesn't serialise).

#include "core/JavaHashMap.h"
#include "util/BlockPos.h"

#include <cstdint>
#include <memory>
#include <optional>
#include <string>
#include <variant>
#include <vector>

namespace mc {

class Entity;
class ItemStack;

struct Rotations {
    float x = 0.0f;
    float y = 0.0f;
    float z = 0.0f;
    bool operator==(const Rotations&) const = default;
};

class DataWatcher {
public:
    // The Java types by id: Byte, Short, Integer, Float, String, ItemStack, BlockPos, Rotations. ItemStacks
    // compare by identity (ItemStack doesn't override equals).
    using Value = std::variant<int8_t, int16_t, int32_t, float, std::string, std::shared_ptr<ItemStack>, BlockPos, Rotations>;

    struct WatchableObject {
        WatchableObject(int type, int id, Value object)
            : objectType(type)
            , dataValueId(id)
            , watchedObject(std::move(object))
        {
        }

        int getDataValueId() const { return dataValueId; }
        const Value& getObject() const { return watchedObject; }
        void setObject(Value object) { watchedObject = std::move(object); }
        int getObjectType() const { return objectType; }
        bool isWatched() const { return watched; }
        void setWatched(bool value) { watched = value; }

        int objectType;
        int dataValueId;
        Value watchedObject;
        bool watched = true;
    };

    explicit DataWatcher(Entity* ownerIn)
        : owner(ownerIn)
    {
    }

    // Throws std::invalid_argument for ids above 31 or taken ids, as Java.
    void addObject(int id, Value object);
    void addObjectByDataType(int id, int type);
    int8_t getWatchableObjectByte(int id) const { return std::get<int8_t>(get(id).getObject()); }
    int16_t getWatchableObjectShort(int id) const { return std::get<int16_t>(get(id).getObject()); }
    int32_t getWatchableObjectInt(int id) const { return std::get<int32_t>(get(id).getObject()); }
    float getWatchableObjectFloat(int id) const { return std::get<float>(get(id).getObject()); }
    const std::string& getWatchableObjectString(int id) const { return std::get<std::string>(get(id).getObject()); }
    std::shared_ptr<ItemStack> getWatchableObjectItemStack(int id) const;
    Rotations getWatchableObjectRotations(int id) const { return std::get<Rotations>(get(id).getObject()); }
    void updateObject(int id, Value newData);
    void setObjectWatched(int id);
    bool hasObjectChanged() const { return objectChanged; }
    // Java: getChanged -- copies of the changed entries (null in Java when none: empty here).
    std::vector<WatchableObject> getChanged();
    std::vector<WatchableObject> getAllWatched() const;
    void updateWatchedObjectsFromList(const std::vector<WatchableObject>& list);
    bool getIsBlank() const { return isBlank; }
    void func_111144_e() { objectChanged = false; }

private:
    const WatchableObject& get(int id) const;
    WatchableObject& get(int id);

    Entity* owner;
    bool isBlank = true;
    JavaHashMap<int, WatchableObject> watchedObjects;
    bool objectChanged = false;
};

} // namespace mc

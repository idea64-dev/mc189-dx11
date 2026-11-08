#pragma once

// Original: net/minecraft/util/ObjectIntIdentityMap.java, RegistrySimple.java, RegistryNamespaced.java and
// RegistryNamespacedDefaultedByKey.java.
// Objects are registered by pointer; the registry doesn't own them. Iteration is by id, like Java's.
// Not reproduced yet: the iteration order of getKeys() (a Guava HashBiMap in Java; used by command completion,
// stage 8) -- keys come in registration order here.

#include <optional>
#include <unordered_map>
#include <vector>

namespace mc {

template <class T>
class ObjectIntIdentityMap {
public:
    void put(const T* key, int value)
    {
        identityMap[key] = value;
        if (objectList.size() <= static_cast<size_t>(value))
            objectList.resize(static_cast<size_t>(value) + 1, nullptr);
        objectList[static_cast<size_t>(value)] = key;
    }

    // -1 when absent.
    int get(const T* key) const
    {
        const auto it = identityMap.find(key);
        return it == identityMap.end() ? -1 : it->second;
    }

    const T* getByValue(int value) const
    {
        return value >= 0 && static_cast<size_t>(value) < objectList.size() ? objectList[static_cast<size_t>(value)] : nullptr;
    }

    // Non-null entries in id order.
    std::vector<const T*> values() const
    {
        std::vector<const T*> result;
        for (const T* object : objectList) {
            if (object)
                result.push_back(object);
        }
        return result;
    }

private:
    std::unordered_map<const T*, int> identityMap;
    std::vector<const T*> objectList;
};

template <class K, class V>
class RegistryNamespaced {
public:
    virtual ~RegistryNamespaced() = default;

    virtual void registerObject(int id, const K& key, V* value)
    {
        underlyingIntegerMap.put(value, id);
        putObject(key, value);
    }

    void putObject(const K& key, V* value)
    {
        if (const auto it = byKey.find(key); it == byKey.end())
            keys.push_back(key);
        else
            inverse.erase(it->second);
        byKey.insert_or_assign(key, value);
        inverse.insert_or_assign(value, key);
    }

    virtual V* getObject(const K& name) const
    {
        const auto it = byKey.find(name);
        return it == byKey.end() ? nullptr : it->second;
    }

    // Java returns null for unregistered objects; here an empty optional.
    std::optional<K> getNameForObject(const V* value) const
    {
        const auto it = inverse.find(value);
        return it == inverse.end() ? std::nullopt : std::optional<K>(it->second);
    }

    bool containsKey(const K& key) const { return byKey.contains(key); }
    int getIDForObject(const V* value) const { return underlyingIntegerMap.get(value); }
    virtual V* getObjectById(int id) const { return const_cast<V*>(underlyingIntegerMap.getByValue(id)); }
    const std::vector<K>& getKeys() const { return keys; }

    // By id.
    std::vector<V*> values() const
    {
        std::vector<V*> result;
        for (const V* value : underlyingIntegerMap.values())
            result.push_back(const_cast<V*>(value));
        return result;
    }

protected:
    ObjectIntIdentityMap<V> underlyingIntegerMap;
    std::unordered_map<K, V*> byKey;
    std::unordered_map<const V*, K> inverse;
    std::vector<K> keys;
};

template <class K, class V>
class RegistryNamespacedDefaultedByKey : public RegistryNamespaced<K, V> {
public:
    explicit RegistryNamespacedDefaultedByKey(K defaultValueKeyIn)
        : defaultValueKey(std::move(defaultValueKeyIn))
    {
    }

    void registerObject(int id, const K& key, V* value) override
    {
        if (defaultValueKey == key)
            defaultValue = value;
        RegistryNamespaced<K, V>::registerObject(id, key, value);
    }

    V* getObject(const K& name) const override
    {
        V* v = RegistryNamespaced<K, V>::getObject(name);
        return v ? v : defaultValue;
    }

    V* getObjectById(int id) const override
    {
        V* v = RegistryNamespaced<K, V>::getObjectById(id);
        return v ? v : defaultValue;
    }

private:
    K defaultValueKey;
    V* defaultValue = nullptr;
};

} // namespace mc

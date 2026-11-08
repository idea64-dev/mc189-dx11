#pragma once

// Original: net/minecraft/nbt/NBTTagCompound.java and NBTTagList.java
// The compound keeps Java's HashMap order, so files and toString() list the keys in the same order as the
// original game.

#include "core/JavaHashMap.h"
#include "nbt/NBTBase.h"

#include <memory>
#include <string>
#include <vector>

namespace mc {

class NBTTagList;

class NBTTagCompound : public NBTBase {
public:
    void write(DataOutput& output) const override;
    void read(DataInput& input, int depth, NBTSizeTracker& sizeTracker) override;
    std::vector<std::string> getKeySet() const;
    int8_t getId() const override { return 10; }

    void setTag(const std::string& key, std::shared_ptr<NBTBase> value);
    void setByte(const std::string& key, int8_t value);
    void setShort(const std::string& key, int16_t value);
    void setInteger(const std::string& key, int32_t value);
    void setLong(const std::string& key, int64_t value);
    void setFloat(const std::string& key, float value);
    void setDouble(const std::string& key, double value);
    void setString(const std::string& key, const std::string& value);
    void setByteArray(const std::string& key, std::vector<uint8_t> value);
    void setIntArray(const std::string& key, std::vector<int32_t> value);
    void setBoolean(const std::string& key, bool value) { setByte(key, value ? 1 : 0); }

    std::shared_ptr<NBTBase> getTag(const std::string& key) const;
    int8_t getTagId(const std::string& key) const;
    bool hasKey(const std::string& key) const { return tagMap.contains(key); }
    // Type 99 matches any number.
    bool hasKey(const std::string& key, int type) const;
    int8_t getByte(const std::string& key) const;
    int16_t getShort(const std::string& key) const;
    int32_t getInteger(const std::string& key) const;
    int64_t getLong(const std::string& key) const;
    float getFloat(const std::string& key) const;
    double getDouble(const std::string& key) const;
    std::string getString(const std::string& key) const;
    std::vector<uint8_t> getByteArray(const std::string& key) const;
    std::vector<int32_t> getIntArray(const std::string& key) const;
    // The stored compound (shared, so changes show in this tag), or a new detached one.
    std::shared_ptr<NBTTagCompound> getCompoundTag(const std::string& key) const;
    // The stored list if it holds `type` elements or is empty, otherwise a new detached one.
    std::shared_ptr<NBTTagList> getTagList(const std::string& key, int type) const;
    bool getBoolean(const std::string& key) const { return getByte(key) != 0; }
    void removeTag(const std::string& key) { tagMap.erase(key); }

    std::string toString() const override;
    bool hasNoTags() const override { return tagMap.empty(); }
    std::shared_ptr<NBTBase> copy() const override;
    bool equals(const NBTBase& other) const override;
    int32_t hashCode() const override;
    void merge(const NBTTagCompound& other);

    // NBTBase::getString() takes no key; keep it reachable next to getString(key).
    using NBTBase::getString;

    static std::shared_ptr<NBTBase> readNBT(int8_t id, const std::string& key, DataInput& input, int depth, NBTSizeTracker& sizeTracker);

private:
    JavaHashMap<std::string, std::shared_ptr<NBTBase>> tagMap;
};

class NBTTagList : public NBTBase {
public:
    void write(DataOutput& output) const override;
    void read(DataInput& input, int depth, NBTSizeTracker& sizeTracker) override;
    int8_t getId() const override { return 9; }
    std::string toString() const override;

    // Refuses (with a warning) end tags and tags of another type than the list's.
    void appendTag(std::shared_ptr<NBTBase> nbt);
    void set(int idx, std::shared_ptr<NBTBase> nbt);
    // Throws std::out_of_range (Java: IndexOutOfBoundsException).
    std::shared_ptr<NBTBase> removeTag(int i);
    bool hasNoTags() const override { return tagList.empty(); }
    std::shared_ptr<NBTTagCompound> getCompoundTagAt(int i) const;
    std::vector<int32_t> getIntArrayAt(int i) const;
    double getDoubleAt(int i) const;
    float getFloatAt(int i) const;
    std::string getStringTagAt(int i) const;
    std::shared_ptr<NBTBase> get(int idx) const;
    int tagCount() const { return static_cast<int>(tagList.size()); }
    std::shared_ptr<NBTBase> copy() const override;
    bool equals(const NBTBase& other) const override;
    int32_t hashCode() const override;
    int getTagType() const { return tagType; }

private:
    std::vector<std::shared_ptr<NBTBase>> tagList;
    // Java writes the first element's type here on every write().
    mutable int8_t tagType = 0;
};

} // namespace mc

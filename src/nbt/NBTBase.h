#pragma once

// Original: net/minecraft/nbt/NBTBase.java, NBTSizeTracker.java, NBTException.java and the simple tags
// (NBTTagEnd, NBTTagByte, NBTTagShort, NBTTagInt, NBTTagLong, NBTTagFloat, NBTTagDouble, NBTTagByteArray,
// NBTTagString, NBTTagIntArray).
// Tags are held by shared_ptr: Java code hands out and stores references to the same tag objects (getCompoundTag
// returns the live child), and the C++ code keeps that sharing.

#include "core/DataStream.h"

#include <cstdint>
#include <memory>
#include <stdexcept>
#include <string>
#include <vector>

namespace mc {

// Java: NBTException, thrown by JsonToNBT.
class NBTException : public std::runtime_error {
public:
    using std::runtime_error::runtime_error;
};

class NBTSizeTracker {
public:
    explicit NBTSizeTracker(int64_t max)
        : max(max)
    {
    }

    // Java: NBTSizeTracker.INFINITE
    static NBTSizeTracker infinite();

    // Throws std::runtime_error past the limit.
    void read(int64_t bits);

private:
    NBTSizeTracker(int64_t maxIn, bool unlimitedIn)
        : max(maxIn)
        , unlimited(unlimitedIn)
    {
    }

    int64_t max;
    int64_t readBytes = 0;
    bool unlimited = false;
};

class NBTBase {
public:
    static constexpr const char* NBT_TYPES[12] { "END", "BYTE", "SHORT", "INT", "LONG", "FLOAT", "DOUBLE", "BYTE[]", "STRING", "LIST", "COMPOUND", "INT[]" };

    virtual ~NBTBase() = default;

    virtual void write(DataOutput& output) const = 0;
    virtual void read(DataInput& input, int depth, NBTSizeTracker& sizeTracker) = 0;
    virtual std::string toString() const = 0;
    virtual int8_t getId() const = 0;
    virtual std::shared_ptr<NBTBase> copy() const = 0;
    virtual bool hasNoTags() const { return false; }
    virtual bool equals(const NBTBase& other) const { return getId() == other.getId(); }
    virtual int32_t hashCode() const { return getId(); }
    // The text of a string tag, toString() for the others.
    virtual std::string getString() const { return toString(); }

    // Null for an unknown id.
    static std::shared_ptr<NBTBase> createNewByType(int8_t id);
};

class NBTPrimitive : public NBTBase {
public:
    virtual int64_t getLong() const = 0;
    virtual int32_t getInt() const = 0;
    virtual int16_t getShort() const = 0;
    virtual int8_t getByte() const = 0;
    virtual double getDouble() const = 0;
    virtual float getFloat() const = 0;
};

class NBTTagEnd : public NBTBase {
public:
    void write(DataOutput&) const override { }
    void read(DataInput& input, int depth, NBTSizeTracker& sizeTracker) override;
    std::string toString() const override { return "END"; }
    int8_t getId() const override { return 0; }
    std::shared_ptr<NBTBase> copy() const override { return std::make_shared<NBTTagEnd>(); }
};

class NBTTagByte : public NBTPrimitive {
public:
    explicit NBTTagByte(int8_t data = 0)
        : data(data)
    {
    }
    void write(DataOutput& output) const override { output.writeByte(data); }
    void read(DataInput& input, int depth, NBTSizeTracker& sizeTracker) override;
    std::string toString() const override;
    int8_t getId() const override { return 1; }
    std::shared_ptr<NBTBase> copy() const override { return std::make_shared<NBTTagByte>(data); }
    bool equals(const NBTBase& other) const override;
    int32_t hashCode() const override { return NBTBase::hashCode() ^ data; }
    int64_t getLong() const override { return data; }
    int32_t getInt() const override { return data; }
    int16_t getShort() const override { return data; }
    int8_t getByte() const override { return data; }
    double getDouble() const override { return data; }
    float getFloat() const override { return data; }

private:
    int8_t data;
};

class NBTTagShort : public NBTPrimitive {
public:
    explicit NBTTagShort(int16_t data = 0)
        : data(data)
    {
    }
    void write(DataOutput& output) const override { output.writeShort(data); }
    void read(DataInput& input, int depth, NBTSizeTracker& sizeTracker) override;
    std::string toString() const override;
    int8_t getId() const override { return 2; }
    std::shared_ptr<NBTBase> copy() const override { return std::make_shared<NBTTagShort>(data); }
    bool equals(const NBTBase& other) const override;
    int32_t hashCode() const override { return NBTBase::hashCode() ^ data; }
    int64_t getLong() const override { return data; }
    int32_t getInt() const override { return data; }
    int16_t getShort() const override { return data; }
    int8_t getByte() const override { return static_cast<int8_t>(data & 255); }
    double getDouble() const override { return data; }
    float getFloat() const override { return data; }

private:
    int16_t data;
};

class NBTTagInt : public NBTPrimitive {
public:
    explicit NBTTagInt(int32_t data = 0)
        : data(data)
    {
    }
    void write(DataOutput& output) const override { output.writeInt(data); }
    void read(DataInput& input, int depth, NBTSizeTracker& sizeTracker) override;
    std::string toString() const override;
    int8_t getId() const override { return 3; }
    std::shared_ptr<NBTBase> copy() const override { return std::make_shared<NBTTagInt>(data); }
    bool equals(const NBTBase& other) const override;
    int32_t hashCode() const override { return NBTBase::hashCode() ^ data; }
    int64_t getLong() const override { return data; }
    int32_t getInt() const override { return data; }
    int16_t getShort() const override { return static_cast<int16_t>(data & 65535); }
    int8_t getByte() const override { return static_cast<int8_t>(data & 255); }
    double getDouble() const override { return data; }
    float getFloat() const override { return static_cast<float>(data); }

private:
    int32_t data;
};

class NBTTagLong : public NBTPrimitive {
public:
    explicit NBTTagLong(int64_t data = 0)
        : data(data)
    {
    }
    void write(DataOutput& output) const override { output.writeLong(data); }
    void read(DataInput& input, int depth, NBTSizeTracker& sizeTracker) override;
    std::string toString() const override;
    int8_t getId() const override { return 4; }
    std::shared_ptr<NBTBase> copy() const override { return std::make_shared<NBTTagLong>(data); }
    bool equals(const NBTBase& other) const override;
    int32_t hashCode() const override;
    int64_t getLong() const override { return data; }
    int32_t getInt() const override { return static_cast<int32_t>(data); }
    int16_t getShort() const override { return static_cast<int16_t>(data & 65535); }
    int8_t getByte() const override { return static_cast<int8_t>(data & 255); }
    double getDouble() const override { return static_cast<double>(data); }
    float getFloat() const override { return static_cast<float>(data); }

private:
    int64_t data;
};

class NBTTagFloat : public NBTPrimitive {
public:
    explicit NBTTagFloat(float data = 0.0f)
        : data(data)
    {
    }
    void write(DataOutput& output) const override { output.writeFloat(data); }
    void read(DataInput& input, int depth, NBTSizeTracker& sizeTracker) override;
    std::string toString() const override;
    int8_t getId() const override { return 5; }
    std::shared_ptr<NBTBase> copy() const override { return std::make_shared<NBTTagFloat>(data); }
    bool equals(const NBTBase& other) const override;
    int32_t hashCode() const override;
    int64_t getLong() const override;
    int32_t getInt() const override;
    int16_t getShort() const override { return static_cast<int16_t>(getInt() & 65535); }
    int8_t getByte() const override { return static_cast<int8_t>(getInt() & 255); }
    double getDouble() const override { return data; }
    float getFloat() const override { return data; }

private:
    float data;
};

class NBTTagDouble : public NBTPrimitive {
public:
    explicit NBTTagDouble(double data = 0.0)
        : data(data)
    {
    }
    void write(DataOutput& output) const override { output.writeDouble(data); }
    void read(DataInput& input, int depth, NBTSizeTracker& sizeTracker) override;
    std::string toString() const override;
    int8_t getId() const override { return 6; }
    std::shared_ptr<NBTBase> copy() const override { return std::make_shared<NBTTagDouble>(data); }
    bool equals(const NBTBase& other) const override;
    int32_t hashCode() const override;
    int64_t getLong() const override;
    int32_t getInt() const override;
    int16_t getShort() const override { return static_cast<int16_t>(getInt() & 65535); }
    int8_t getByte() const override { return static_cast<int8_t>(getInt() & 255); }
    double getDouble() const override { return data; }
    float getFloat() const override { return static_cast<float>(data); }

private:
    double data;
};

class NBTTagByteArray : public NBTBase {
public:
    NBTTagByteArray() = default;
    explicit NBTTagByteArray(std::vector<uint8_t> data)
        : data(std::move(data))
    {
    }
    void write(DataOutput& output) const override;
    void read(DataInput& input, int depth, NBTSizeTracker& sizeTracker) override;
    std::string toString() const override { return "[" + std::to_string(data.size()) + " bytes]"; }
    int8_t getId() const override { return 7; }
    std::shared_ptr<NBTBase> copy() const override { return std::make_shared<NBTTagByteArray>(data); }
    bool equals(const NBTBase& other) const override;
    int32_t hashCode() const override;
    const std::vector<uint8_t>& getByteArray() const { return data; }
    std::vector<uint8_t>& getByteArray() { return data; }

private:
    std::vector<uint8_t> data;
};

class NBTTagString : public NBTBase {
public:
    NBTTagString() = default;
    explicit NBTTagString(std::string data)
        : data(std::move(data))
    {
    }
    void write(DataOutput& output) const override { output.writeUTF(data); }
    void read(DataInput& input, int depth, NBTSizeTracker& sizeTracker) override;
    std::string toString() const override;
    int8_t getId() const override { return 8; }
    std::shared_ptr<NBTBase> copy() const override { return std::make_shared<NBTTagString>(data); }
    bool hasNoTags() const override { return data.empty(); }
    bool equals(const NBTBase& other) const override;
    int32_t hashCode() const override;
    std::string getString() const override { return data; }

private:
    std::string data;
};

class NBTTagIntArray : public NBTBase {
public:
    NBTTagIntArray() = default;
    explicit NBTTagIntArray(std::vector<int32_t> intArray)
        : intArray(std::move(intArray))
    {
    }
    void write(DataOutput& output) const override;
    void read(DataInput& input, int depth, NBTSizeTracker& sizeTracker) override;
    std::string toString() const override;
    int8_t getId() const override { return 11; }
    std::shared_ptr<NBTBase> copy() const override { return std::make_shared<NBTTagIntArray>(intArray); }
    bool equals(const NBTBase& other) const override;
    int32_t hashCode() const override;
    const std::vector<int32_t>& getIntArray() const { return intArray; }
    std::vector<int32_t>& getIntArray() { return intArray; }

private:
    std::vector<int32_t> intArray;
};

} // namespace mc

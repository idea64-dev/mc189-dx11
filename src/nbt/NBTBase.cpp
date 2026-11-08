#include "nbt/NBTBase.h"

#include "core/Java.h"
#include "core/JavaString.h"
#include "nbt/NBTTagCompound.h"
#include "util/MathHelper.h"

#include <cmath>
#include <limits>

namespace mc {

namespace {

template <class T>
const T* same(const NBTBase& self, const NBTBase& other)
{
    return self.getId() == other.getId() ? static_cast<const T*>(&other) : nullptr;
}

int32_t floatToIntBits(float value)
{
    return value != value ? 0x7fc00000 : java::floatToRawIntBits(value);
}

int64_t doubleToLongBits(double value)
{
    return value != value ? 0x7ff8000000000000LL : java::doubleToRawLongBits(value);
}

} // namespace

// --- NBTSizeTracker ---

NBTSizeTracker NBTSizeTracker::infinite()
{
    return NBTSizeTracker(0, true);
}

void NBTSizeTracker::read(int64_t bits)
{
    if (unlimited)
        return;
    readBytes += bits / 8;
    if (readBytes > max)
        throw std::runtime_error("Tried to read NBT tag that was too big; tried to allocate: " + std::to_string(readBytes) + "bytes where max allowed: " + std::to_string(max));
}

// --- NBTBase ---

std::shared_ptr<NBTBase> NBTBase::createNewByType(int8_t id)
{
    switch (id) {
    case 0: return std::make_shared<NBTTagEnd>();
    case 1: return std::make_shared<NBTTagByte>();
    case 2: return std::make_shared<NBTTagShort>();
    case 3: return std::make_shared<NBTTagInt>();
    case 4: return std::make_shared<NBTTagLong>();
    case 5: return std::make_shared<NBTTagFloat>();
    case 6: return std::make_shared<NBTTagDouble>();
    case 7: return std::make_shared<NBTTagByteArray>();
    case 8: return std::make_shared<NBTTagString>();
    case 9: return std::make_shared<NBTTagList>();
    case 10: return std::make_shared<NBTTagCompound>();
    case 11: return std::make_shared<NBTTagIntArray>();
    default: return nullptr;
    }
}

void NBTTagEnd::read(DataInput& /*input*/, int /*depth*/, NBTSizeTracker& sizeTracker)
{
    sizeTracker.read(64);
}

// --- Byte ---

void NBTTagByte::read(DataInput& input, int /*depth*/, NBTSizeTracker& sizeTracker)
{
    sizeTracker.read(72);
    data = input.readByte();
}

std::string NBTTagByte::toString() const
{
    return std::to_string(data) + "b";
}

bool NBTTagByte::equals(const NBTBase& other) const
{
    const auto* tag = same<NBTTagByte>(*this, other);
    return tag != nullptr && data == tag->data;
}

// --- Short ---

void NBTTagShort::read(DataInput& input, int /*depth*/, NBTSizeTracker& sizeTracker)
{
    sizeTracker.read(80);
    data = input.readShort();
}

std::string NBTTagShort::toString() const
{
    return std::to_string(data) + "s";
}

bool NBTTagShort::equals(const NBTBase& other) const
{
    const auto* tag = same<NBTTagShort>(*this, other);
    return tag != nullptr && data == tag->data;
}

// --- Int ---

void NBTTagInt::read(DataInput& input, int /*depth*/, NBTSizeTracker& sizeTracker)
{
    sizeTracker.read(96);
    data = input.readInt();
}

std::string NBTTagInt::toString() const
{
    return std::to_string(data);
}

bool NBTTagInt::equals(const NBTBase& other) const
{
    const auto* tag = same<NBTTagInt>(*this, other);
    return tag != nullptr && data == tag->data;
}

// --- Long ---

void NBTTagLong::read(DataInput& input, int /*depth*/, NBTSizeTracker& sizeTracker)
{
    sizeTracker.read(128);
    data = input.readLong();
}

std::string NBTTagLong::toString() const
{
    return std::to_string(data) + "L";
}

bool NBTTagLong::equals(const NBTBase& other) const
{
    const auto* tag = same<NBTTagLong>(*this, other);
    return tag != nullptr && data == tag->data;
}

int32_t NBTTagLong::hashCode() const
{
    return NBTBase::hashCode() ^ static_cast<int32_t>(data ^ java::ushr(data, 32));
}

// --- Float ---

void NBTTagFloat::read(DataInput& input, int /*depth*/, NBTSizeTracker& sizeTracker)
{
    sizeTracker.read(96);
    data = input.readFloat();
}

std::string NBTTagFloat::toString() const
{
    return java::toString(data) + "f";
}

bool NBTTagFloat::equals(const NBTBase& other) const
{
    const auto* tag = same<NBTTagFloat>(*this, other);
    return tag != nullptr && data == tag->data;
}

int32_t NBTTagFloat::hashCode() const
{
    return NBTBase::hashCode() ^ floatToIntBits(data);
}

int64_t NBTTagFloat::getLong() const
{
    return java::f2l(data);
}

int32_t NBTTagFloat::getInt() const
{
    return MathHelper::floor_float(data);
}

// --- Double ---

void NBTTagDouble::read(DataInput& input, int /*depth*/, NBTSizeTracker& sizeTracker)
{
    sizeTracker.read(128);
    data = input.readDouble();
}

std::string NBTTagDouble::toString() const
{
    return java::toString(data) + "d";
}

bool NBTTagDouble::equals(const NBTBase& other) const
{
    const auto* tag = same<NBTTagDouble>(*this, other);
    return tag != nullptr && data == tag->data;
}

int32_t NBTTagDouble::hashCode() const
{
    const int64_t i = doubleToLongBits(data);
    return NBTBase::hashCode() ^ static_cast<int32_t>(i ^ java::ushr(i, 32));
}

int64_t NBTTagDouble::getLong() const
{
    return java::d2l(std::floor(data));
}

int32_t NBTTagDouble::getInt() const
{
    return MathHelper::floor_double(data);
}

// --- Byte array ---

void NBTTagByteArray::write(DataOutput& output) const
{
    output.writeInt(static_cast<int32_t>(data.size()));
    output.write(data);
}

void NBTTagByteArray::read(DataInput& input, int /*depth*/, NBTSizeTracker& sizeTracker)
{
    sizeTracker.read(192);
    const int32_t i = input.readInt();
    sizeTracker.read(java::mul(8, i));
    if (i < 0)
        throw std::runtime_error("java.lang.NegativeArraySizeException");
    if (static_cast<size_t>(i) > input.remaining())
        throw IOException("java.io.EOFException");
    data.assign(static_cast<size_t>(i), 0);
    input.readFully(data);
}

bool NBTTagByteArray::equals(const NBTBase& other) const
{
    const auto* tag = same<NBTTagByteArray>(*this, other);
    return tag != nullptr && data == tag->data;
}

int32_t NBTTagByteArray::hashCode() const
{
    int32_t result = 1;
    for (const uint8_t b : data)
        result = java::add(java::mul(31, result), static_cast<int8_t>(b));
    return NBTBase::hashCode() ^ result;
}

// --- String ---

void NBTTagString::read(DataInput& input, int /*depth*/, NBTSizeTracker& sizeTracker)
{
    sizeTracker.read(288);
    data = input.readUTF();
    sizeTracker.read(java::mul(16, static_cast<int32_t>(java::utf16Length(data))));
}

std::string NBTTagString::toString() const
{
    std::string escaped = "\"";
    for (const char c : data) {
        if (c == '"')
            escaped += "\\\"";
        else
            escaped += c;
    }
    return escaped + "\"";
}

bool NBTTagString::equals(const NBTBase& other) const
{
    const auto* tag = same<NBTTagString>(*this, other);
    return tag != nullptr && data == tag->data;
}

int32_t NBTTagString::hashCode() const
{
    return NBTBase::hashCode() ^ java::hashCode(data);
}

// --- Int array ---

void NBTTagIntArray::write(DataOutput& output) const
{
    output.writeInt(static_cast<int32_t>(intArray.size()));
    for (const int32_t value : intArray)
        output.writeInt(value);
}

void NBTTagIntArray::read(DataInput& input, int /*depth*/, NBTSizeTracker& sizeTracker)
{
    sizeTracker.read(192);
    const int32_t i = input.readInt();
    sizeTracker.read(java::mul(32, i));
    if (i < 0)
        throw std::runtime_error("java.lang.NegativeArraySizeException");
    if (static_cast<size_t>(i) > input.remaining() / 4)
        throw IOException("java.io.EOFException");
    intArray.assign(static_cast<size_t>(i), 0);
    for (int32_t& value : intArray)
        value = input.readInt();
}

std::string NBTTagIntArray::toString() const
{
    std::string s = "[";
    for (const int32_t value : intArray)
        s += std::to_string(value) + ",";
    return s + "]";
}

bool NBTTagIntArray::equals(const NBTBase& other) const
{
    const auto* tag = same<NBTTagIntArray>(*this, other);
    return tag != nullptr && intArray == tag->intArray;
}

int32_t NBTTagIntArray::hashCode() const
{
    int32_t result = 1;
    for (const int32_t value : intArray)
        result = java::add(java::mul(31, result), value);
    return NBTBase::hashCode() ^ result;
}

} // namespace mc

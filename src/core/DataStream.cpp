#include "core/DataStream.h"

#include "core/StringUtil.h"

#include <bit>
#include <cstring>

namespace mc {

const uint8_t* DataInput::take(size_t count)
{
    if (data.size() - pos < count)
        throw IOException("java.io.EOFException");
    const uint8_t* p = data.data() + pos;
    pos += count;
    return p;
}

int8_t DataInput::readByte()
{
    return static_cast<int8_t>(*take(1));
}

int16_t DataInput::readShort()
{
    const uint8_t* p = take(2);
    return static_cast<int16_t>((p[0] << 8) | p[1]);
}

int32_t DataInput::readInt()
{
    const uint8_t* p = take(4);
    return static_cast<int32_t>((static_cast<uint32_t>(p[0]) << 24) | (static_cast<uint32_t>(p[1]) << 16) | (static_cast<uint32_t>(p[2]) << 8) | p[3]);
}

int64_t DataInput::readLong()
{
    const uint64_t high = static_cast<uint32_t>(readInt());
    const uint64_t low = static_cast<uint32_t>(readInt());
    return static_cast<int64_t>((high << 32) | low);
}

float DataInput::readFloat()
{
    return std::bit_cast<float>(readInt());
}

double DataInput::readDouble()
{
    return std::bit_cast<double>(readLong());
}

void DataInput::readFully(std::span<uint8_t> out)
{
    const uint8_t* p = take(out.size());
    if (!out.empty())
        std::memcpy(out.data(), p, out.size());
}

std::string DataInput::readUTF()
{
    const size_t length = readUnsignedShort();
    const uint8_t* p = take(length);
    std::u16string chars;
    chars.reserve(length);
    for (size_t i = 0; i < length;) {
        const uint8_t c = p[i];
        if (c < 0x80) {
            chars.push_back(c);
            i += 1;
        } else if ((c >> 5) == 0x6) {
            if (i + 2 > length || (p[i + 1] & 0xC0) != 0x80)
                throw IOException("java.io.UTFDataFormatException: malformed input around byte " + std::to_string(i));
            chars.push_back(static_cast<char16_t>(((c & 0x1F) << 6) | (p[i + 1] & 0x3F)));
            i += 2;
        } else if ((c >> 4) == 0xE) {
            if (i + 3 > length || (p[i + 1] & 0xC0) != 0x80 || (p[i + 2] & 0xC0) != 0x80)
                throw IOException("java.io.UTFDataFormatException: malformed input around byte " + std::to_string(i));
            chars.push_back(static_cast<char16_t>(((c & 0x0F) << 12) | ((p[i + 1] & 0x3F) << 6) | (p[i + 2] & 0x3F)));
            i += 3;
        } else {
            throw IOException("java.io.UTFDataFormatException: malformed input around byte " + std::to_string(i));
        }
    }
    return fromUtf16(chars);
}

void DataOutput::writeShort(int value)
{
    writeByte(value >> 8);
    writeByte(value);
}

void DataOutput::writeInt(int32_t value)
{
    const auto v = static_cast<uint32_t>(value);
    writeByte(static_cast<int>(v >> 24));
    writeByte(static_cast<int>(v >> 16));
    writeByte(static_cast<int>(v >> 8));
    writeByte(static_cast<int>(v));
}

void DataOutput::writeLong(int64_t value)
{
    writeInt(static_cast<int32_t>(static_cast<uint64_t>(value) >> 32));
    writeInt(static_cast<int32_t>(value));
}

void DataOutput::writeFloat(float value)
{
    // Java's writeFloat uses floatToIntBits, which collapses every NaN to 0x7fc00000.
    writeInt(value != value ? 0x7fc00000 : std::bit_cast<int32_t>(value));
}

void DataOutput::writeDouble(double value)
{
    writeLong(value != value ? 0x7ff8000000000000LL : std::bit_cast<int64_t>(value));
}

void DataOutput::writeUTF(std::string_view text)
{
    const std::u16string chars = toUtf16(text);
    std::vector<uint8_t> encoded;
    encoded.reserve(chars.size());
    for (const char16_t c : chars) {
        if (c >= 0x0001 && c <= 0x007F) {
            encoded.push_back(static_cast<uint8_t>(c));
        } else if (c > 0x07FF) {
            encoded.push_back(static_cast<uint8_t>(0xE0 | ((c >> 12) & 0x0F)));
            encoded.push_back(static_cast<uint8_t>(0x80 | ((c >> 6) & 0x3F)));
            encoded.push_back(static_cast<uint8_t>(0x80 | (c & 0x3F)));
        } else {
            encoded.push_back(static_cast<uint8_t>(0xC0 | ((c >> 6) & 0x1F)));
            encoded.push_back(static_cast<uint8_t>(0x80 | (c & 0x3F)));
        }
    }
    if (encoded.size() > 65535)
        throw IOException("java.io.UTFDataFormatException: encoded string too long: " + std::to_string(encoded.size()) + " bytes");
    writeShort(static_cast<int>(encoded.size()));
    write(encoded);
}

} // namespace mc

#pragma once

// java.io.DataInputStream / DataOutputStream over memory: big-endian primitives and writeUTF/readUTF's
// "modified UTF-8" (2-byte length, U+0000 as C0 80, characters outside the BMP as two 3-byte surrogates).
// Strings are UTF-8 on the C++ side.

#include <cstdint>
#include <span>
#include <stdexcept>
#include <string>
#include <vector>

namespace mc {

// Java: java.io.IOException and its subclasses (EOFException, UTFDataFormatException).
class IOException : public std::runtime_error {
public:
    using std::runtime_error::runtime_error;
};

class DataInput {
public:
    explicit DataInput(std::span<const uint8_t> data)
        : data(data)
    {
    }

    int8_t readByte();
    uint8_t readUnsignedByte() { return static_cast<uint8_t>(readByte()); }
    int16_t readShort();
    uint16_t readUnsignedShort() { return static_cast<uint16_t>(readShort()); }
    int32_t readInt();
    int64_t readLong();
    float readFloat();
    double readDouble();
    bool readBoolean() { return readByte() != 0; }
    void readFully(std::span<uint8_t> out);
    std::string readUTF();

    size_t position() const { return pos; }
    size_t remaining() const { return data.size() - pos; }

private:
    const uint8_t* take(size_t count);

    std::span<const uint8_t> data;
    size_t pos = 0;
};

class DataOutput {
public:
    void writeByte(int value) { bytes.push_back(static_cast<uint8_t>(value)); }
    void writeShort(int value);
    void writeInt(int32_t value);
    void writeLong(int64_t value);
    void writeFloat(float value);
    void writeDouble(double value);
    void writeBoolean(bool value) { writeByte(value ? 1 : 0); }
    void write(std::span<const uint8_t> data) { bytes.insert(bytes.end(), data.begin(), data.end()); }
    // Throws IOException (UTFDataFormatException) when the encoding exceeds 65535 bytes.
    void writeUTF(std::string_view text);

    const std::vector<uint8_t>& getBytes() const { return bytes; }
    std::vector<uint8_t>& getBytes() { return bytes; }

private:
    std::vector<uint8_t> bytes;
};

} // namespace mc

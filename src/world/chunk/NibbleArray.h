#pragma once

// Original: net/minecraft/world/chunk/NibbleArray.java -- 4096 four-bit values, index y << 8 | z << 4 | x.

#include <array>
#include <cstdint>
#include <span>
#include <stdexcept>
#include <string>

namespace mc {

class NibbleArray {
public:
    NibbleArray() { data.fill(0); }
    // Throws std::invalid_argument unless the array has 2048 bytes (Java: IllegalArgumentException).
    explicit NibbleArray(std::span<const uint8_t> storageArray)
    {
        if (storageArray.size() != 2048)
            throw std::invalid_argument("ChunkNibbleArrays should be 2048 bytes not: " + std::to_string(storageArray.size()));
        std::copy(storageArray.begin(), storageArray.end(), data.begin());
    }

    int get(int x, int y, int z) const { return getFromIndex(y << 8 | z << 4 | x); }
    void set(int x, int y, int z, int value) { setIndex(y << 8 | z << 4 | x, value); }
    int getFromIndex(int index) const
    {
        const uint8_t b = data[static_cast<size_t>(index >> 1)];
        return (index & 1) == 0 ? b & 15 : b >> 4 & 15;
    }
    void setIndex(int index, int value)
    {
        uint8_t& b = data[static_cast<size_t>(index >> 1)];
        if ((index & 1) == 0)
            b = static_cast<uint8_t>((b & 240) | (value & 15));
        else
            b = static_cast<uint8_t>((b & 15) | (value & 15) << 4);
    }
    std::array<uint8_t, 2048>& getData() { return data; }
    const std::array<uint8_t, 2048>& getData() const { return data; }

private:
    std::array<uint8_t, 2048> data;
};

} // namespace mc

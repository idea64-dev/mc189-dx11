#pragma once

// Original: net/minecraft/world/chunk/storage/ExtendedBlockStorage.java -- a 16x16x16 section of a chunk: block
// state ids (Block.BLOCK_STATE_IDS), block light and, in dimensions with a sky, sky light.

#include "world/chunk/NibbleArray.h"

#include <array>
#include <cstdint>
#include <memory>

namespace mc {

class Block;
class IBlockState;

class ExtendedBlockStorage {
public:
    ExtendedBlockStorage(int y, bool storeSkylight);

    // Unknown ids read as air.
    const IBlockState* get(int x, int y, int z) const;
    void set(int x, int y, int z, const IBlockState* state);
    Block* getBlockByExtId(int x, int y, int z) const;
    int getExtBlockMetadata(int x, int y, int z) const;
    bool isEmpty() const { return blockRefCount == 0; }
    bool getNeedsRandomTick() const { return tickRefCount > 0; }
    int getYLocation() const { return yBase; }
    void setExtSkylightValue(int x, int y, int z, int value) { skylightArray->set(x, y, z, value); }
    int getExtSkylightValue(int x, int y, int z) const { return skylightArray->get(x, y, z); }
    void setExtBlocklightValue(int x, int y, int z, int value) { blocklightArray->set(x, y, z, value); }
    int getExtBlocklightValue(int x, int y, int z) const { return blocklightArray->get(x, y, z); }
    void removeInvalidBlocks();
    std::array<uint16_t, 4096>& getData() { return data; }
    const std::array<uint16_t, 4096>& getData() const { return data; }
    void setData(const std::array<uint16_t, 4096>& dataArray) { data = dataArray; }
    NibbleArray* getBlocklightArray() const { return blocklightArray.get(); }
    // Null in dimensions without a sky.
    NibbleArray* getSkylightArray() const { return skylightArray.get(); }
    void setBlocklightArray(std::unique_ptr<NibbleArray> array) { blocklightArray = std::move(array); }
    void setSkylightArray(std::unique_ptr<NibbleArray> array) { skylightArray = std::move(array); }

private:
    int yBase;
    int blockRefCount = 0;
    int tickRefCount = 0;
    std::array<uint16_t, 4096> data {};
    std::unique_ptr<NibbleArray> blocklightArray;
    std::unique_ptr<NibbleArray> skylightArray;
};

} // namespace mc

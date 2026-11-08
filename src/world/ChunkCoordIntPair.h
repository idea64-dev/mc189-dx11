#pragma once

// Original: net/minecraft/world/ChunkCoordIntPair.java

#include "core/Java.h"
#include "util/BlockPos.h"

#include <cstdint>
#include <functional>
#include <string>

namespace mc {

struct ChunkCoordIntPair {
    int32_t chunkXPos;
    int32_t chunkZPos;

    constexpr ChunkCoordIntPair(int32_t x, int32_t z)
        : chunkXPos(x)
        , chunkZPos(z)
    {
    }

    // Both coordinates in one long: x in the low 32 bits, z in the high ones.
    static constexpr int64_t chunkXZ2Int(int32_t x, int32_t z)
    {
        return (static_cast<int64_t>(x) & 4294967295LL) | (static_cast<int64_t>(static_cast<uint32_t>(z)) << 32);
    }

    constexpr int32_t hashCode() const
    {
        const int32_t i = java::add(java::mul(1664525, chunkXPos), 1013904223);
        const int32_t j = java::add(java::mul(1664525, chunkZPos ^ -559038737), 1013904223);
        return i ^ j;
    }

    constexpr bool operator==(const ChunkCoordIntPair& other) const = default;
    constexpr int32_t getCenterXPos() const { return (chunkXPos << 4) + 8; }
    constexpr int32_t getCenterZPosition() const { return (chunkZPos << 4) + 8; }
    constexpr int32_t getXStart() const { return chunkXPos << 4; }
    constexpr int32_t getZStart() const { return chunkZPos << 4; }
    constexpr int32_t getXEnd() const { return (chunkXPos << 4) + 15; }
    constexpr int32_t getZEnd() const { return (chunkZPos << 4) + 15; }
    constexpr BlockPos getBlock(int32_t x, int32_t y, int32_t z) const { return BlockPos((chunkXPos << 4) + x, y, (chunkZPos << 4) + z); }
    constexpr BlockPos getCenterBlock(int32_t y) const { return BlockPos(getCenterXPos(), y, getCenterZPosition()); }
    std::string toString() const { return "[" + std::to_string(chunkXPos) + ", " + std::to_string(chunkZPos) + "]"; }
};

} // namespace mc

template <>
struct std::hash<mc::ChunkCoordIntPair> {
    size_t operator()(const mc::ChunkCoordIntPair& pair) const noexcept { return std::hash<int64_t>()(mc::ChunkCoordIntPair::chunkXZ2Int(pair.chunkXPos, pair.chunkZPos)); }
};

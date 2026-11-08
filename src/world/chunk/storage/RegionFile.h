#pragma once

// Original: net/minecraft/world/chunk/storage/RegionFile.java and RegionFileCache.java -- the .mca files: a
// 4 KiB table of sector offsets, a 4 KiB table of timestamps, then 4 KiB sectors holding length-prefixed,
// compressed chunk NBT. Writes go through zlib like Java's DeflaterOutputStream; the compressed bytes differ
// from Java's (miniz instead of zlib), what they decompress to does not.

#include <array>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <memory>
#include <mutex>
#include <optional>
#include <span>
#include <vector>

namespace mc {

class RegionFile {
public:
    explicit RegionFile(const std::filesystem::path& fileNameIn);

    RegionFile(const RegionFile&) = delete;
    RegionFile& operator=(const RegionFile&) = delete;

    // The decompressed chunk data (Java: a DataInputStream over it); nullopt when the chunk is absent, out of the
    // region or unreadable. Throws IOException for corrupt compressed data, like reading Java's stream would.
    std::optional<std::vector<uint8_t>> getChunkData(int x, int z);
    // Java: getChunkDataOutputStream + close(): compresses and stores the chunk data; ignored out of bounds.
    void writeChunkData(int x, int z, std::span<const uint8_t> data);
    bool isChunkSaved(int x, int z) const { return getOffset(x, z) != 0; }
    void close();

private:
    void write(int x, int z, std::span<const uint8_t> data);
    void writeSector(int sectorNumber, std::span<const uint8_t> data);
    static bool outOfBounds(int x, int z) { return x < 0 || x >= 32 || z < 0 || z >= 32; }
    int getOffset(int x, int z) const { return offsets[static_cast<size_t>(x + z * 32)]; }
    void setOffset(int x, int z, int offset);
    void setChunkTimestamp(int x, int z, int timestamp);
    int64_t length();
    void writeIntAt(int64_t position, int32_t value);

    std::filesystem::path fileName;
    std::fstream dataFile;
    std::array<int32_t, 1024> offsets {};
    std::array<int32_t, 1024> chunkTimestamps {};
    std::vector<bool> sectorFree;
    int sizeDelta = 0;
    int64_t lastModified = 0;
    std::recursive_mutex lock;
};

class RegionFileCache {
public:
    // Shared: a region dropped from the cache stays usable by whoever still holds it, as with Java GC.
    static std::shared_ptr<RegionFile> createOrLoadRegionFile(const std::filesystem::path& worldDir, int chunkX, int chunkZ);
    static void clearRegionFileReferences();
    static std::optional<std::vector<uint8_t>> getChunkData(const std::filesystem::path& worldDir, int chunkX, int chunkZ);
    static void writeChunkData(const std::filesystem::path& worldDir, int chunkX, int chunkZ, std::span<const uint8_t> data);
};

} // namespace mc

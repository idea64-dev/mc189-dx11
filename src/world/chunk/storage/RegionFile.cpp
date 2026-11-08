#include "world/chunk/storage/RegionFile.h"

#include "core/Compression.h"
#include "core/DataStream.h"
#include "core/JavaTime.h"
#include "core/Log.h"

#include <map>

namespace mc {

namespace {

std::array<char, 4096> emptySector {};

int32_t readIntAt(std::fstream& file, int64_t position)
{
    file.seekg(position);
    unsigned char b[4] {};
    file.read(reinterpret_cast<char*>(b), 4);
    if (file.gcount() != 4)
        throw IOException("EOF");
    return static_cast<int32_t>(static_cast<uint32_t>(b[0]) << 24 | static_cast<uint32_t>(b[1]) << 16 | static_cast<uint32_t>(b[2]) << 8 | b[3]);
}

} // namespace

RegionFile::RegionFile(const std::filesystem::path& fileNameIn)
    : fileName(fileNameIn)
{
    try {
        std::error_code ec;
        if (std::filesystem::exists(fileNameIn, ec)) {
            const auto time = std::filesystem::last_write_time(fileNameIn, ec);
            lastModified = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::clock_cast<std::chrono::system_clock>(time).time_since_epoch()).count();
        } else {
            std::ofstream create(fileNameIn, std::ios::binary); // RandomAccessFile "rw" creates the file
        }
        dataFile.open(fileNameIn, std::ios::in | std::ios::out | std::ios::binary);
        if (!dataFile)
            throw IOException("Couldn't open " + fileNameIn.string());
        dataFile.seekp(0);
        if (length() < 4096) {
            for (int i = 0; i < 1024; ++i)
                dataFile.write("\0\0\0\0", 4);
            for (int i1 = 0; i1 < 1024; ++i1)
                dataFile.write("\0\0\0\0", 4);
            sizeDelta += 8192;
        }
        // Java pads an unaligned file with zeros at the current position (which, for an existing file, is the
        // start of the offset table).
        const int64_t misalignment = length() & 4095;
        for (int64_t j1 = 0; j1 < misalignment; ++j1)
            dataFile.put('\0');
        dataFile.flush();
        const int k1 = static_cast<int>(length()) / 4096;
        sectorFree.assign(static_cast<size_t>(k1), true);
        sectorFree.at(0) = false;
        sectorFree.at(1) = false;
        for (int l1 = 0; l1 < 1024; ++l1) {
            const int k = readIntAt(dataFile, static_cast<int64_t>(l1) * 4);
            offsets[static_cast<size_t>(l1)] = k;
            if (k != 0 && (k >> 8) + (k & 255) <= static_cast<int>(sectorFree.size())) {
                for (int l = 0; l < (k & 255); ++l)
                    sectorFree.at(static_cast<size_t>((k >> 8) + l)) = false;
            }
        }
        for (int i2 = 0; i2 < 1024; ++i2)
            chunkTimestamps[static_cast<size_t>(i2)] = readIntAt(dataFile, 4096 + static_cast<int64_t>(i2) * 4);
    } catch (const IOException& e) {
        Log::error("{}", e.what()); // Java: printStackTrace
    }
}

int64_t RegionFile::length()
{
    dataFile.flush();
    std::error_code ec;
    const auto size = std::filesystem::file_size(fileName, ec);
    return ec ? 0 : static_cast<int64_t>(size);
}

std::optional<std::vector<uint8_t>> RegionFile::getChunkData(int x, int z)
{
    std::lock_guard guard(lock);
    if (outOfBounds(x, z))
        return std::nullopt;
    std::vector<uint8_t> compressed;
    uint8_t b0 = 0;
    try {
        const int i = getOffset(x, z);
        if (i == 0)
            return std::nullopt;
        const int j = i >> 8;
        const int k = i & 255;
        if (j + k > static_cast<int>(sectorFree.size()))
            return std::nullopt;
        const int l = readIntAt(dataFile, static_cast<int64_t>(j) * 4096);
        if (l > 4096 * k || l <= 0)
            return std::nullopt;
        b0 = static_cast<uint8_t>(dataFile.get());
        if (!dataFile)
            throw IOException("EOF");
        if (b0 != 1 && b0 != 2)
            return std::nullopt;
        // RandomAccessFile.read fills what is there; the rest stays zero.
        compressed.assign(static_cast<size_t>(l - 1), 0);
        dataFile.read(reinterpret_cast<char*>(compressed.data()), static_cast<std::streamsize>(compressed.size()));
        dataFile.clear();
    } catch (const IOException&) {
        dataFile.clear();
        return std::nullopt;
    }
    return b0 == 1 ? gzipDecompress(compressed) : zlibDecompress(compressed);
}

void RegionFile::writeChunkData(int x, int z, std::span<const uint8_t> data)
{
    if (outOfBounds(x, z))
        return;
    write(x, z, zlibCompress(data));
}

void RegionFile::write(int x, int z, std::span<const uint8_t> data)
{
    std::lock_guard guard(lock);
    try {
        const int i = getOffset(x, z);
        int j = i >> 8;
        const int k = i & 255;
        const int length = static_cast<int>(data.size());
        const int l = (length + 5) / 4096 + 1;
        if (l >= 256)
            return;
        if (j != 0 && k == l) {
            writeSector(j, data);
        } else {
            for (int i1 = 0; i1 < k; ++i1)
                sectorFree.at(static_cast<size_t>(j + i1)) = true;
            int l1 = -1;
            for (size_t s = 0; s < sectorFree.size(); ++s) {
                if (sectorFree[s]) {
                    l1 = static_cast<int>(s);
                    break;
                }
            }
            int j1 = 0;
            if (l1 != -1) {
                for (int k1 = l1; k1 < static_cast<int>(sectorFree.size()); ++k1) {
                    if (j1 != 0) {
                        if (sectorFree[static_cast<size_t>(k1)])
                            ++j1;
                        else
                            j1 = 0;
                    } else if (sectorFree[static_cast<size_t>(k1)]) {
                        l1 = k1;
                        j1 = 1;
                    }
                    if (j1 >= l)
                        break;
                }
            }
            if (j1 >= l) {
                j = l1;
                setOffset(x, z, l1 << 8 | l);
                for (int j2 = 0; j2 < l; ++j2)
                    sectorFree.at(static_cast<size_t>(j + j2)) = false;
                writeSector(j, data);
            } else {
                dataFile.seekp(this->length());
                j = static_cast<int>(sectorFree.size());
                for (int i2 = 0; i2 < l; ++i2) {
                    dataFile.write(emptySector.data(), static_cast<std::streamsize>(emptySector.size()));
                    sectorFree.push_back(false);
                }
                sizeDelta += 4096 * l;
                writeSector(j, data);
                setOffset(x, z, j << 8 | l);
            }
        }
        setChunkTimestamp(x, z, static_cast<int>(java::currentTimeMillis() / 1000LL));
        dataFile.flush();
    } catch (const IOException& e) {
        Log::error("{}", e.what());
    }
}

void RegionFile::writeIntAt(int64_t position, int32_t value)
{
    dataFile.seekp(position);
    const char b[4] { static_cast<char>(value >> 24), static_cast<char>(value >> 16), static_cast<char>(value >> 8), static_cast<char>(value) };
    dataFile.write(b, 4);
    if (!dataFile)
        throw IOException("Write failed: " + fileName.string());
}

void RegionFile::writeSector(int sectorNumber, std::span<const uint8_t> data)
{
    writeIntAt(static_cast<int64_t>(sectorNumber) * 4096, static_cast<int32_t>(data.size()) + 1);
    dataFile.put(2);
    dataFile.write(reinterpret_cast<const char*>(data.data()), static_cast<std::streamsize>(data.size()));
    if (!dataFile)
        throw IOException("Write failed: " + fileName.string());
}

void RegionFile::setOffset(int x, int z, int offset)
{
    offsets[static_cast<size_t>(x + z * 32)] = offset;
    writeIntAt(static_cast<int64_t>(x + z * 32) * 4, offset);
}

void RegionFile::setChunkTimestamp(int x, int z, int timestamp)
{
    chunkTimestamps[static_cast<size_t>(x + z * 32)] = timestamp;
    writeIntAt(4096 + static_cast<int64_t>(x + z * 32) * 4, timestamp);
}

void RegionFile::close()
{
    std::lock_guard guard(lock);
    if (dataFile.is_open())
        dataFile.close();
}

// --- RegionFileCache ---

namespace {

std::recursive_mutex& cacheLock()
{
    static std::recursive_mutex mutex;
    return mutex;
}

std::map<std::filesystem::path, std::shared_ptr<RegionFile>>& regionsByFilename()
{
    static std::map<std::filesystem::path, std::shared_ptr<RegionFile>> map;
    return map;
}

} // namespace

std::shared_ptr<RegionFile> RegionFileCache::createOrLoadRegionFile(const std::filesystem::path& worldDir, int chunkX, int chunkZ)
{
    std::lock_guard guard(cacheLock());
    const std::filesystem::path file1 = worldDir / "region";
    const std::filesystem::path file2 = file1 / ("r." + std::to_string(chunkX >> 5) + "." + std::to_string(chunkZ >> 5) + ".mca");
    auto& regions = regionsByFilename();
    if (const auto it = regions.find(file2); it != regions.end())
        return it->second;
    std::error_code ec;
    if (!std::filesystem::exists(file1, ec))
        std::filesystem::create_directories(file1, ec);
    if (regions.size() >= 256)
        clearRegionFileReferences();
    auto regionfile1 = std::make_shared<RegionFile>(file2);
    regions[file2] = regionfile1;
    return regionfile1;
}

void RegionFileCache::clearRegionFileReferences()
{
    std::lock_guard guard(cacheLock());
    for (auto& [path, regionfile] : regionsByFilename())
        regionfile->close();
    regionsByFilename().clear();
}

std::optional<std::vector<uint8_t>> RegionFileCache::getChunkData(const std::filesystem::path& worldDir, int chunkX, int chunkZ)
{
    return createOrLoadRegionFile(worldDir, chunkX, chunkZ)->getChunkData(chunkX & 31, chunkZ & 31);
}

void RegionFileCache::writeChunkData(const std::filesystem::path& worldDir, int chunkX, int chunkZ, std::span<const uint8_t> data)
{
    createOrLoadRegionFile(worldDir, chunkX, chunkZ)->writeChunkData(chunkX & 31, chunkZ & 31, data);
}

} // namespace mc

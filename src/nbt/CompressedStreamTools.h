#pragma once

// Original: net/minecraft/nbt/CompressedStreamTools.java
// Works on whole buffers and files instead of Java streams.

#include "nbt/NBTTagCompound.h"

#include <filesystem>
#include <memory>
#include <span>
#include <vector>

namespace mc {

class CompressedStreamTools {
public:
    static std::shared_ptr<NBTTagCompound> readCompressed(std::span<const uint8_t> gzipData);
    static std::vector<uint8_t> writeCompressed(const NBTTagCompound& compound);
    static std::shared_ptr<NBTTagCompound> readCompressed(const std::filesystem::path& file);
    static void writeCompressed(const NBTTagCompound& compound, const std::filesystem::path& file);

    // Writes <file>_tmp, then replaces the file with it.
    static void safeWrite(const NBTTagCompound& compound, const std::filesystem::path& file);
    // Uncompressed files; read returns null when the file doesn't exist.
    static void write(const NBTTagCompound& compound, const std::filesystem::path& file);
    static std::shared_ptr<NBTTagCompound> read(const std::filesystem::path& file);

    // The root tag: a type byte, an empty name and the compound. Throws IOException if the root isn't one.
    static std::shared_ptr<NBTTagCompound> read(DataInput& input, NBTSizeTracker& sizeTracker);
    static void write(const NBTTagCompound& compound, DataOutput& output);
};

} // namespace mc

#pragma once

// GZIP and zlib streams (java.util.zip.GZIPInputStream/GZIPOutputStream, InflaterInputStream/DeflaterOutputStream)
// over miniz. The compressed bytes differ from Java's zlib output, the decompressed data never does.

#include <cstdint>
#include <span>
#include <vector>

namespace mc {

// Throws IOException on a bad header, corrupt data or a CRC/size mismatch. Concatenated members are joined,
// like GZIPInputStream does.
std::vector<uint8_t> gzipDecompress(std::span<const uint8_t> data);
// Header as GZIPOutputStream writes it (no name, mtime 0), default compression level.
std::vector<uint8_t> gzipCompress(std::span<const uint8_t> data);

std::vector<uint8_t> zlibDecompress(std::span<const uint8_t> data);
std::vector<uint8_t> zlibCompress(std::span<const uint8_t> data);

} // namespace mc

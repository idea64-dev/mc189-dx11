#include "core/Compression.h"

#include "core/DataStream.h"

#include <miniz/miniz.h>

#include <string>

namespace mc {

namespace {

// Inflates one stream; `windowBits` negative for raw deflate. Returns the number of input bytes consumed.
size_t inflate(std::span<const uint8_t> input, int windowBits, std::vector<uint8_t>& out)
{
    mz_stream stream {};
    if (mz_inflateInit2(&stream, windowBits) != MZ_OK)
        throw IOException("inflateInit failed");

    stream.next_in = input.data();
    stream.avail_in = static_cast<unsigned int>(input.size());
    uint8_t buffer[65536];
    int status = MZ_OK;
    while (status != MZ_STREAM_END) {
        stream.next_out = buffer;
        stream.avail_out = sizeof(buffer);
        status = mz_inflate(&stream, MZ_NO_FLUSH);
        out.insert(out.end(), buffer, buffer + (sizeof(buffer) - stream.avail_out));
        if (status == MZ_BUF_ERROR && stream.avail_in == 0) {
            mz_inflateEnd(&stream);
            throw IOException("java.io.EOFException: Unexpected end of ZLIB input stream");
        }
        if (status != MZ_OK && status != MZ_STREAM_END && status != MZ_BUF_ERROR) {
            mz_inflateEnd(&stream);
            throw IOException("java.util.zip.ZipException: invalid compressed data (" + std::to_string(status) + ")");
        }
    }
    const size_t consumed = input.size() - stream.avail_in;
    mz_inflateEnd(&stream);
    return consumed;
}

std::vector<uint8_t> deflate(std::span<const uint8_t> input, int windowBits)
{
    mz_stream stream {};
    if (mz_deflateInit2(&stream, MZ_DEFAULT_LEVEL, MZ_DEFLATED, windowBits, 8, MZ_DEFAULT_STRATEGY) != MZ_OK)
        throw IOException("deflateInit failed");
    std::vector<uint8_t> out(mz_deflateBound(&stream, static_cast<mz_ulong>(input.size())) + 16);
    stream.next_in = input.data();
    stream.avail_in = static_cast<unsigned int>(input.size());
    stream.next_out = out.data();
    stream.avail_out = static_cast<unsigned int>(out.size());
    const int status = mz_deflate(&stream, MZ_FINISH);
    const size_t produced = out.size() - stream.avail_out;
    mz_deflateEnd(&stream);
    if (status != MZ_STREAM_END)
        throw IOException("deflate failed");
    out.resize(produced);
    return out;
}

uint32_t readLittle32(const uint8_t* p)
{
    return static_cast<uint32_t>(p[0]) | (static_cast<uint32_t>(p[1]) << 8) | (static_cast<uint32_t>(p[2]) << 16) | (static_cast<uint32_t>(p[3]) << 24);
}

void writeLittle32(std::vector<uint8_t>& out, uint32_t value)
{
    for (int i = 0; i < 4; ++i)
        out.push_back(static_cast<uint8_t>(value >> (8 * i)));
}

} // namespace

std::vector<uint8_t> gzipDecompress(std::span<const uint8_t> data)
{
    std::vector<uint8_t> out;
    size_t pos = 0;
    bool first = true;
    while (first || pos < data.size()) {
        // A trailing member must start with the magic; GZIPInputStream stops quietly otherwise.
        if (data.size() - pos < 10 || data[pos] != 0x1F || data[pos + 1] != 0x8B) {
            if (first)
                throw IOException("java.util.zip.ZipException: Not in GZIP format");
            break;
        }
        if (data[pos + 2] != 8)
            throw IOException("java.util.zip.ZipException: Unsupported compression method");
        const uint8_t flags = data[pos + 3];
        size_t p = pos + 10;
        const auto need = [&](size_t n) {
            if (data.size() - p < n)
                throw IOException("java.io.EOFException");
        };
        if (flags & 4) { // FEXTRA
            need(2);
            const size_t extra = data[p] | (data[p + 1] << 8);
            p += 2;
            need(extra);
            p += extra;
        }
        for (const uint8_t bit : { uint8_t(8), uint8_t(16) }) { // FNAME, FCOMMENT
            if (flags & bit) {
                while (true) {
                    need(1);
                    if (data[p++] == 0)
                        break;
                }
            }
        }
        if (flags & 2) { // FHCRC
            need(2);
            p += 2;
        }

        const size_t start = out.size();
        p += inflate(data.subspan(p), -MZ_DEFAULT_WINDOW_BITS, out);
        need(8);
        const uint32_t crc = readLittle32(&data[p]);
        const uint32_t size = readLittle32(&data[p + 4]);
        p += 8;
        const auto member = std::span<const uint8_t>(out).subspan(start);
        if (static_cast<uint32_t>(mz_crc32(MZ_CRC32_INIT, member.data(), member.size())) != crc)
            throw IOException("java.util.zip.ZipException: Corrupt GZIP trailer");
        if (static_cast<uint32_t>(member.size()) != size)
            throw IOException("java.util.zip.ZipException: Corrupt GZIP trailer");
        pos = p;
        first = false;
    }
    return out;
}

std::vector<uint8_t> gzipCompress(std::span<const uint8_t> data)
{
    std::vector<uint8_t> out { 0x1F, 0x8B, 8, 0, 0, 0, 0, 0, 0, 0 };
    const std::vector<uint8_t> body = deflate(data, -MZ_DEFAULT_WINDOW_BITS);
    out.insert(out.end(), body.begin(), body.end());
    writeLittle32(out, static_cast<uint32_t>(mz_crc32(MZ_CRC32_INIT, data.data(), data.size())));
    writeLittle32(out, static_cast<uint32_t>(data.size()));
    return out;
}

std::vector<uint8_t> zlibDecompress(std::span<const uint8_t> data)
{
    std::vector<uint8_t> out;
    inflate(data, MZ_DEFAULT_WINDOW_BITS, out);
    return out;
}

std::vector<uint8_t> zlibCompress(std::span<const uint8_t> data)
{
    return deflate(data, MZ_DEFAULT_WINDOW_BITS);
}

} // namespace mc

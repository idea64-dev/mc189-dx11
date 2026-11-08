#include "nbt/CompressedStreamTools.h"

#include "core/Compression.h"
#include "core/StringUtil.h"

#include <fstream>
#include <iterator>

namespace mc {

namespace {

std::vector<uint8_t> readFile(const std::filesystem::path& file)
{
    std::ifstream stream(file, std::ios::binary);
    if (!stream)
        throw IOException("java.io.FileNotFoundException: " + toUtf8(file.wstring()));
    return std::vector<uint8_t>(std::istreambuf_iterator<char>(stream), std::istreambuf_iterator<char>());
}

void writeFile(const std::filesystem::path& file, std::span<const uint8_t> data)
{
    std::ofstream stream(file, std::ios::binary | std::ios::trunc);
    if (!stream)
        throw IOException("java.io.FileNotFoundException: " + toUtf8(file.wstring()));
    stream.write(reinterpret_cast<const char*>(data.data()), static_cast<std::streamsize>(data.size()));
    if (!stream)
        throw IOException("write failed: " + toUtf8(file.wstring()));
}

} // namespace

std::shared_ptr<NBTTagCompound> CompressedStreamTools::readCompressed(std::span<const uint8_t> gzipData)
{
    const std::vector<uint8_t> data = gzipDecompress(gzipData);
    DataInput input(data);
    NBTSizeTracker tracker = NBTSizeTracker::infinite();
    return read(input, tracker);
}

std::vector<uint8_t> CompressedStreamTools::writeCompressed(const NBTTagCompound& compound)
{
    DataOutput output;
    write(compound, output);
    return gzipCompress(output.getBytes());
}

std::shared_ptr<NBTTagCompound> CompressedStreamTools::readCompressed(const std::filesystem::path& file)
{
    return readCompressed(readFile(file));
}

void CompressedStreamTools::writeCompressed(const NBTTagCompound& compound, const std::filesystem::path& file)
{
    writeFile(file, writeCompressed(compound));
}

void CompressedStreamTools::safeWrite(const NBTTagCompound& compound, const std::filesystem::path& file)
{
    std::filesystem::path temp = std::filesystem::absolute(file);
    temp += "_tmp";
    std::error_code ec;
    std::filesystem::remove(temp, ec);
    write(compound, temp);
    std::filesystem::remove(file, ec);
    if (std::filesystem::exists(file, ec))
        throw IOException("Failed to delete " + toUtf8(file.wstring()));
    std::filesystem::rename(temp, file, ec);
}

void CompressedStreamTools::write(const NBTTagCompound& compound, const std::filesystem::path& file)
{
    DataOutput output;
    write(compound, output);
    writeFile(file, output.getBytes());
}

std::shared_ptr<NBTTagCompound> CompressedStreamTools::read(const std::filesystem::path& file)
{
    std::error_code ec;
    if (!std::filesystem::exists(file, ec))
        return nullptr;
    const std::vector<uint8_t> data = readFile(file);
    DataInput input(data);
    NBTSizeTracker tracker = NBTSizeTracker::infinite();
    return read(input, tracker);
}

std::shared_ptr<NBTTagCompound> CompressedStreamTools::read(DataInput& input, NBTSizeTracker& sizeTracker)
{
    const int8_t b0 = input.readByte();
    if (b0 == 0)
        throw IOException("Root tag must be a named compound tag");
    input.readUTF();
    auto tag = NBTTagCompound::readNBT(b0, "[UNNAMED TAG]", input, 0, sizeTracker);
    if (tag->getId() != 10)
        throw IOException("Root tag must be a named compound tag");
    return std::static_pointer_cast<NBTTagCompound>(tag);
}

void CompressedStreamTools::write(const NBTTagCompound& compound, DataOutput& output)
{
    output.writeByte(compound.getId());
    output.writeUTF("");
    compound.write(output);
}

} // namespace mc

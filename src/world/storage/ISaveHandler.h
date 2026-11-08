#pragma once

// Original: net/minecraft/world/storage/ISaveHandler.java, net/minecraft/world/chunk/storage/IChunkLoader.java and
// net/minecraft/world/MinecraftException.java.
// getPlayerNBTManager (player data) comes with the player (stage 8).

#include <filesystem>
#include <memory>
#include <stdexcept>
#include <string>

namespace mc {

class Chunk;
class NBTTagCompound;
class World;
class WorldInfo;
class WorldProvider;

// Java: MinecraftException -- the session lock was taken by another instance.
class MinecraftException : public std::runtime_error {
public:
    using std::runtime_error::runtime_error;
};

class IChunkLoader {
public:
    virtual ~IChunkLoader() = default;
    // Null when the chunk isn't stored. Throws IOException for unreadable data.
    virtual std::unique_ptr<Chunk> loadChunk(World* worldIn, int x, int z) = 0;
    virtual void saveChunk(World* worldIn, Chunk* chunkIn) = 0;
    virtual void saveExtraChunkData(World* worldIn, Chunk* chunkIn) = 0;
    virtual void chunkTick() = 0;
    virtual void saveExtraData() = 0;
};

class ISaveHandler {
public:
    virtual ~ISaveHandler() = default;
    // Null when there is no readable level.dat (or level.dat_old).
    virtual std::unique_ptr<WorldInfo> loadWorldInfo() = 0;
    // Throws MinecraftException when another instance took over the world.
    virtual void checkSessionLock() = 0;
    virtual std::unique_ptr<IChunkLoader> getChunkLoader(const WorldProvider& provider) = 0;
    virtual void saveWorldInfoWithPlayer(WorldInfo& worldInformation, const std::shared_ptr<NBTTagCompound>& tagCompound) = 0;
    virtual void saveWorldInfo(WorldInfo& worldInformation) = 0;
    virtual void flush() = 0;
    virtual std::filesystem::path getWorldDirectory() const = 0;
    virtual std::filesystem::path getMapFileFromName(const std::string& mapName) const = 0;
    virtual std::string getWorldDirectoryName() const = 0;
};

} // namespace mc

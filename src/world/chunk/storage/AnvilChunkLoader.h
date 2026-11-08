#pragma once

// Original: net/minecraft/world/chunk/storage/AnvilChunkLoader.java -- chunks in region files as NBT. Saving
// queues the NBT; the File IO thread writes it.
// Entities are kept as NBT on the chunk until stage 10 (see Chunk.h); scheduled ticks go to a WorldServer.

#include "world/ChunkCoordIntPair.h"
#include "world/storage/ISaveHandler.h"
#include "world/storage/ThreadedFileIOBase.h"

#include <filesystem>
#include <list>
#include <memory>
#include <mutex>
#include <unordered_map>
#include <unordered_set>

namespace mc {

class AnvilChunkLoader : public IChunkLoader, public IThreadedFileIO {
public:
    explicit AnvilChunkLoader(std::filesystem::path chunkSaveLocationIn);
    ~AnvilChunkLoader() override;

    std::unique_ptr<Chunk> loadChunk(World* worldIn, int x, int z) override;
    void saveChunk(World* worldIn, Chunk* chunkIn) override;
    bool writeNextIO() override;
    void saveExtraChunkData(World*, Chunk*) override { }
    void chunkTick() override { }
    void saveExtraData() override;

    // Public for the reference tests.
    std::shared_ptr<NBTTagCompound> writeChunkToNBT(Chunk& chunkIn, World& worldIn);
    std::unique_ptr<Chunk> readChunkFromNBT(World* worldIn, const NBTTagCompound& level);

protected:
    std::unique_ptr<Chunk> checkedReadChunkFromNBT(World* worldIn, int x, int z, const std::shared_ptr<NBTTagCompound>& nbt);
    void addChunkToPending(const ChunkCoordIntPair& pos, std::shared_ptr<NBTTagCompound> nbt);

private:
    void writeChunkNBT(const ChunkCoordIntPair& pos, const NBTTagCompound& nbt);

    // Java: a ConcurrentHashMap; writes take whichever chunk its iterator yields first. Here the oldest.
    std::mutex pendingLock;
    std::unordered_map<ChunkCoordIntPair, std::shared_ptr<NBTTagCompound>> chunksToRemove;
    std::list<ChunkCoordIntPair> chunkOrder;
    std::unordered_set<ChunkCoordIntPair> pendingAnvilChunksCoordinates;
    const std::filesystem::path chunkSaveLocation;
    bool field_183014_e = false;
};

} // namespace mc

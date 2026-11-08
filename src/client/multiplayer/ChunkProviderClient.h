#pragma once

// Original: net/minecraft/client/multiplayer/ChunkProviderClient.java
// The chunks the server sent. Java's LongHashMap is only looked up, never iterated; the list keeps the order the
// chunks arrived in, which the per-tick chunk update walks.

#include "world/chunk/Chunk.h"
#include "world/chunk/IChunkProvider.h"

#include <cstdint>
#include <memory>
#include <unordered_map>
#include <vector>

namespace mc {

class World;

class ChunkProviderClient : public IChunkProvider {
public:
    explicit ChunkProviderClient(World* worldIn);

    bool chunkExists(int, int) override { return true; }
    // Java: unloadChunk -- on a chunk packet without data.
    void unloadChunk(int x, int z);
    Chunk* loadChunk(int chunkX, int chunkZ);
    Chunk* provideChunk(int x, int z) override;
    Chunk* provideChunk(const BlockPos& blockPosIn) override { return provideChunk(blockPosIn.getX() >> 4, blockPosIn.getZ() >> 4); }
    bool saveChunks(bool, IProgressUpdate*) override { return true; }
    void saveExtraData() override { }
    bool unloadQueuedChunks() override;
    bool canSave() override { return false; }
    void populate(IChunkProvider*, int, int) override { }
    bool populateChunk(IChunkProvider*, Chunk*, int, int) override { return false; }
    std::string makeString() override;
    int getLoadedChunkCount() override { return static_cast<int>(chunkListing.size()); }
    void recreateStructures(Chunk*, int, int) override { }

private:
    EmptyChunk blankChunk;
    std::unordered_map<int64_t, std::unique_ptr<Chunk>> chunkMapping;
    std::vector<Chunk*> chunkListing;
    World* worldObj;
};

} // namespace mc

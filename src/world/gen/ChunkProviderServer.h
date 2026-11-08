#pragma once

// Original: net/minecraft/world/gen/ChunkProviderServer.java -- the server's chunk cache: loads chunks from the
// save or has the generator make them, populates them, saves and unloads the dropped ones.

#include "core/JavaHashMap.h"
#include "world/chunk/IChunkProvider.h"

#include <array>
#include <memory>
#include <unordered_map>
#include <vector>

namespace mc {

class Chunk;
class IChunkLoader;
class WorldServer;

class ChunkProviderServer : public IChunkProvider {
public:
    ChunkProviderServer(WorldServer* worldIn, std::unique_ptr<IChunkLoader> chunkLoaderIn, std::unique_ptr<IChunkProvider> chunkGeneratorIn);
    ~ChunkProviderServer() override;

    bool chunkExists(int x, int z) override;
    // Java: func_152380_a -- the loaded chunks in load order.
    const std::vector<Chunk*>& func_152380_a() const { return loadedChunks; }
    // Queues the chunk for unloading (spawn chunks of a respawn dimension stay).
    void dropChunk(int x, int z);
    void unloadAllChunks();
    Chunk* loadChunk(int chunkX, int chunkZ);
    Chunk* provideChunk(int x, int z) override;
    Chunk* provideChunk(const BlockPos& blockPosIn) override { return provideChunk(blockPosIn.getX() >> 4, blockPosIn.getZ() >> 4); }
    void populate(IChunkProvider* chunkProvider, int x, int z) override;
    bool populateChunk(IChunkProvider* chunkProvider, Chunk* chunkIn, int x, int z) override;
    bool saveChunks(bool saveAllChunks, IProgressUpdate* progressCallback) override;
    void saveExtraData() override;
    bool unloadQueuedChunks() override;
    bool canSave() override;
    std::string makeString() override;
    std::vector<BiomeGenBase::SpawnListEntry> getPossibleCreatures(EnumCreatureType creatureType, const BlockPos& pos) override;
    std::optional<BlockPos> getStrongholdGen(World* worldIn, const std::string& structureName, const BlockPos& position) override;
    int getLoadedChunkCount() override { return static_cast<int>(id2ChunkMap.size()); }
    void recreateStructures(Chunk*, int, int) override { }

    bool chunkLoadOverride = true;

private:
    // The loaded chunk (the dummy one for a null entry), or null when the position is not loaded.
    Chunk* findLoaded(int x, int z);
    std::unique_ptr<Chunk> loadChunkFromFile(int x, int z);
    void saveChunkExtraData(Chunk* chunkIn);
    void saveChunkData(Chunk* chunkIn);

    // Java: a set over a ConcurrentHashMap; unloading takes the first key its iterator yields.
    JavaHashMap<int64_t, bool> droppedChunksSet;
    std::unique_ptr<Chunk> dummyChunk;
    std::unique_ptr<IChunkProvider> serverChunkGenerator;
    std::unique_ptr<IChunkLoader> chunkLoader;
    // Java: a LongHashMap, never iterated.
    std::unordered_map<int64_t, std::unique_ptr<Chunk>> id2ChunkMap;
    // In front of id2ChunkMap: block access and lighting stay within a few chunks, so most lookups hit here.
    // Cleared when chunks unload.
    struct CachedChunk {
        int64_t key = 0;
        Chunk* chunk = nullptr;
    };
    std::array<CachedChunk, 64> chunkCache {};
    std::vector<Chunk*> loadedChunks;
    WorldServer* worldObj;
};

} // namespace mc

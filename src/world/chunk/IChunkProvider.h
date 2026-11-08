#pragma once

// Original: net/minecraft/world/chunk/IChunkProvider.java

#include "util/BlockPos.h"
#include "world/biome/BiomeGenBase.h"

#include <memory>
#include <optional>
#include <string>
#include <vector>

namespace mc {

class Chunk;
class IProgressUpdate;
class World;

class IChunkProvider {
public:
    virtual ~IChunkProvider() = default;
    virtual bool chunkExists(int x, int z) = 0;
    // Never null: providers hand out an empty chunk where there is none.
    virtual Chunk* provideChunk(int x, int z) = 0;
    virtual Chunk* provideChunk(const BlockPos& blockPosIn) = 0;
    // The generators' provideChunk: a new chunk the caller owns (ChunkProviderServer keeps it). Null for the
    // providers that keep their own chunks.
    virtual std::unique_ptr<Chunk> generateChunk(int x, int z);
    virtual void populate(IChunkProvider* chunkProvider, int x, int z) = 0;
    virtual bool populateChunk(IChunkProvider* chunkProvider, Chunk* chunkIn, int x, int z) = 0;
    virtual bool saveChunks(bool saveAllChunks, IProgressUpdate* progressCallback) = 0;
    virtual bool unloadQueuedChunks() = 0;
    virtual bool canSave() = 0;
    virtual std::string makeString() = 0;
    virtual std::vector<BiomeGenBase::SpawnListEntry> getPossibleCreatures(EnumCreatureType, const BlockPos&) { return {}; }
    // Java returns null when there is none.
    virtual std::optional<BlockPos> getStrongholdGen(World*, const std::string&, const BlockPos&) { return std::nullopt; }
    virtual int getLoadedChunkCount() = 0;
    virtual void recreateStructures(Chunk* chunkIn, int x, int z) = 0;
    virtual void saveExtraData() = 0;
};

} // namespace mc

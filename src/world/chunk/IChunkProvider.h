#pragma once

// Original: net/minecraft/world/chunk/IChunkProvider.java
// getPossibleCreatures and getStrongholdGen come with mob spawning and structures (stages 10 and 6).

#include "util/BlockPos.h"

#include <string>

namespace mc {

class Chunk;
class IProgressUpdate;

class IChunkProvider {
public:
    virtual ~IChunkProvider() = default;
    virtual bool chunkExists(int x, int z) = 0;
    // Never null: providers hand out an empty chunk where there is none.
    virtual Chunk* provideChunk(int x, int z) = 0;
    virtual Chunk* provideChunk(const BlockPos& blockPosIn) = 0;
    virtual void populate(IChunkProvider* chunkProvider, int x, int z) = 0;
    virtual bool populateChunk(IChunkProvider* chunkProvider, Chunk* chunkIn, int x, int z) = 0;
    virtual bool saveChunks(bool saveAllChunks, IProgressUpdate* progressCallback) = 0;
    virtual bool unloadQueuedChunks() = 0;
    virtual bool canSave() = 0;
    virtual std::string makeString() = 0;
    virtual int getLoadedChunkCount() = 0;
    virtual void recreateStructures(Chunk* chunkIn, int x, int z) = 0;
    virtual void saveExtraData() = 0;
};

} // namespace mc

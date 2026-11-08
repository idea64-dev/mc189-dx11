#pragma once

// Original: net/minecraft/world/gen/ChunkProviderDebug.java -- the debug world: every block state on a grid at
// y 70 over a barrier floor.

#include "world/chunk/IChunkProvider.h"

#include <memory>

namespace mc {

class IBlockState;
class World;

class ChunkProviderDebug : public IChunkProvider {
public:
    explicit ChunkProviderDebug(World* worldIn)
        : world(worldIn)
    {
    }
    ~ChunkProviderDebug() override;

    // The state at a block of the grid, null between the cells.
    static const IBlockState* func_177461_b(int x, int z);

    std::unique_ptr<Chunk> generateChunk(int x, int z) override;
    bool chunkExists(int, int) override { return true; }
    Chunk* provideChunk(int x, int z) override;
    Chunk* provideChunk(const BlockPos& blockPosIn) override { return provideChunk(blockPosIn.getX() >> 4, blockPosIn.getZ() >> 4); }
    void populate(IChunkProvider*, int, int) override { }
    bool populateChunk(IChunkProvider*, Chunk*, int, int) override { return false; }
    bool saveChunks(bool, IProgressUpdate*) override { return true; }
    void saveExtraData() override { }
    bool unloadQueuedChunks() override { return false; }
    bool canSave() override { return true; }
    std::string makeString() override { return "DebugLevelSource"; }
    std::vector<BiomeGenBase::SpawnListEntry> getPossibleCreatures(EnumCreatureType creatureType, const BlockPos& pos) override;
    int getLoadedChunkCount() override { return 0; }
    void recreateStructures(Chunk*, int, int) override { }

private:
    World* const world;
    std::unique_ptr<Chunk> lastProvided;
};

} // namespace mc

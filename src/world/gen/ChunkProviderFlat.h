#pragma once

// Original: net/minecraft/world/gen/ChunkProviderFlat.java -- the superflat generator.

#include "util/Random.h"
#include "world/chunk/IChunkProvider.h"
#include "world/gen/FlatGeneratorInfo.h"

#include <array>
#include <memory>
#include <vector>

namespace mc {

class IBlockState;
class MapGenStructure;
class World;
class WorldGenLakes;

class ChunkProviderFlat : public IChunkProvider {
public:
    ChunkProviderFlat(World* worldIn, int64_t seed, bool generateStructures, const std::string& flatGeneratorSettings);
    ~ChunkProviderFlat() override;

    std::unique_ptr<Chunk> generateChunk(int x, int z) override;
    bool chunkExists(int, int) override { return true; }
    Chunk* provideChunk(int x, int z) override;
    Chunk* provideChunk(const BlockPos& blockPosIn) override { return provideChunk(blockPosIn.getX() >> 4, blockPosIn.getZ() >> 4); }
    void populate(IChunkProvider* chunkProvider, int x, int z) override;
    bool populateChunk(IChunkProvider*, Chunk*, int, int) override { return false; }
    bool saveChunks(bool, IProgressUpdate*) override { return true; }
    void saveExtraData() override { }
    bool unloadQueuedChunks() override { return false; }
    bool canSave() override { return true; }
    std::string makeString() override { return "FlatLevelSource"; }
    std::vector<BiomeGenBase::SpawnListEntry> getPossibleCreatures(EnumCreatureType creatureType, const BlockPos& pos) override;
    std::optional<BlockPos> getStrongholdGen(World* worldIn, const std::string& structureName, const BlockPos& position) override;
    int getLoadedChunkCount() override { return 0; }
    void recreateStructures(Chunk* chunkIn, int x, int z) override;

private:
    World* worldObj;
    Random random;
    std::array<const IBlockState*, 256> cachedBlockIDs {};
    FlatGeneratorInfo flatWorldGenInfo;
    std::vector<std::unique_ptr<MapGenStructure>> structureGenerators;
    bool hasDecoration;
    bool hasDungeons;
    std::unique_ptr<WorldGenLakes> waterLakeGenerator;
    std::unique_ptr<WorldGenLakes> lavaLakeGenerator;
    std::unique_ptr<Chunk> lastProvided;
};

} // namespace mc

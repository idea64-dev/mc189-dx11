#pragma once

// Original: net/minecraft/world/gen/ChunkProviderEnd.java -- the End's island.

#include "util/Random.h"
#include "world/chunk/IChunkProvider.h"

#include <memory>
#include <vector>

namespace mc {

class ChunkPrimer;
class NoiseGeneratorOctaves;
class World;

class ChunkProviderEnd : public IChunkProvider {
public:
    ChunkProviderEnd(World* worldIn, int64_t seed);
    ~ChunkProviderEnd() override;

    void func_180520_a(int x, int z, ChunkPrimer& primer);
    void func_180519_a(ChunkPrimer& primer);
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
    std::string makeString() override { return "RandomLevelSource"; }
    std::vector<BiomeGenBase::SpawnListEntry> getPossibleCreatures(EnumCreatureType creatureType, const BlockPos& pos) override;
    int getLoadedChunkCount() override { return 0; }
    void recreateStructures(Chunk*, int, int) override { }

    std::unique_ptr<NoiseGeneratorOctaves> noiseGen4;
    std::unique_ptr<NoiseGeneratorOctaves> noiseGen5;

private:
    std::vector<double>& initializeNoiseField(std::vector<double>& field, int x, int y, int z, int xSize, int ySize, int zSize);

    Random endRNG;
    std::unique_ptr<NoiseGeneratorOctaves> noiseGen1;
    std::unique_ptr<NoiseGeneratorOctaves> noiseGen2;
    std::unique_ptr<NoiseGeneratorOctaves> noiseGen3;
    World* endWorld;
    std::vector<double> densities;
    std::vector<BiomeGenBase*> biomesForGeneration;
    std::vector<double> noiseData1;
    std::vector<double> noiseData2;
    std::vector<double> noiseData3;
    std::vector<double> noiseData4;
    std::vector<double> noiseData5;
    std::unique_ptr<Chunk> lastProvided;
};

} // namespace mc

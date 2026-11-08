#pragma once

// Original: net/minecraft/world/gen/ChunkProviderHell.java -- the Nether generator.

#include "util/Random.h"
#include "world/chunk/IChunkProvider.h"

#include <memory>
#include <vector>

namespace mc {

class ChunkPrimer;
class MapGenBase;
class MapGenNetherBridge;
class NoiseGeneratorOctaves;
class World;
class WorldGenerator;

class ChunkProviderHell : public IChunkProvider {
public:
    ChunkProviderHell(World* worldIn, bool generateStructures, int64_t seed);
    ~ChunkProviderHell() override;

    void func_180515_a(int x, int z, ChunkPrimer& primer);
    void func_180516_b(int x, int z, ChunkPrimer& primer);
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
    std::string makeString() override { return "HellRandomLevelSource"; }
    std::vector<BiomeGenBase::SpawnListEntry> getPossibleCreatures(EnumCreatureType creatureType, const BlockPos& pos) override;
    int getLoadedChunkCount() override { return 0; }
    void recreateStructures(Chunk* chunkIn, int x, int z) override;

    std::unique_ptr<NoiseGeneratorOctaves> netherNoiseGen6;
    std::unique_ptr<NoiseGeneratorOctaves> netherNoiseGen7;

private:
    std::vector<double>& initializeNoiseField(std::vector<double>& field, int x, int y, int z, int xSize, int ySize, int zSize);

    World* const worldObj;
    const bool field_177466_i;
    Random hellRNG;
    std::vector<double> slowsandNoise;
    std::vector<double> gravelNoise;
    std::vector<double> netherrackExclusivityNoise;
    std::vector<double> noiseField;
    std::unique_ptr<NoiseGeneratorOctaves> netherNoiseGen1;
    std::unique_ptr<NoiseGeneratorOctaves> netherNoiseGen2;
    std::unique_ptr<NoiseGeneratorOctaves> netherNoiseGen3;
    std::unique_ptr<NoiseGeneratorOctaves> slowsandGravelNoiseGen;
    std::unique_ptr<NoiseGeneratorOctaves> netherrackExculsivityNoiseGen;
    std::unique_ptr<WorldGenerator> field_177470_t;
    std::unique_ptr<WorldGenerator> field_177469_u;
    std::unique_ptr<WorldGenerator> field_177468_v;
    std::unique_ptr<WorldGenerator> field_177467_w;
    std::unique_ptr<WorldGenerator> field_177473_x;
    std::unique_ptr<WorldGenerator> field_177472_y;
    std::unique_ptr<WorldGenerator> field_177471_z;
    std::unique_ptr<WorldGenerator> field_177465_A;
    std::unique_ptr<MapGenNetherBridge> genNetherBridge;
    std::unique_ptr<MapGenBase> netherCaveGenerator;
    std::vector<double> noiseData1;
    std::vector<double> noiseData2;
    std::vector<double> noiseData3;
    std::vector<double> noiseData4;
    std::vector<double> noiseData5;
    std::unique_ptr<Chunk> lastProvided;
};

} // namespace mc

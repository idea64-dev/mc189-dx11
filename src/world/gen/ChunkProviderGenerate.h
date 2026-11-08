#pragma once

// Original: net/minecraft/world/gen/ChunkProviderGenerate.java -- the overworld generator ("RandomLevelSource"):
// density noise, biome surface, carvers, structures, then the populate pass with lakes, dungeons, decoration
// and the snow and ice pass.

#include "util/Random.h"
#include "world/biome/BiomeGenBase.h"
#include "world/chunk/IChunkProvider.h"
#include "world/gen/ChunkProviderSettings.h"

#include <array>
#include <memory>
#include <optional>
#include <vector>

namespace mc {

class Block;
class ChunkPrimer;
class MapGenBase;
class MapGenMineshaft;
class MapGenScatteredFeature;
class MapGenStronghold;
class MapGenVillage;
class NoiseGeneratorOctaves;
class NoiseGeneratorPerlin;
class StructureOceanMonument;
class World;
class WorldType;

class ChunkProviderGenerate : public IChunkProvider {
public:
    ChunkProviderGenerate(World* worldIn, int64_t seed, bool generateStructures, const std::string& structuresJson);
    ~ChunkProviderGenerate() override;

    void setBlocksInChunk(int x, int z, ChunkPrimer& primer);
    void replaceBlocksForBiome(int x, int z, ChunkPrimer& primer, const std::vector<BiomeGenBase*>& biomeGens);
    // A new chunk; the caller (ChunkProviderServer) owns it.
    std::unique_ptr<Chunk> generateChunk(int x, int z);

    bool chunkExists(int, int) override { return true; }
    // IChunkProvider: generates and keeps the latest chunk alive until the next call (the server provider takes
    // ownership through generateChunk instead).
    Chunk* provideChunk(int x, int z) override;
    Chunk* provideChunk(const BlockPos& blockPosIn) override { return provideChunk(blockPosIn.getX() >> 4, blockPosIn.getZ() >> 4); }
    void populate(IChunkProvider* chunkProvider, int x, int z) override;
    bool populateChunk(IChunkProvider* chunkProvider, Chunk* chunkIn, int x, int z) override;
    bool saveChunks(bool, IProgressUpdate*) override { return true; }
    void saveExtraData() override { }
    bool unloadQueuedChunks() override { return false; }
    bool canSave() override { return true; }
    std::string makeString() override { return "RandomLevelSource"; }
    std::vector<BiomeGenBase::SpawnListEntry> getPossibleCreatures(EnumCreatureType creatureType, const BlockPos& pos) override;
    std::optional<BlockPos> getStrongholdGen(World* worldIn, const std::string& structureName, const BlockPos& position) override;
    int getLoadedChunkCount() override { return 0; }
    void recreateStructures(Chunk* chunkIn, int x, int z) override;

    std::unique_ptr<NoiseGeneratorOctaves> noiseGen5;
    std::unique_ptr<NoiseGeneratorOctaves> noiseGen6;
    std::unique_ptr<NoiseGeneratorOctaves> mobSpawnerNoise;

private:
    void func_147423_a(int x, int y, int z);

    Random rand;
    std::unique_ptr<NoiseGeneratorOctaves> field_147431_j;
    std::unique_ptr<NoiseGeneratorOctaves> field_147432_k;
    std::unique_ptr<NoiseGeneratorOctaves> field_147429_l;
    std::unique_ptr<NoiseGeneratorPerlin> field_147430_m;
    World* worldObj;
    const bool mapFeaturesEnabled;
    const WorldType* field_177475_o;
    std::array<double, 825> field_147434_q {};
    std::array<float, 25> parabolicField {};
    ChunkProviderSettings::Factory settings;
    Block* oceanBlockTmpl;
    std::vector<double> stoneNoise;
    std::unique_ptr<MapGenBase> caveGenerator;
    std::unique_ptr<MapGenStronghold> strongholdGenerator;
    std::unique_ptr<MapGenVillage> villageGenerator;
    std::unique_ptr<MapGenMineshaft> mineshaftGenerator;
    std::unique_ptr<MapGenScatteredFeature> scatteredFeatureGenerator;
    std::unique_ptr<MapGenBase> ravineGenerator;
    std::unique_ptr<StructureOceanMonument> oceanMonumentGenerator;
    std::vector<BiomeGenBase*> biomesForGeneration;
    std::vector<double> mainNoiseArray;
    std::vector<double> lowerLimitNoiseArray;
    std::vector<double> upperLimitNoiseArray;
    std::vector<double> depthNoiseArray;
    std::unique_ptr<Chunk> lastProvided;
};

} // namespace mc

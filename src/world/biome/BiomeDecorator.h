#pragma once

// Original: net/minecraft/world/biome/BiomeDecorator.java and BiomeEndDecorator.java -- the per-chunk
// decoration pass: ores, sand and clay, trees, plants, springs.

#include "util/BlockPos.h"
#include "world/gen/ChunkProviderSettings.h"

#include <memory>
#include <optional>

namespace mc {

class BiomeGenBase;
class Random;
class World;
class WorldGenerator;
class WorldGenFlowers;

class BiomeDecorator {
public:
    BiomeDecorator();
    virtual ~BiomeDecorator();

    // Throws std::runtime_error ("Already decorating") when re-entered, as Java.
    void decorate(World* worldIn, Random& random, BiomeGenBase* biome, const BlockPos& pos);

    int waterlilyPerChunk = 0;
    int treesPerChunk = 0;
    int flowersPerChunk = 2;
    int grassPerChunk = 1;
    int deadBushPerChunk = 0;
    int mushroomsPerChunk = 0;
    int reedsPerChunk = 0;
    int cactiPerChunk = 0;
    int sandPerChunk = 1;
    int sandPerChunk2 = 3;
    int clayPerChunk = 1;
    int bigMushroomsPerChunk = 0;
    bool generateLakes = true;

protected:
    virtual void genDecorations(BiomeGenBase* biome);
    void genStandardOre1(int blockCount, WorldGenerator& generator, int minHeight, int maxHeight);
    void genStandardOre2(int blockCount, WorldGenerator& generator, int centerHeight, int spread);
    void generateOres();

    World* currentWorld = nullptr;
    Random* randomGenerator = nullptr;
    BlockPos field_180294_c;
    std::optional<ChunkProviderSettings::Factory> chunkProviderSettings;
    std::unique_ptr<WorldGenerator> clayGen;
    std::unique_ptr<WorldGenerator> sandGen;
    std::unique_ptr<WorldGenerator> gravelAsSandGen;
    std::unique_ptr<WorldGenerator> dirtGen;
    std::unique_ptr<WorldGenerator> gravelGen;
    std::unique_ptr<WorldGenerator> graniteGen;
    std::unique_ptr<WorldGenerator> dioriteGen;
    std::unique_ptr<WorldGenerator> andesiteGen;
    std::unique_ptr<WorldGenerator> coalGen;
    std::unique_ptr<WorldGenerator> ironGen;
    std::unique_ptr<WorldGenerator> goldGen;
    std::unique_ptr<WorldGenerator> redstoneGen;
    std::unique_ptr<WorldGenerator> diamondGen;
    std::unique_ptr<WorldGenerator> lapisGen;
    std::unique_ptr<WorldGenFlowers> yellowFlowerGen;
    std::unique_ptr<WorldGenerator> mushroomBrownGen;
    std::unique_ptr<WorldGenerator> mushroomRedGen;
    std::unique_ptr<WorldGenerator> bigMushroomGen;
    std::unique_ptr<WorldGenerator> reedGen;
    std::unique_ptr<WorldGenerator> cactusGen;
    std::unique_ptr<WorldGenerator> waterlilyGen;
};

class BiomeEndDecorator : public BiomeDecorator {
public:
    BiomeEndDecorator();

protected:
    void genDecorations(BiomeGenBase* biome) override;

    std::unique_ptr<WorldGenerator> spikeGen;
};

} // namespace mc

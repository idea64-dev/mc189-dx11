#pragma once

// Original: net/minecraft/world/biome/WorldChunkManager.java, WorldChunkManagerHell.java and BiomeCache.java --
// the biome source of a dimension: the GenLayer stack (or one fixed biome) behind a cache of 16x16 blocks.

#include "util/BlockPos.h"

#include <array>
#include <cstdint>
#include <memory>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

namespace mc {

class BiomeGenBase;
class GenLayer;
class Random;
class World;
class WorldChunkManager;
class WorldType;

class BiomeCache {
public:
    struct Block {
        Block(WorldChunkManager& chunkManager, int x, int z);
        BiomeGenBase* getBiomeGenAt(int x, int z) const { return biomes[static_cast<size_t>((x & 15) | (z & 15) << 4)]; }

        std::vector<float> rainfallValues;
        std::vector<BiomeGenBase*> biomes;
        int xPosition;
        int zPosition;
        int64_t lastAccessTime = 0;
    };

    explicit BiomeCache(WorldChunkManager& chunkManagerIn)
        : chunkManager(chunkManagerIn)
    {
    }

    Block& getBiomeCacheBlock(int x, int z);
    BiomeGenBase* func_180284_a(int x, int z, BiomeGenBase* defaultBiome);
    void cleanupCache();
    const std::vector<BiomeGenBase*>& getCachedBiomes(int x, int z) { return getBiomeCacheBlock(x, z).biomes; }

private:
    WorldChunkManager& chunkManager;
    int64_t lastCleanupTime = 0;
    std::unordered_map<int64_t, std::unique_ptr<Block>> cacheMap;
    std::vector<Block*> cache;
};

class WorldChunkManager {
public:
    WorldChunkManager(int64_t seed, const WorldType* worldTypeIn, const std::string& options);
    explicit WorldChunkManager(World* worldIn);
    virtual ~WorldChunkManager();

    const std::vector<BiomeGenBase*>& getBiomesToSpawnIn() const { return biomesToSpawnIn; }
    virtual BiomeGenBase* getBiomeGenerator(const BlockPos& pos) { return getBiomeGenerator(pos, nullptr); }
    BiomeGenBase* getBiomeGenerator(const BlockPos& pos, BiomeGenBase* biomeGenBaseIn);
    // The Java methods take an array to reuse when big enough; here the vector is resized when smaller.
    virtual std::vector<float>& getRainfall(std::vector<float>& listToReuse, int x, int z, int width, int length);
    float getTemperatureAtHeight(float temperature, int) const { return temperature; }
    virtual std::vector<BiomeGenBase*>& getBiomesForGeneration(std::vector<BiomeGenBase*>& biomes, int x, int z, int width, int height);
    virtual std::vector<BiomeGenBase*>& loadBlockGeneratorData(std::vector<BiomeGenBase*>& oldBiomeList, int x, int z, int width, int depth);
    virtual std::vector<BiomeGenBase*>& getBiomeGenAt(std::vector<BiomeGenBase*>& listToReuse, int x, int z, int width, int length, bool cacheFlag);
    virtual bool areBiomesViable(int x, int z, int radius, const std::vector<BiomeGenBase*>& allowed);
    // Java returns null when no position matches.
    virtual std::optional<BlockPos> findBiomePosition(int x, int z, int range, const std::vector<BiomeGenBase*>& biomes, Random& random);
    void cleanupCache() { biomeCache.cleanupCache(); }

protected:
    WorldChunkManager();

private:
    std::shared_ptr<GenLayer> genBiomes;
    std::shared_ptr<GenLayer> biomeIndexLayer;
    BiomeCache biomeCache;
    std::vector<BiomeGenBase*> biomesToSpawnIn;
    std::string generatorOptions;
};

class WorldChunkManagerHell : public WorldChunkManager {
public:
    WorldChunkManagerHell(BiomeGenBase* biome, float rainfallIn)
        : biomeGenerator(biome)
        , rainfall(rainfallIn)
    {
    }

    BiomeGenBase* getBiomeGenerator(const BlockPos&) override { return biomeGenerator; }
    std::vector<BiomeGenBase*>& getBiomesForGeneration(std::vector<BiomeGenBase*>& biomes, int x, int z, int width, int height) override;
    std::vector<float>& getRainfall(std::vector<float>& listToReuse, int x, int z, int width, int length) override;
    std::vector<BiomeGenBase*>& loadBlockGeneratorData(std::vector<BiomeGenBase*>& oldBiomeList, int x, int z, int width, int depth) override;
    std::vector<BiomeGenBase*>& getBiomeGenAt(std::vector<BiomeGenBase*>& listToReuse, int x, int z, int width, int length, bool cacheFlag) override;
    std::optional<BlockPos> findBiomePosition(int x, int z, int range, const std::vector<BiomeGenBase*>& biomes, Random& random) override;
    bool areBiomesViable(int x, int z, int radius, const std::vector<BiomeGenBase*>& allowed) override;

private:
    BiomeGenBase* biomeGenerator;
    float rainfall;
};

} // namespace mc

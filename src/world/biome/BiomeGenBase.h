#pragma once

// Original: net/minecraft/world/biome/BiomeGenBase.java (with Height, SpawnListEntry and TempCategory).
// The biome table is built by registerBiomes() from Bootstrap, after the blocks: the Java static initializer.
// The subclasses are in world/biome/Biomes.h.
// SpawnListEntry names the entity by its EntityList name until entities are ported (stage 10).

#include "block/BlockFlower.h"
#include "util/BlockPos.h"

#include <array>
#include <memory>
#include <string>
#include <typeinfo>
#include <unordered_map>
#include <vector>

namespace mc {

class BiomeDecorator;
class ChunkPrimer;
class IBlockState;
class NoiseGeneratorPerlin;
class Random;
class World;
class WorldGenAbstractTree;
class WorldGenBigTree;
class WorldGenDoublePlant;
class WorldGenSwamp;
class WorldGenTrees;
class WorldGenerator;

enum class EnumCreatureType { MONSTER, CREATURE, AMBIENT, WATER_CREATURE };

class BiomeGenBase {
public:
    struct Height {
        float rootHeight;
        float variation;

        Height attenuate() const { return Height { rootHeight * 0.8f, variation * 0.6f }; }
    };

    // Java: extends WeightedRandom.Item
    struct SpawnListEntry {
        std::string entityName;
        int itemWeight;
        int minGroupCount;
        int maxGroupCount;
    };

    enum class TempCategory { OCEAN, COLD, MEDIUM, WARM };

    static const Height height_Default, height_ShallowWaters, height_Oceans, height_DeepOceans, height_LowPlains, height_MidPlains,
        height_LowHills, height_HighPlateaus, height_MidHills, height_Shores, height_RockyWaters, height_LowIslands, height_PartiallySubmerged;

    static inline BiomeGenBase* ocean = nullptr;
    static inline BiomeGenBase* plains = nullptr;
    static inline BiomeGenBase* desert = nullptr;
    static inline BiomeGenBase* extremeHills = nullptr;
    static inline BiomeGenBase* forest = nullptr;
    static inline BiomeGenBase* taiga = nullptr;
    static inline BiomeGenBase* swampland = nullptr;
    static inline BiomeGenBase* river = nullptr;
    static inline BiomeGenBase* hell = nullptr;
    static inline BiomeGenBase* sky = nullptr;
    static inline BiomeGenBase* frozenOcean = nullptr;
    static inline BiomeGenBase* frozenRiver = nullptr;
    static inline BiomeGenBase* icePlains = nullptr;
    static inline BiomeGenBase* iceMountains = nullptr;
    static inline BiomeGenBase* mushroomIsland = nullptr;
    static inline BiomeGenBase* mushroomIslandShore = nullptr;
    static inline BiomeGenBase* beach = nullptr;
    static inline BiomeGenBase* desertHills = nullptr;
    static inline BiomeGenBase* forestHills = nullptr;
    static inline BiomeGenBase* taigaHills = nullptr;
    static inline BiomeGenBase* extremeHillsEdge = nullptr;
    static inline BiomeGenBase* jungle = nullptr;
    static inline BiomeGenBase* jungleHills = nullptr;
    static inline BiomeGenBase* jungleEdge = nullptr;
    static inline BiomeGenBase* deepOcean = nullptr;
    static inline BiomeGenBase* stoneBeach = nullptr;
    static inline BiomeGenBase* coldBeach = nullptr;
    static inline BiomeGenBase* birchForest = nullptr;
    static inline BiomeGenBase* birchForestHills = nullptr;
    static inline BiomeGenBase* roofedForest = nullptr;
    static inline BiomeGenBase* coldTaiga = nullptr;
    static inline BiomeGenBase* coldTaigaHills = nullptr;
    static inline BiomeGenBase* megaTaiga = nullptr;
    static inline BiomeGenBase* megaTaigaHills = nullptr;
    static inline BiomeGenBase* extremeHillsPlus = nullptr;
    static inline BiomeGenBase* savanna = nullptr;
    static inline BiomeGenBase* savannaPlateau = nullptr;
    static inline BiomeGenBase* mesa = nullptr;
    static inline BiomeGenBase* mesaPlateau_F = nullptr;
    static inline BiomeGenBase* mesaPlateau = nullptr;
    static inline BiomeGenBase* field_180279_ad = nullptr;

    // Java: explorationBiomesList (a HashSet; only membership is used) and BIOME_ID_MAP.
    static std::vector<BiomeGenBase*> explorationBiomesList;
    static std::unordered_map<std::string, BiomeGenBase*> BIOME_ID_MAP;

    static void registerBiomes();

    virtual ~BiomeGenBase();
    BiomeGenBase(const BiomeGenBase&) = delete;
    BiomeGenBase& operator=(const BiomeGenBase&) = delete;

    virtual std::shared_ptr<WorldGenAbstractTree> genBigTreeChance(Random& rand);
    // Gets a WorldGen appropriate for this biome.
    virtual std::shared_ptr<WorldGenerator> getRandomWorldGenForGrass(Random& rand);
    virtual BlockFlower::EnumFlowerType pickRandomFlower(Random& rand, const BlockPos& pos);
    virtual int getSkyColorByTemp(float temperatureIn);
    const std::vector<SpawnListEntry>& getSpawnableList(EnumCreatureType creatureType) const;
    bool getEnableSnow() const { return isSnowyBiome(); }
    bool canRain() const { return isSnowyBiome() ? false : enableRain; }
    bool isHighHumidity() const { return rainfall > 0.85f; }
    virtual float getSpawningChance() { return 0.1f; }
    int getIntRainfall() const;
    float getFloatRainfall() const { return rainfall; }
    float getFloatTemperature(const BlockPos& pos) const;
    virtual void decorate(World* worldIn, Random& rand, const BlockPos& pos);
    virtual int getGrassColorAtPos(const BlockPos& pos);
    virtual int getFoliageColorAtPos(const BlockPos& pos);
    bool isSnowyBiome() const { return enableSnow; }
    virtual void genTerrainBlocks(World* worldIn, Random& rand, ChunkPrimer& chunkPrimerIn, int x, int z, double noiseVal);
    void generateBiomeTerrain(World* worldIn, Random& rand, ChunkPrimer& chunkPrimerIn, int x, int z, double noiseVal);
    // Java: getBiomeClass() -- the class identity used by isEqualTo.
    virtual const std::type_info& getBiomeClass() const { return typeid(*this); }
    virtual bool isEqualTo(const BiomeGenBase* biome) const;
    virtual TempCategory getTempCategory() const;

    static const std::array<BiomeGenBase*, 256>& getBiomeGenArray() { return biomeList; }
    // The biome with that id, or null.
    static BiomeGenBase* getBiome(int id) { return getBiomeFromBiomeList(id, nullptr); }
    static BiomeGenBase* getBiomeFromBiomeList(int biomeId, BiomeGenBase* biome);

    std::string biomeName;
    int color = 0;
    int field_150609_ah = 0;
    const IBlockState* topBlock;
    const IBlockState* fillerBlock;
    int fillerBlockMetadata = 5169201;
    float minHeight;
    float maxHeight;
    float temperature;
    float rainfall;
    int waterColorMultiplier;
    std::unique_ptr<BiomeDecorator> theBiomeDecorator;
    const int biomeID;

protected:
    explicit BiomeGenBase(int id);

    // The parts of the Java constructor that call overridable methods (createBiomeDecorator).
    template <class T, class... Args>
    static T* create(Args&&... args)
    {
        auto biome = std::make_unique<T>(std::forward<Args>(args)...);
        T* raw = biome.get();
        ownedBiomes().push_back(std::move(biome));
        return raw;
    }

    virtual std::unique_ptr<BiomeDecorator> createBiomeDecorator();

public:
    // Java: protected, used across the package by the biome table.
    BiomeGenBase* setTemperatureRainfall(float temperatureIn, float rainfallIn);
    BiomeGenBase* setHeight(const Height& heights);
    BiomeGenBase* setDisableRain();
    BiomeGenBase* setEnableSnow();
    BiomeGenBase* setBiomeName(std::string name);
    BiomeGenBase* setFillerBlockMetadata(int meta);
    BiomeGenBase* setColor(int colorIn);
    BiomeGenBase* func_150563_c(int value);
    virtual BiomeGenBase* func_150557_a(int colorIn, bool darken);
    BiomeGenBase* createMutation() { return createMutatedBiome(biomeID + 128); }
    virtual BiomeGenBase* createMutatedBiome(int id);

protected:
    static NoiseGeneratorPerlin& temperatureNoise();
    static NoiseGeneratorPerlin& GRASS_COLOR_NOISE();
    static WorldGenDoublePlant& DOUBLE_PLANT_GENERATOR();

    std::vector<SpawnListEntry> spawnableMonsterList;
    std::vector<SpawnListEntry> spawnableCreatureList;
    std::vector<SpawnListEntry> spawnableWaterCreatureList;
    std::vector<SpawnListEntry> spawnableCaveCreatureList;
    bool enableSnow = false;
    bool enableRain = true;
    std::shared_ptr<WorldGenTrees> worldGeneratorTrees;
    std::shared_ptr<WorldGenBigTree> worldGeneratorBigTree;
    std::shared_ptr<WorldGenSwamp> worldGeneratorSwamp;

    friend class BiomeGenMutated;

private:
    static std::vector<std::unique_ptr<BiomeGenBase>>& ownedBiomes();

    static std::array<BiomeGenBase*, 256> biomeList;
};

} // namespace mc

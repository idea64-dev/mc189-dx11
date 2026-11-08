#pragma once

// Original: the BiomeGenBase subclasses in net/minecraft/world/biome (BiomeGenOcean.java ... BiomeGenMutated.java).
// Classes matter: BiomeGenBase.isEqualTo compares getBiomeClass() and the layers test for jungles and mesas.

#include "world/biome/BiomeGenBase.h"

namespace mc {

class NoiseGeneratorPerlin;
class WorldGenAbstractTree;
class WorldGenBlockBlob;
class WorldGenCanopyTree;
class WorldGenForest;
class WorldGenIcePath;
class WorldGenIceSpike;
class WorldGenMegaPineTree;
class WorldGenSavannaTree;
class WorldGenTaiga1;
class WorldGenTaiga2;

class BiomeGenOcean : public BiomeGenBase {
public:
    explicit BiomeGenOcean(int id);
    TempCategory getTempCategory() const override { return TempCategory::OCEAN; }
};

class BiomeGenPlains : public BiomeGenBase {
public:
    explicit BiomeGenPlains(int id);
    BlockFlower::EnumFlowerType pickRandomFlower(Random& rand, const BlockPos& pos) override;
    void decorate(World* worldIn, Random& rand, const BlockPos& pos) override;

protected:
    BiomeGenBase* createMutatedBiome(int id) override;

    bool field_150628_aC = false;
};

class BiomeGenDesert : public BiomeGenBase {
public:
    explicit BiomeGenDesert(int id);
    void decorate(World* worldIn, Random& rand, const BlockPos& pos) override;
};

class BiomeGenHills : public BiomeGenBase {
public:
    BiomeGenHills(int id, bool extraTrees);
    ~BiomeGenHills() override;
    std::shared_ptr<WorldGenAbstractTree> genBigTreeChance(Random& rand) override;
    void decorate(World* worldIn, Random& rand, const BlockPos& pos) override;
    void genTerrainBlocks(World* worldIn, Random& rand, ChunkPrimer& chunkPrimerIn, int x, int z, double noiseVal) override;

protected:
    BiomeGenBase* createMutatedBiome(int id) override;

private:
    BiomeGenHills* mutateHills(BiomeGenBase* base);

    std::unique_ptr<WorldGenerator> theWorldGenerator;
    std::shared_ptr<WorldGenTaiga2> field_150634_aD;
    int field_150635_aE = 0;
    int field_150636_aF = 1;
    int field_150637_aG = 2;
    int field_150638_aH;
};

class BiomeGenForest : public BiomeGenBase {
public:
    BiomeGenForest(int id, int type);
    std::shared_ptr<WorldGenAbstractTree> genBigTreeChance(Random& rand) override;
    BlockFlower::EnumFlowerType pickRandomFlower(Random& rand, const BlockPos& pos) override;
    void decorate(World* worldIn, Random& rand, const BlockPos& pos) override;
    int getGrassColorAtPos(const BlockPos& pos) override;

    static std::shared_ptr<WorldGenForest> field_150629_aC();
    static std::shared_ptr<WorldGenForest> field_150630_aD();
    static std::shared_ptr<WorldGenCanopyTree> field_150631_aE();

protected:
    BiomeGenBase* func_150557_a(int colorIn, bool darken) override;
    BiomeGenBase* createMutatedBiome(int id) override;

private:
    int field_150632_aF;
};

class BiomeGenTaiga : public BiomeGenBase {
public:
    BiomeGenTaiga(int id, int type);
    std::shared_ptr<WorldGenAbstractTree> genBigTreeChance(Random& rand) override;
    std::shared_ptr<WorldGenerator> getRandomWorldGenForGrass(Random& rand) override;
    void decorate(World* worldIn, Random& rand, const BlockPos& pos) override;
    void genTerrainBlocks(World* worldIn, Random& rand, ChunkPrimer& chunkPrimerIn, int x, int z, double noiseVal) override;

protected:
    BiomeGenBase* createMutatedBiome(int id) override;

private:
    int field_150644_aH;
};

class BiomeGenSwamp : public BiomeGenBase {
public:
    explicit BiomeGenSwamp(int id);
    std::shared_ptr<WorldGenAbstractTree> genBigTreeChance(Random& rand) override;
    int getGrassColorAtPos(const BlockPos& pos) override;
    int getFoliageColorAtPos(const BlockPos&) override { return 6975545; }
    BlockFlower::EnumFlowerType pickRandomFlower(Random&, const BlockPos&) override { return BlockFlower::EnumFlowerType::BLUE_ORCHID; }
    void genTerrainBlocks(World* worldIn, Random& rand, ChunkPrimer& chunkPrimerIn, int x, int z, double noiseVal) override;
};

class BiomeGenRiver : public BiomeGenBase {
public:
    explicit BiomeGenRiver(int id);
};

class BiomeGenHell : public BiomeGenBase {
public:
    explicit BiomeGenHell(int id);
};

class BiomeGenEnd : public BiomeGenBase {
public:
    explicit BiomeGenEnd(int id);
    int getSkyColorByTemp(float) override { return 0; }
};

class BiomeGenSnow : public BiomeGenBase {
public:
    BiomeGenSnow(int id, bool spikes);
    ~BiomeGenSnow() override;
    void decorate(World* worldIn, Random& rand, const BlockPos& pos) override;
    std::shared_ptr<WorldGenAbstractTree> genBigTreeChance(Random& rand) override;

protected:
    BiomeGenBase* createMutatedBiome(int id) override;

private:
    bool field_150615_aC;
    std::unique_ptr<WorldGenIceSpike> field_150616_aD;
    std::unique_ptr<WorldGenIcePath> field_150617_aE;
};

class BiomeGenMushroomIsland : public BiomeGenBase {
public:
    explicit BiomeGenMushroomIsland(int id);
};

class BiomeGenBeach : public BiomeGenBase {
public:
    explicit BiomeGenBeach(int id);
};

class BiomeGenStoneBeach : public BiomeGenBase {
public:
    explicit BiomeGenStoneBeach(int id);
};

class BiomeGenJungle : public BiomeGenBase {
public:
    BiomeGenJungle(int id, bool edge);
    std::shared_ptr<WorldGenAbstractTree> genBigTreeChance(Random& rand) override;
    std::shared_ptr<WorldGenerator> getRandomWorldGenForGrass(Random& rand) override;
    void decorate(World* worldIn, Random& rand, const BlockPos& pos) override;

private:
    bool field_150614_aC;
};

class BiomeGenSavanna : public BiomeGenBase {
public:
    explicit BiomeGenSavanna(int id);
    std::shared_ptr<WorldGenAbstractTree> genBigTreeChance(Random& rand) override;
    void decorate(World* worldIn, Random& rand, const BlockPos& pos) override;

protected:
    BiomeGenBase* createMutatedBiome(int id) override;
};

class BiomeGenMesa : public BiomeGenBase {
public:
    BiomeGenMesa(int id, bool bryce, bool forest);
    ~BiomeGenMesa() override;
    std::shared_ptr<WorldGenAbstractTree> genBigTreeChance(Random& rand) override;
    int getFoliageColorAtPos(const BlockPos&) override { return 10387789; }
    int getGrassColorAtPos(const BlockPos&) override { return 9470285; }
    void genTerrainBlocks(World* worldIn, Random& rand, ChunkPrimer& chunkPrimerIn, int x, int z, double noiseVal) override;

protected:
    BiomeGenBase* createMutatedBiome(int id) override;

private:
    void func_150619_a(int64_t seed);
    const IBlockState* func_180629_a(int x, int y, int z) const;

    std::vector<const IBlockState*> field_150621_aC;
    int64_t field_150622_aD = 0;
    std::unique_ptr<NoiseGeneratorPerlin> field_150623_aE;
    std::unique_ptr<NoiseGeneratorPerlin> field_150624_aF;
    std::unique_ptr<NoiseGeneratorPerlin> field_150625_aG;
    bool field_150626_aH;
    bool field_150620_aI;
};

class BiomeGenMutated : public BiomeGenBase {
public:
    BiomeGenMutated(int id, BiomeGenBase* biome);
    void decorate(World* worldIn, Random& rand, const BlockPos& pos) override;
    void genTerrainBlocks(World* worldIn, Random& rand, ChunkPrimer& chunkPrimerIn, int x, int z, double noiseVal) override;
    float getSpawningChance() override { return baseBiome->getSpawningChance(); }
    std::shared_ptr<WorldGenAbstractTree> genBigTreeChance(Random& rand) override { return baseBiome->genBigTreeChance(rand); }
    int getFoliageColorAtPos(const BlockPos& pos) override { return baseBiome->getFoliageColorAtPos(pos); }
    int getGrassColorAtPos(const BlockPos& pos) override { return baseBiome->getGrassColorAtPos(pos); }
    const std::type_info& getBiomeClass() const override { return baseBiome->getBiomeClass(); }
    bool isEqualTo(const BiomeGenBase* biome) const override { return baseBiome->isEqualTo(biome); }
    TempCategory getTempCategory() const override { return baseBiome->getTempCategory(); }

protected:
    BiomeGenBase* baseBiome;
};

} // namespace mc

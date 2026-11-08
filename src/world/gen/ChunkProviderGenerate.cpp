#include "world/gen/ChunkProviderGenerate.h"

#include "block/AllBlocks.h"
#include "core/Java.h"
#include "init/Blocks.h"
#include "util/MathHelper.h"
#include "world/ChunkCoordIntPair.h"
#include "world/World.h"
#include "world/WorldType.h"
#include "world/biome/WorldChunkManager.h"
#include "world/chunk/Chunk.h"
#include "world/gen/MapGenBase.h"
#include "world/gen/NoiseGenerators.h"
#include "world/gen/feature/WorldGenerators.h"
#include "world/gen/structure/MapGenMineshaft.h"
#include "world/gen/structure/MapGenScatteredFeature.h"
#include "world/gen/structure/MapGenStronghold.h"
#include "world/gen/structure/MapGenVillage.h"
#include "world/gen/structure/StructureOceanMonument.h"
#include "world/storage/WorldInfo.h"

namespace mc {

ChunkProviderGenerate::ChunkProviderGenerate(World* worldIn, int64_t seed, bool generateStructures, const std::string& structuresJson)
    : rand(seed)
    , worldObj(worldIn)
    , mapFeaturesEnabled(generateStructures)
    , field_177475_o(worldIn->getWorldInfo().getTerrainType())
    , oceanBlockTmpl(Blocks::water)
    , stoneNoise(256)
    , caveGenerator(std::make_unique<MapGenCaves>())
    , strongholdGenerator(std::make_unique<MapGenStronghold>())
    , villageGenerator(std::make_unique<MapGenVillage>())
    , mineshaftGenerator(std::make_unique<MapGenMineshaft>())
    , scatteredFeatureGenerator(std::make_unique<MapGenScatteredFeature>())
    , ravineGenerator(std::make_unique<MapGenRavine>())
    , oceanMonumentGenerator(std::make_unique<StructureOceanMonument>())
{
    field_147431_j = std::make_unique<NoiseGeneratorOctaves>(rand, 16);
    field_147432_k = std::make_unique<NoiseGeneratorOctaves>(rand, 16);
    field_147429_l = std::make_unique<NoiseGeneratorOctaves>(rand, 8);
    field_147430_m = std::make_unique<NoiseGeneratorPerlin>(rand, 4);
    noiseGen5 = std::make_unique<NoiseGeneratorOctaves>(rand, 10);
    noiseGen6 = std::make_unique<NoiseGeneratorOctaves>(rand, 16);
    mobSpawnerNoise = std::make_unique<NoiseGeneratorOctaves>(rand, 8);
    for (int i = -2; i <= 2; ++i) {
        for (int j = -2; j <= 2; ++j) {
            const float f = 10.0f / MathHelper::sqrt_float(static_cast<float>(i * i + j * j) + 0.2f);
            parabolicField[static_cast<size_t>(i + 2 + (j + 2) * 5)] = f;
        }
    }
    // Java: only when the options string isn't null, which it never is from the world info.
    settings = ChunkProviderSettings::Factory::jsonToFactory(structuresJson);
    oceanBlockTmpl = settings.useLavaOceans ? Blocks::lava : Blocks::water;
    worldIn->setSeaLevel(settings.seaLevel);
}

ChunkProviderGenerate::~ChunkProviderGenerate() = default;

void ChunkProviderGenerate::setBlocksInChunk(int x, int z, ChunkPrimer& primer)
{
    biomesForGeneration = worldObj->getWorldChunkManager()->getBiomesForGeneration(biomesForGeneration, x * 4 - 2, z * 4 - 2, 10, 10);
    func_147423_a(x * 4, 0, z * 4);
    const IBlockState* const stone = Blocks::stone->getDefaultState();
    const IBlockState* const ocean = oceanBlockTmpl->getDefaultState();
    for (int i = 0; i < 4; ++i) {
        const int j = i * 5;
        const int k = (i + 1) * 5;
        for (int l = 0; l < 4; ++l) {
            const int i1 = (j + l) * 33;
            const int j1 = (j + l + 1) * 33;
            const int k1 = (k + l) * 33;
            const int l1 = (k + l + 1) * 33;
            for (int i2 = 0; i2 < 32; ++i2) {
                const double d0 = 0.125;
                double d1 = field_147434_q[static_cast<size_t>(i1 + i2)];
                double d2 = field_147434_q[static_cast<size_t>(j1 + i2)];
                double d3 = field_147434_q[static_cast<size_t>(k1 + i2)];
                double d4 = field_147434_q[static_cast<size_t>(l1 + i2)];
                const double d5 = (field_147434_q[static_cast<size_t>(i1 + i2 + 1)] - d1) * d0;
                const double d6 = (field_147434_q[static_cast<size_t>(j1 + i2 + 1)] - d2) * d0;
                const double d7 = (field_147434_q[static_cast<size_t>(k1 + i2 + 1)] - d3) * d0;
                const double d8 = (field_147434_q[static_cast<size_t>(l1 + i2 + 1)] - d4) * d0;
                for (int j2 = 0; j2 < 8; ++j2) {
                    const double d9 = 0.25;
                    double d10 = d1;
                    double d11 = d2;
                    const double d12 = (d3 - d1) * d9;
                    const double d13 = (d4 - d2) * d9;
                    for (int k2 = 0; k2 < 4; ++k2) {
                        const double d14 = 0.25;
                        const double d16 = (d11 - d10) * d14;
                        double lvt_45_1_ = d10 - d16;
                        for (int l2 = 0; l2 < 4; ++l2) {
                            if ((lvt_45_1_ += d16) > 0.0)
                                primer.setBlockState(i * 4 + k2, i2 * 8 + j2, l * 4 + l2, stone);
                            else if (i2 * 8 + j2 < settings.seaLevel)
                                primer.setBlockState(i * 4 + k2, i2 * 8 + j2, l * 4 + l2, ocean);
                        }
                        d10 += d12;
                        d11 += d13;
                    }
                    d1 += d5;
                    d2 += d6;
                    d3 += d7;
                    d4 += d8;
                }
            }
        }
    }
}

void ChunkProviderGenerate::replaceBlocksForBiome(int x, int z, ChunkPrimer& primer, const std::vector<BiomeGenBase*>& biomeGens)
{
    const double d0 = 0.03125;
    field_147430_m->getRegion(stoneNoise, static_cast<double>(x * 16), static_cast<double>(z * 16), 16, 16, d0 * 2.0, d0 * 2.0, 1.0);
    for (int i = 0; i < 16; ++i) {
        for (int j = 0; j < 16; ++j) {
            BiomeGenBase* biomegenbase = biomeGens[static_cast<size_t>(j + i * 16)];
            biomegenbase->genTerrainBlocks(worldObj, rand, primer, x * 16 + i, z * 16 + j, stoneNoise[static_cast<size_t>(j + i * 16)]);
        }
    }
}

std::unique_ptr<Chunk> ChunkProviderGenerate::generateChunk(int x, int z)
{
    rand.setSeed(java::add(java::mul(static_cast<int64_t>(x), static_cast<int64_t>(341873128712LL)), java::mul(static_cast<int64_t>(z), static_cast<int64_t>(132897987541LL))));
    auto chunkprimer = std::make_unique<ChunkPrimer>();
    setBlocksInChunk(x, z, *chunkprimer);
    biomesForGeneration = worldObj->getWorldChunkManager()->loadBlockGeneratorData(biomesForGeneration, x * 16, z * 16, 16, 16);
    replaceBlocksForBiome(x, z, *chunkprimer, biomesForGeneration);
    if (settings.useCaves)
        caveGenerator->generate(this, worldObj, x, z, chunkprimer.get());
    if (settings.useRavines)
        ravineGenerator->generate(this, worldObj, x, z, chunkprimer.get());
    if (settings.useMineShafts && mapFeaturesEnabled)
        mineshaftGenerator->generate(this, worldObj, x, z, chunkprimer.get());
    if (settings.useVillages && mapFeaturesEnabled)
        villageGenerator->generate(this, worldObj, x, z, chunkprimer.get());
    if (settings.useStrongholds && mapFeaturesEnabled)
        strongholdGenerator->generate(this, worldObj, x, z, chunkprimer.get());
    if (settings.useTemples && mapFeaturesEnabled)
        scatteredFeatureGenerator->generate(this, worldObj, x, z, chunkprimer.get());
    if (settings.useMonuments && mapFeaturesEnabled)
        oceanMonumentGenerator->generate(this, worldObj, x, z, chunkprimer.get());
    auto chunk = std::make_unique<Chunk>(worldObj, *chunkprimer, x, z);
    std::array<uint8_t, 256>& abyte = chunk->getBiomeArray();
    for (size_t i = 0; i < abyte.size(); ++i)
        abyte[i] = static_cast<uint8_t>(biomesForGeneration[i]->biomeID);
    chunk->generateSkylightMap();
    return chunk;
}

Chunk* ChunkProviderGenerate::provideChunk(int x, int z)
{
    lastProvided = generateChunk(x, z);
    return lastProvided.get();
}

void ChunkProviderGenerate::func_147423_a(int x, int y, int z)
{
    noiseGen6->generateNoiseOctaves(depthNoiseArray, x, z, 5, 5, static_cast<double>(settings.depthNoiseScaleX), static_cast<double>(settings.depthNoiseScaleZ),
        static_cast<double>(settings.depthNoiseScaleExponent));
    const float f = settings.coordinateScale;
    const float f1 = settings.heightScale;
    field_147429_l->generateNoiseOctaves(mainNoiseArray, x, y, z, 5, 33, 5, static_cast<double>(f / settings.mainNoiseScaleX),
        static_cast<double>(f1 / settings.mainNoiseScaleY), static_cast<double>(f / settings.mainNoiseScaleZ));
    field_147431_j->generateNoiseOctaves(lowerLimitNoiseArray, x, y, z, 5, 33, 5, static_cast<double>(f), static_cast<double>(f1), static_cast<double>(f));
    field_147432_k->generateNoiseOctaves(upperLimitNoiseArray, x, y, z, 5, 33, 5, static_cast<double>(f), static_cast<double>(f1), static_cast<double>(f));
    int i = 0;
    int j = 0;
    for (int k = 0; k < 5; ++k) {
        for (int l = 0; l < 5; ++l) {
            float f2 = 0.0f;
            float f3 = 0.0f;
            float f4 = 0.0f;
            const int i1 = 2;
            const BiomeGenBase* biomegenbase = biomesForGeneration[static_cast<size_t>(k + 2 + (l + 2) * 10)];
            for (int j1 = -i1; j1 <= i1; ++j1) {
                for (int k1 = -i1; k1 <= i1; ++k1) {
                    const BiomeGenBase* biomegenbase1 = biomesForGeneration[static_cast<size_t>(k + j1 + 2 + (l + k1 + 2) * 10)];
                    float f5 = settings.biomeDepthOffset + biomegenbase1->minHeight * settings.biomeDepthWeight;
                    float f6 = settings.biomeScaleOffset + biomegenbase1->maxHeight * settings.biomeScaleWeight;
                    if (field_177475_o == &WorldType::AMPLIFIED && f5 > 0.0f) {
                        f5 = 1.0f + f5 * 2.0f;
                        f6 = 1.0f + f6 * 4.0f;
                    }
                    float f7 = parabolicField[static_cast<size_t>(j1 + 2 + (k1 + 2) * 5)] / (f5 + 2.0f);
                    if (biomegenbase1->minHeight > biomegenbase->minHeight)
                        f7 /= 2.0f;
                    f2 += f6 * f7;
                    f3 += f5 * f7;
                    f4 += f7;
                }
            }
            f2 = f2 / f4;
            f3 = f3 / f4;
            f2 = f2 * 0.9f + 0.1f;
            f3 = (f3 * 4.0f - 1.0f) / 8.0f;
            double d7 = depthNoiseArray[static_cast<size_t>(j)] / 8000.0;
            if (d7 < 0.0)
                d7 = -d7 * 0.3;
            d7 = d7 * 3.0 - 2.0;
            if (d7 < 0.0) {
                d7 = d7 / 2.0;
                if (d7 < -1.0)
                    d7 = -1.0;
                d7 = d7 / 1.4;
                d7 = d7 / 2.0;
            } else {
                if (d7 > 1.0)
                    d7 = 1.0;
                d7 = d7 / 8.0;
            }
            ++j;
            double d8 = static_cast<double>(f3);
            const double d9 = static_cast<double>(f2);
            d8 = d8 + d7 * 0.2;
            d8 = d8 * static_cast<double>(settings.baseSize) / 8.0;
            const double d0 = static_cast<double>(settings.baseSize) + d8 * 4.0;
            for (int l1 = 0; l1 < 33; ++l1) {
                double d1 = (static_cast<double>(l1) - d0) * static_cast<double>(settings.stretchY) * 128.0 / 256.0 / d9;
                if (d1 < 0.0)
                    d1 *= 4.0;
                const double d2 = lowerLimitNoiseArray[static_cast<size_t>(i)] / static_cast<double>(settings.lowerLimitScale);
                const double d3 = upperLimitNoiseArray[static_cast<size_t>(i)] / static_cast<double>(settings.upperLimitScale);
                const double d4 = (mainNoiseArray[static_cast<size_t>(i)] / 10.0 + 1.0) / 2.0;
                double d5 = MathHelper::denormalizeClamp(d2, d3, d4) - d1;
                if (l1 > 29) {
                    const double d6 = static_cast<double>(static_cast<float>(l1 - 29) / 3.0f);
                    d5 = d5 * (1.0 - d6) + -10.0 * d6;
                }
                field_147434_q[static_cast<size_t>(i)] = d5;
                ++i;
            }
        }
    }
}

void ChunkProviderGenerate::populate(IChunkProvider*, int x, int z)
{
    BlockFalling::fallInstantly = true;
    const int i = x * 16;
    const int j = z * 16;
    BlockPos blockpos(i, 0, j);
    BiomeGenBase* biomegenbase = worldObj->getBiomeGenForCoords(blockpos.add(16, 0, 16));
    rand.setSeed(worldObj->getSeed());
    const int64_t k = java::add(java::mul(rand.nextLong() / 2LL, static_cast<int64_t>(2)), static_cast<int64_t>(1));
    const int64_t l = java::add(java::mul(rand.nextLong() / 2LL, static_cast<int64_t>(2)), static_cast<int64_t>(1));
    rand.setSeed(java::add(java::mul(static_cast<int64_t>(x), k), java::mul(static_cast<int64_t>(z), l)) ^ worldObj->getSeed());
    bool flag = false;
    const ChunkCoordIntPair chunkcoordintpair(x, z);
    if (settings.useMineShafts && mapFeaturesEnabled)
        mineshaftGenerator->generateStructure(worldObj, rand, chunkcoordintpair);
    if (settings.useVillages && mapFeaturesEnabled)
        flag = villageGenerator->generateStructure(worldObj, rand, chunkcoordintpair);
    if (settings.useStrongholds && mapFeaturesEnabled)
        strongholdGenerator->generateStructure(worldObj, rand, chunkcoordintpair);
    if (settings.useTemples && mapFeaturesEnabled)
        scatteredFeatureGenerator->generateStructure(worldObj, rand, chunkcoordintpair);
    if (settings.useMonuments && mapFeaturesEnabled)
        oceanMonumentGenerator->generateStructure(worldObj, rand, chunkcoordintpair);
    if (biomegenbase != BiomeGenBase::desert && biomegenbase != BiomeGenBase::desertHills && settings.useWaterLakes && !flag
        && rand.nextInt(settings.waterLakeChance) == 0) {
        const int i1 = rand.nextInt(16) + 8;
        const int j1 = rand.nextInt(256);
        const int k1 = rand.nextInt(16) + 8;
        WorldGenLakes(Blocks::water).generate(worldObj, rand, blockpos.add(i1, j1, k1));
    }
    if (!flag && rand.nextInt(settings.lavaLakeChance / 10) == 0 && settings.useLavaLakes) {
        const int i2 = rand.nextInt(16) + 8;
        const int a = rand.nextInt(248) + 8;
        const int l2 = rand.nextInt(a);
        const int k3 = rand.nextInt(16) + 8;
        if (l2 < worldObj->getSeaLevel() || rand.nextInt(settings.lavaLakeChance / 8) == 0)
            WorldGenLakes(Blocks::lava).generate(worldObj, rand, blockpos.add(i2, l2, k3));
    }
    if (settings.useDungeons) {
        for (int j2 = 0; j2 < settings.dungeonChance; ++j2) {
            const int i3 = rand.nextInt(16) + 8;
            const int l3 = rand.nextInt(256);
            const int l1 = rand.nextInt(16) + 8;
            WorldGenDungeons().generate(worldObj, rand, blockpos.add(i3, l3, l1));
        }
    }
    biomegenbase->decorate(worldObj, rand, BlockPos(i, 0, j));
    // Java: SpawnerAnimals.performWorldGenSpawning(world, biome, i + 8, j + 8, 16, 16, rand) -- the animals come
    // with stage 10. It only draws from rand after the blocks are placed.
    blockpos = blockpos.add(8, 0, 8);
    for (int k2 = 0; k2 < 16; ++k2) {
        for (int j3 = 0; j3 < 16; ++j3) {
            const BlockPos blockpos1 = worldObj->getPrecipitationHeight(blockpos.add(k2, 0, j3));
            const BlockPos blockpos2 = blockpos1.down();
            if (worldObj->canBlockFreezeWater(blockpos2))
                worldObj->setBlockState(blockpos2, Blocks::ice->getDefaultState(), 2);
            if (worldObj->canSnowAt(blockpos1, true))
                worldObj->setBlockState(blockpos1, Blocks::snow_layer->getDefaultState(), 2);
        }
    }
    BlockFalling::fallInstantly = false;
}

bool ChunkProviderGenerate::populateChunk(IChunkProvider*, Chunk* chunkIn, int x, int z)
{
    bool flag = false;
    if (settings.useMonuments && mapFeaturesEnabled && chunkIn->getInhabitedTime() < 3600LL)
        flag |= oceanMonumentGenerator->generateStructure(worldObj, rand, ChunkCoordIntPair(x, z));
    return flag;
}

std::vector<BiomeGenBase::SpawnListEntry> ChunkProviderGenerate::getPossibleCreatures(EnumCreatureType creatureType, const BlockPos& pos)
{
    BiomeGenBase* biomegenbase = worldObj->getBiomeGenForCoords(pos);
    if (mapFeaturesEnabled) {
        if (creatureType == EnumCreatureType::MONSTER && scatteredFeatureGenerator->func_175798_a(pos))
            return scatteredFeatureGenerator->getScatteredFeatureSpawnList();
        if (creatureType == EnumCreatureType::MONSTER && settings.useMonuments && oceanMonumentGenerator->isPositionInStructure(worldObj, pos))
            return oceanMonumentGenerator->getScatteredFeatureSpawnList();
    }
    return biomegenbase->getSpawnableList(creatureType);
}

std::optional<BlockPos> ChunkProviderGenerate::getStrongholdGen(World* worldIn, const std::string& structureName, const BlockPos& position)
{
    if (structureName == "Stronghold" && strongholdGenerator)
        return strongholdGenerator->getClosestStrongholdPos(worldIn, position);
    return std::nullopt;
}

void ChunkProviderGenerate::recreateStructures(Chunk*, int x, int z)
{
    if (settings.useMineShafts && mapFeaturesEnabled)
        mineshaftGenerator->generate(this, worldObj, x, z, nullptr);
    if (settings.useVillages && mapFeaturesEnabled)
        villageGenerator->generate(this, worldObj, x, z, nullptr);
    if (settings.useStrongholds && mapFeaturesEnabled)
        strongholdGenerator->generate(this, worldObj, x, z, nullptr);
    if (settings.useTemples && mapFeaturesEnabled)
        scatteredFeatureGenerator->generate(this, worldObj, x, z, nullptr);
    if (settings.useMonuments && mapFeaturesEnabled)
        oceanMonumentGenerator->generate(this, worldObj, x, z, nullptr);
}

} // namespace mc

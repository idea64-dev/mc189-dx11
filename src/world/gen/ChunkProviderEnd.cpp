#include "world/gen/ChunkProviderEnd.h"

#include "block/AllBlocks.h"
#include "core/Java.h"
#include "init/Blocks.h"
#include "util/MathHelper.h"
#include "world/World.h"
#include "world/biome/WorldChunkManager.h"
#include "world/chunk/Chunk.h"
#include "world/gen/NoiseGenerators.h"

namespace mc {

ChunkProviderEnd::ChunkProviderEnd(World* worldIn, int64_t seed)
    : endRNG(seed)
    , endWorld(worldIn)
{
    noiseGen1 = std::make_unique<NoiseGeneratorOctaves>(endRNG, 16);
    noiseGen2 = std::make_unique<NoiseGeneratorOctaves>(endRNG, 16);
    noiseGen3 = std::make_unique<NoiseGeneratorOctaves>(endRNG, 8);
    noiseGen4 = std::make_unique<NoiseGeneratorOctaves>(endRNG, 10);
    noiseGen5 = std::make_unique<NoiseGeneratorOctaves>(endRNG, 16);
}

ChunkProviderEnd::~ChunkProviderEnd() = default;

void ChunkProviderEnd::func_180520_a(int x, int z, ChunkPrimer& primer)
{
    const int i = 2;
    const int j = i + 1;
    const int k = 33;
    const int l = i + 1;
    initializeNoiseField(densities, x * i, 0, z * i, j, k, l);
    const auto d = [this](int index) { return densities[static_cast<size_t>(index)]; };
    const IBlockState* const air = Blocks::air->getDefaultState();
    const IBlockState* const endStone = Blocks::end_stone->getDefaultState();
    for (int i1 = 0; i1 < i; ++i1) {
        for (int j1 = 0; j1 < i; ++j1) {
            for (int k1 = 0; k1 < 32; ++k1) {
                const double d0 = 0.25;
                double d1 = d(((i1 + 0) * l + j1 + 0) * k + k1 + 0);
                double d2 = d(((i1 + 0) * l + j1 + 1) * k + k1 + 0);
                double d3 = d(((i1 + 1) * l + j1 + 0) * k + k1 + 0);
                double d4 = d(((i1 + 1) * l + j1 + 1) * k + k1 + 0);
                const double d5 = (d(((i1 + 0) * l + j1 + 0) * k + k1 + 1) - d1) * d0;
                const double d6 = (d(((i1 + 0) * l + j1 + 1) * k + k1 + 1) - d2) * d0;
                const double d7 = (d(((i1 + 1) * l + j1 + 0) * k + k1 + 1) - d3) * d0;
                const double d8 = (d(((i1 + 1) * l + j1 + 1) * k + k1 + 1) - d4) * d0;
                for (int l1 = 0; l1 < 4; ++l1) {
                    const double d9 = 0.125;
                    double d10 = d1;
                    double d11 = d2;
                    const double d12 = (d3 - d1) * d9;
                    const double d13 = (d4 - d2) * d9;
                    for (int i2 = 0; i2 < 8; ++i2) {
                        const double d14 = 0.125;
                        double d15 = d10;
                        const double d16 = (d11 - d10) * d14;
                        for (int j2 = 0; j2 < 8; ++j2) {
                            primer.setBlockState(i2 + i1 * 8, l1 + k1 * 4, j2 + j1 * 8, d15 > 0.0 ? endStone : air);
                            d15 += d16;
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

void ChunkProviderEnd::func_180519_a(ChunkPrimer& primer)
{
    // Java looks for stone in a chunk of end stone, so this never changes anything; kept as in the original.
    for (int i = 0; i < 16; ++i) {
        for (int j = 0; j < 16; ++j) {
            const int k = 1;
            int l = -1;
            const IBlockState* iblockstate = Blocks::end_stone->getDefaultState();
            const IBlockState* iblockstate1 = Blocks::end_stone->getDefaultState();
            for (int i1 = 127; i1 >= 0; --i1) {
                const IBlockState* iblockstate2 = primer.getBlockState(i, i1, j);
                if (&iblockstate2->getBlock()->getMaterial() == &Material::air) {
                    l = -1;
                } else if (iblockstate2->getBlock() == Blocks::stone) {
                    if (l == -1) {
                        if (k <= 0) {
                            iblockstate = Blocks::air->getDefaultState();
                            iblockstate1 = Blocks::end_stone->getDefaultState();
                        }
                        l = k;
                        if (i1 >= 0)
                            primer.setBlockState(i, i1, j, iblockstate);
                        else
                            primer.setBlockState(i, i1, j, iblockstate1);
                    } else if (l > 0) {
                        --l;
                        primer.setBlockState(i, i1, j, iblockstate1);
                    }
                }
            }
        }
    }
}

std::unique_ptr<Chunk> ChunkProviderEnd::generateChunk(int x, int z)
{
    endRNG.setSeed(java::add(java::mul(static_cast<int64_t>(x), static_cast<int64_t>(341873128712LL)), java::mul(static_cast<int64_t>(z), static_cast<int64_t>(132897987541LL))));
    auto chunkprimer = std::make_unique<ChunkPrimer>();
    endWorld->getWorldChunkManager()->loadBlockGeneratorData(biomesForGeneration, x * 16, z * 16, 16, 16);
    func_180520_a(x, z, *chunkprimer);
    func_180519_a(*chunkprimer);
    auto chunk = std::make_unique<Chunk>(endWorld, *chunkprimer, x, z);
    std::array<uint8_t, 256>& abyte = chunk->getBiomeArray();
    for (size_t i = 0; i < abyte.size(); ++i)
        abyte[i] = static_cast<uint8_t>(biomesForGeneration[i]->biomeID);
    chunk->generateSkylightMap();
    return chunk;
}

Chunk* ChunkProviderEnd::provideChunk(int x, int z)
{
    lastProvided = generateChunk(x, z);
    return lastProvided.get();
}

std::vector<double>& ChunkProviderEnd::initializeNoiseField(std::vector<double>& field, int x, int y, int z, int xSize, int ySize, int zSize)
{
    if (field.empty())
        field.assign(static_cast<size_t>(xSize * ySize * zSize), 0.0);
    double d0 = 684.412;
    const double d1 = 684.412;
    noiseGen4->generateNoiseOctaves(noiseData4, x, z, xSize, zSize, 1.121, 1.121, 0.5);
    noiseGen5->generateNoiseOctaves(noiseData5, x, z, xSize, zSize, 200.0, 200.0, 0.5);
    d0 = d0 * 2.0;
    noiseGen3->generateNoiseOctaves(noiseData1, x, y, z, xSize, ySize, zSize, d0 / 80.0, d1 / 160.0, d0 / 80.0);
    noiseGen1->generateNoiseOctaves(noiseData2, x, y, z, xSize, ySize, zSize, d0, d1, d0);
    noiseGen2->generateNoiseOctaves(noiseData3, x, y, z, xSize, ySize, zSize, d0, d1, d0);
    int i = 0;
    for (int j = 0; j < xSize; ++j) {
        for (int k = 0; k < zSize; ++k) {
            const float f = static_cast<float>(j + x) / 1.0f;
            const float f1 = static_cast<float>(k + z) / 1.0f;
            float f2 = 100.0f - MathHelper::sqrt_float(f * f + f1 * f1) * 8.0f;
            if (f2 > 80.0f)
                f2 = 80.0f;
            if (f2 < -100.0f)
                f2 = -100.0f;
            for (int l = 0; l < ySize; ++l) {
                double d2 = 0.0;
                const double d3 = noiseData2[static_cast<size_t>(i)] / 512.0;
                const double d4 = noiseData3[static_cast<size_t>(i)] / 512.0;
                const double d5 = (noiseData1[static_cast<size_t>(i)] / 10.0 + 1.0) / 2.0;
                if (d5 < 0.0)
                    d2 = d3;
                else if (d5 > 1.0)
                    d2 = d4;
                else
                    d2 = d3 + (d4 - d3) * d5;
                d2 = d2 - 8.0;
                d2 = d2 + static_cast<double>(f2);
                int i1 = 2;
                if (l > ySize / 2 - i1) {
                    double d6 = static_cast<double>(static_cast<float>(l - (ySize / 2 - i1)) / 64.0f);
                    d6 = MathHelper::clamp_double(d6, 0.0, 1.0);
                    d2 = d2 * (1.0 - d6) + -3000.0 * d6;
                }
                i1 = 8;
                if (l < i1) {
                    const double d7 = static_cast<double>(static_cast<float>(i1 - l) / (static_cast<float>(i1) - 1.0f));
                    d2 = d2 * (1.0 - d7) + -30.0 * d7;
                }
                field[static_cast<size_t>(i)] = d2;
                ++i;
            }
        }
    }
    return field;
}

void ChunkProviderEnd::populate(IChunkProvider*, int x, int z)
{
    BlockFalling::fallInstantly = true;
    const BlockPos blockpos(x * 16, 0, z * 16);
    endWorld->getBiomeGenForCoords(blockpos.add(16, 0, 16))->decorate(endWorld, endWorld->rand, blockpos);
    BlockFalling::fallInstantly = false;
}

std::vector<BiomeGenBase::SpawnListEntry> ChunkProviderEnd::getPossibleCreatures(EnumCreatureType creatureType, const BlockPos& pos)
{
    return endWorld->getBiomeGenForCoords(pos)->getSpawnableList(creatureType);
}

} // namespace mc

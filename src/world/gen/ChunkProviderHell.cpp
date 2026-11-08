#include "world/gen/ChunkProviderHell.h"

#include "block/AllBlocks.h"
#include "core/Java.h"
#include "core/JavaMath.h"
#include "init/Blocks.h"
#include "util/MathHelper.h"
#include "world/ChunkCoordIntPair.h"
#include "world/World.h"
#include "world/biome/WorldChunkManager.h"
#include "world/chunk/Chunk.h"
#include "world/gen/MapGenBase.h"
#include "world/gen/NoiseGenerators.h"
#include "world/gen/feature/WorldGenerators.h"
#include "world/gen/structure/MapGenNetherBridge.h"

namespace mc {

ChunkProviderHell::ChunkProviderHell(World* worldIn, bool generateStructures, int64_t seed)
    : worldObj(worldIn)
    , field_177466_i(generateStructures)
    , hellRNG(seed)
    , slowsandNoise(256)
    , gravelNoise(256)
    , netherrackExclusivityNoise(256)
    , field_177470_t(std::make_unique<WorldGenFire>())
    , field_177469_u(std::make_unique<WorldGenGlowStone1>())
    , field_177468_v(std::make_unique<WorldGenGlowStone2>())
    , field_177467_w(std::make_unique<WorldGenMinable>(Blocks::quartz_ore->getDefaultState(), 14,
          [](const IBlockState* state) { return state && state->getBlock() == Blocks::netherrack; }))
    , field_177473_x(std::make_unique<WorldGenHellLava>(Blocks::flowing_lava, true))
    , field_177472_y(std::make_unique<WorldGenHellLava>(Blocks::flowing_lava, false))
    , field_177471_z(std::make_unique<GeneratorBushFeature>(Blocks::brown_mushroom))
    , field_177465_A(std::make_unique<GeneratorBushFeature>(Blocks::red_mushroom))
    , genNetherBridge(std::make_unique<MapGenNetherBridge>())
    , netherCaveGenerator(std::make_unique<MapGenCavesHell>())
{
    netherNoiseGen1 = std::make_unique<NoiseGeneratorOctaves>(hellRNG, 16);
    netherNoiseGen2 = std::make_unique<NoiseGeneratorOctaves>(hellRNG, 16);
    netherNoiseGen3 = std::make_unique<NoiseGeneratorOctaves>(hellRNG, 8);
    slowsandGravelNoiseGen = std::make_unique<NoiseGeneratorOctaves>(hellRNG, 4);
    netherrackExculsivityNoiseGen = std::make_unique<NoiseGeneratorOctaves>(hellRNG, 4);
    netherNoiseGen6 = std::make_unique<NoiseGeneratorOctaves>(hellRNG, 10);
    netherNoiseGen7 = std::make_unique<NoiseGeneratorOctaves>(hellRNG, 16);
    worldIn->setSeaLevel(63);
}

ChunkProviderHell::~ChunkProviderHell() = default;

void ChunkProviderHell::func_180515_a(int x, int z, ChunkPrimer& primer)
{
    const int i = 4;
    const int j = worldObj->getSeaLevel() / 2 + 1;
    const int k = i + 1;
    const int l = 17;
    const int i1 = i + 1;
    initializeNoiseField(noiseField, x * i, 0, z * i, k, l, i1);
    const auto n = [this](int index) { return noiseField[static_cast<size_t>(index)]; };
    for (int j1 = 0; j1 < i; ++j1) {
        for (int k1 = 0; k1 < i; ++k1) {
            for (int l1 = 0; l1 < 16; ++l1) {
                const double d0 = 0.125;
                double d1 = n(((j1 + 0) * i1 + k1 + 0) * l + l1 + 0);
                double d2 = n(((j1 + 0) * i1 + k1 + 1) * l + l1 + 0);
                double d3 = n(((j1 + 1) * i1 + k1 + 0) * l + l1 + 0);
                double d4 = n(((j1 + 1) * i1 + k1 + 1) * l + l1 + 0);
                const double d5 = (n(((j1 + 0) * i1 + k1 + 0) * l + l1 + 1) - d1) * d0;
                const double d6 = (n(((j1 + 0) * i1 + k1 + 1) * l + l1 + 1) - d2) * d0;
                const double d7 = (n(((j1 + 1) * i1 + k1 + 0) * l + l1 + 1) - d3) * d0;
                const double d8 = (n(((j1 + 1) * i1 + k1 + 1) * l + l1 + 1) - d4) * d0;
                for (int i2 = 0; i2 < 8; ++i2) {
                    const double d9 = 0.25;
                    double d10 = d1;
                    double d11 = d2;
                    const double d12 = (d3 - d1) * d9;
                    const double d13 = (d4 - d2) * d9;
                    for (int j2 = 0; j2 < 4; ++j2) {
                        const double d14 = 0.25;
                        double d15 = d10;
                        const double d16 = (d11 - d10) * d14;
                        for (int k2 = 0; k2 < 4; ++k2) {
                            // Java stores null for air, which reads back as air.
                            const IBlockState* iblockstate = Blocks::air->getDefaultState();
                            if (l1 * 8 + i2 < j)
                                iblockstate = Blocks::lava->getDefaultState();
                            if (d15 > 0.0)
                                iblockstate = Blocks::netherrack->getDefaultState();
                            primer.setBlockState(j2 + j1 * 4, i2 + l1 * 8, k2 + k1 * 4, iblockstate);
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

void ChunkProviderHell::func_180516_b(int x, int z, ChunkPrimer& primer)
{
    const int i = worldObj->getSeaLevel() + 1;
    const double d0 = 0.03125;
    slowsandGravelNoiseGen->generateNoiseOctaves(slowsandNoise, x * 16, z * 16, 0, 16, 16, 1, d0, d0, 1.0);
    slowsandGravelNoiseGen->generateNoiseOctaves(gravelNoise, x * 16, 109, z * 16, 16, 1, 16, d0, 1.0, d0);
    netherrackExculsivityNoiseGen->generateNoiseOctaves(netherrackExclusivityNoise, x * 16, z * 16, 0, 16, 16, 1, d0 * 2.0, d0 * 2.0, d0 * 2.0);
    const IBlockState* const air = Blocks::air->getDefaultState();
    for (int j = 0; j < 16; ++j) {
        for (int k = 0; k < 16; ++k) {
            const size_t index = static_cast<size_t>(j + k * 16);
            const bool flag = slowsandNoise[index] + hellRNG.nextDouble() * 0.2 > 0.0;
            const bool flag1 = gravelNoise[index] + hellRNG.nextDouble() * 0.2 > 0.0;
            const int l = java::d2i(netherrackExclusivityNoise[index] / 3.0 + 3.0 + hellRNG.nextDouble() * 0.25);
            int i1 = -1;
            const IBlockState* iblockstate = Blocks::netherrack->getDefaultState();
            const IBlockState* iblockstate1 = Blocks::netherrack->getDefaultState();
            for (int j1 = 127; j1 >= 0; --j1) {
                if (j1 < 127 - hellRNG.nextInt(5) && j1 > hellRNG.nextInt(5)) {
                    const IBlockState* iblockstate2 = primer.getBlockState(k, j1, j);
                    if (&iblockstate2->getBlock()->getMaterial() != &Material::air) {
                        if (iblockstate2->getBlock() == Blocks::netherrack) {
                            if (i1 == -1) {
                                if (l <= 0) {
                                    iblockstate = nullptr;
                                    iblockstate1 = Blocks::netherrack->getDefaultState();
                                } else if (j1 >= i - 4 && j1 <= i + 1) {
                                    iblockstate = Blocks::netherrack->getDefaultState();
                                    iblockstate1 = Blocks::netherrack->getDefaultState();
                                    if (flag1) {
                                        iblockstate = Blocks::gravel->getDefaultState();
                                        iblockstate1 = Blocks::netherrack->getDefaultState();
                                    }
                                    if (flag) {
                                        iblockstate = Blocks::soul_sand->getDefaultState();
                                        iblockstate1 = Blocks::soul_sand->getDefaultState();
                                    }
                                }
                                if (j1 < i && (!iblockstate || &iblockstate->getBlock()->getMaterial() == &Material::air))
                                    iblockstate = Blocks::lava->getDefaultState();
                                i1 = l;
                                if (j1 >= i - 1)
                                    primer.setBlockState(k, j1, j, iblockstate ? iblockstate : air);
                                else
                                    primer.setBlockState(k, j1, j, iblockstate1);
                            } else if (i1 > 0) {
                                --i1;
                                primer.setBlockState(k, j1, j, iblockstate1);
                            }
                        }
                    } else {
                        i1 = -1;
                    }
                } else {
                    primer.setBlockState(k, j1, j, Blocks::bedrock->getDefaultState());
                }
            }
        }
    }
}

std::unique_ptr<Chunk> ChunkProviderHell::generateChunk(int x, int z)
{
    hellRNG.setSeed(java::add(java::mul(static_cast<int64_t>(x), static_cast<int64_t>(341873128712LL)), java::mul(static_cast<int64_t>(z), static_cast<int64_t>(132897987541LL))));
    auto chunkprimer = std::make_unique<ChunkPrimer>();
    func_180515_a(x, z, *chunkprimer);
    func_180516_b(x, z, *chunkprimer);
    netherCaveGenerator->generate(this, worldObj, x, z, chunkprimer.get());
    if (field_177466_i)
        genNetherBridge->generate(this, worldObj, x, z, chunkprimer.get());
    auto chunk = std::make_unique<Chunk>(worldObj, *chunkprimer, x, z);
    std::vector<BiomeGenBase*> abiomegenbase;
    worldObj->getWorldChunkManager()->loadBlockGeneratorData(abiomegenbase, x * 16, z * 16, 16, 16);
    std::array<uint8_t, 256>& abyte = chunk->getBiomeArray();
    for (size_t i = 0; i < abyte.size(); ++i)
        abyte[i] = static_cast<uint8_t>(abiomegenbase[i]->biomeID);
    chunk->resetRelightChecks();
    return chunk;
}

Chunk* ChunkProviderHell::provideChunk(int x, int z)
{
    lastProvided = generateChunk(x, z);
    return lastProvided.get();
}

std::vector<double>& ChunkProviderHell::initializeNoiseField(std::vector<double>& field, int x, int y, int z, int xSize, int ySize, int zSize)
{
    if (field.empty())
        field.assign(static_cast<size_t>(xSize * ySize * zSize), 0.0);
    const double d0 = 684.412;
    const double d1 = 2053.236;
    netherNoiseGen6->generateNoiseOctaves(noiseData4, x, y, z, xSize, 1, zSize, 1.0, 0.0, 1.0);
    netherNoiseGen7->generateNoiseOctaves(noiseData5, x, y, z, xSize, 1, zSize, 100.0, 0.0, 100.0);
    netherNoiseGen3->generateNoiseOctaves(noiseData1, x, y, z, xSize, ySize, zSize, d0 / 80.0, d1 / 60.0, d0 / 80.0);
    netherNoiseGen1->generateNoiseOctaves(noiseData2, x, y, z, xSize, ySize, zSize, d0, d1, d0);
    netherNoiseGen2->generateNoiseOctaves(noiseData3, x, y, z, xSize, ySize, zSize, d0, d1, d0);
    int i = 0;
    std::vector<double> adouble(static_cast<size_t>(ySize));
    for (int j = 0; j < ySize; ++j) {
        adouble[static_cast<size_t>(j)] = java::mathCos(static_cast<double>(j) * 3.141592653589793 * 6.0 / static_cast<double>(ySize)) * 2.0;
        double d2 = static_cast<double>(j);
        if (j > ySize / 2)
            d2 = static_cast<double>(ySize - 1 - j);
        if (d2 < 4.0) {
            d2 = 4.0 - d2;
            adouble[static_cast<size_t>(j)] -= d2 * d2 * d2 * 10.0;
        }
    }
    for (int l = 0; l < xSize; ++l) {
        for (int i1 = 0; i1 < zSize; ++i1) {
            const double d3 = 0.0;
            for (int k = 0; k < ySize; ++k) {
                double d4 = 0.0;
                const double d5 = adouble[static_cast<size_t>(k)];
                const double d6 = noiseData2[static_cast<size_t>(i)] / 512.0;
                const double d7 = noiseData3[static_cast<size_t>(i)] / 512.0;
                const double d8 = (noiseData1[static_cast<size_t>(i)] / 10.0 + 1.0) / 2.0;
                if (d8 < 0.0)
                    d4 = d6;
                else if (d8 > 1.0)
                    d4 = d7;
                else
                    d4 = d6 + (d7 - d6) * d8;
                d4 = d4 - d5;
                if (k > ySize - 4) {
                    const double d9 = static_cast<double>(static_cast<float>(k - (ySize - 4)) / 3.0f);
                    d4 = d4 * (1.0 - d9) + -10.0 * d9;
                }
                if (static_cast<double>(k) < d3) {
                    double d10 = (d3 - static_cast<double>(k)) / 4.0;
                    d10 = MathHelper::clamp_double(d10, 0.0, 1.0);
                    d4 = d4 * (1.0 - d10) + -10.0 * d10;
                }
                field[static_cast<size_t>(i)] = d4;
                ++i;
            }
        }
    }
    return field;
}

void ChunkProviderHell::populate(IChunkProvider*, int x, int z)
{
    BlockFalling::fallInstantly = true;
    const BlockPos blockpos(x * 16, 0, z * 16);
    const ChunkCoordIntPair chunkcoordintpair(x, z);
    genNetherBridge->generateStructure(worldObj, hellRNG, chunkcoordintpair);
    // blockpos.add(hellRNG.nextInt(xz) + offset, y, hellRNG.nextInt(xz) + offset), in Java's order.
    const auto at = [this, &blockpos](int xz, int offset, auto&& yDraw) {
        const int dx = hellRNG.nextInt(xz) + offset;
        const int dy = yDraw();
        const int dz = hellRNG.nextInt(xz) + offset;
        return blockpos.add(dx, dy, dz);
    };
    const auto y120 = [this] { return hellRNG.nextInt(120) + 4; };
    const auto y128 = [this] { return hellRNG.nextInt(128); };
    const auto y108 = [this] { return hellRNG.nextInt(108) + 10; };
    for (int i = 0; i < 8; ++i)
        field_177472_y->generate(worldObj, hellRNG, at(16, 8, y120));
    // Java re-evaluates both loop bounds, drawing again on every iteration.
    for (int j = 0;; ++j) {
        const int a = hellRNG.nextInt(10) + 1;
        if (!(j < hellRNG.nextInt(a) + 1))
            break;
        field_177470_t->generate(worldObj, hellRNG, at(16, 8, y120));
    }
    for (int k = 0;; ++k) {
        const int a = hellRNG.nextInt(10) + 1;
        if (!(k < hellRNG.nextInt(a)))
            break;
        field_177469_u->generate(worldObj, hellRNG, at(16, 8, y120));
    }
    for (int l = 0; l < 10; ++l)
        field_177468_v->generate(worldObj, hellRNG, at(16, 8, y128));
    if (hellRNG.nextBoolean())
        field_177471_z->generate(worldObj, hellRNG, at(16, 8, y128));
    if (hellRNG.nextBoolean())
        field_177465_A->generate(worldObj, hellRNG, at(16, 8, y128));
    for (int i1 = 0; i1 < 16; ++i1)
        field_177467_w->generate(worldObj, hellRNG, at(16, 0, y108));
    for (int j1 = 0; j1 < 16; ++j1)
        field_177473_x->generate(worldObj, hellRNG, at(16, 0, y108));
    BlockFalling::fallInstantly = false;
}

std::vector<BiomeGenBase::SpawnListEntry> ChunkProviderHell::getPossibleCreatures(EnumCreatureType creatureType, const BlockPos& pos)
{
    if (creatureType == EnumCreatureType::MONSTER) {
        if (genNetherBridge->func_175795_b(pos))
            return genNetherBridge->getSpawnList();
        if (genNetherBridge->isPositionInStructure(worldObj, pos) && worldObj->getBlockState(pos.down())->getBlock() == Blocks::nether_brick)
            return genNetherBridge->getSpawnList();
    }
    return worldObj->getBiomeGenForCoords(pos)->getSpawnableList(creatureType);
}

void ChunkProviderHell::recreateStructures(Chunk*, int x, int z)
{
    genNetherBridge->generate(this, worldObj, x, z, nullptr);
}

} // namespace mc

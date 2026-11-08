#include "world/gen/MapGenBase.h"

#include "block/AllBlocks.h"
#include "core/Java.h"
#include "init/Blocks.h"
#include "util/MathHelper.h"
#include "world/World.h"
#include "world/biome/BiomeGenBase.h"
#include "world/chunk/Chunk.h"

namespace mc {

namespace {

constexpr float kPi = 3.1415927f;
constexpr float kHalfPi = 1.5707964f;

// (random.nextFloat() - random.nextFloat()) * random.nextFloat() * scale, drawn in Java's order.
float wobble(Random& random, float scale)
{
    const float a = random.nextFloat();
    const float b = random.nextFloat();
    const float c = random.nextFloat();
    return (a - b) * c * scale;
}

} // namespace

void MapGenBase::generate(IChunkProvider*, World* worldIn, int x, int z, ChunkPrimer* chunkPrimerIn)
{
    const int i = range;
    worldObj = worldIn;
    rand.setSeed(worldIn->getSeed());
    const int64_t j = rand.nextLong();
    const int64_t k = rand.nextLong();
    for (int l = x - i; l <= x + i; ++l) {
        for (int i1 = z - i; i1 <= z + i; ++i1) {
            const int64_t j1 = java::mul(static_cast<int64_t>(l), j);
            const int64_t k1 = java::mul(static_cast<int64_t>(i1), k);
            rand.setSeed(j1 ^ k1 ^ worldIn->getSeed());
            recursiveGenerate(worldIn, l, i1, x, z, chunkPrimerIn);
        }
    }
}

// --- MapGenCaves ---

void MapGenCaves::func_180703_a(int64_t seed, int chunkX, int chunkZ, ChunkPrimer* primer, double x, double y, double z)
{
    func_180702_a(seed, chunkX, chunkZ, primer, x, y, z, 1.0f + rand.nextFloat() * 6.0f, 0.0f, 0.0f, -1, -1, 0.5);
}

void MapGenCaves::func_180702_a(int64_t seed, int chunkX, int chunkZ, ChunkPrimer* primer, double x, double y, double z, float width, float yaw, float pitch,
    int step, int steps, double heightScale)
{
    const double d0 = static_cast<double>(chunkX * 16 + 8);
    const double d1 = static_cast<double>(chunkZ * 16 + 8);
    float f = 0.0f;
    float f1 = 0.0f;
    Random random(seed);
    if (steps <= 0) {
        const int i = range * 16 - 16;
        steps = i - random.nextInt(i / 4);
    }
    bool flag2 = false;
    if (step == -1) {
        step = steps / 2;
        flag2 = true;
    }
    const int j = random.nextInt(steps / 2) + steps / 4;
    for (const bool flag = random.nextInt(6) == 0; step < steps; ++step) {
        const double d2 = 1.5 + static_cast<double>(MathHelper::sin(static_cast<float>(step) * kPi / static_cast<float>(steps)) * width * 1.0f);
        const double d3 = d2 * heightScale;
        const float f2 = MathHelper::cos(pitch);
        const float f3 = MathHelper::sin(pitch);
        x += static_cast<double>(MathHelper::cos(yaw) * f2);
        y += static_cast<double>(f3);
        z += static_cast<double>(MathHelper::sin(yaw) * f2);
        if (flag)
            pitch = pitch * 0.92f;
        else
            pitch = pitch * 0.7f;
        pitch = pitch + f1 * 0.1f;
        yaw += f * 0.1f;
        f1 = f1 * 0.9f;
        f = f * 0.75f;
        f1 = f1 + wobble(random, 2.0f);
        f = f + wobble(random, 4.0f);
        if (!flag2 && step == j && width > 1.0f && steps > 0) {
            const int64_t seedA = random.nextLong();
            const float widthA = random.nextFloat() * 0.5f + 0.5f;
            func_180702_a(seedA, chunkX, chunkZ, primer, x, y, z, widthA, yaw - kHalfPi, pitch / 3.0f, step, steps, 1.0);
            const int64_t seedB = random.nextLong();
            const float widthB = random.nextFloat() * 0.5f + 0.5f;
            func_180702_a(seedB, chunkX, chunkZ, primer, x, y, z, widthB, yaw + kHalfPi, pitch / 3.0f, step, steps, 1.0);
            return;
        }
        if (flag2 || random.nextInt(4) != 0) {
            const double d4 = x - d0;
            const double d5 = z - d1;
            const double d6 = static_cast<double>(steps - step);
            const double d7 = static_cast<double>(width + 2.0f + 16.0f);
            if (d4 * d4 + d5 * d5 - d6 * d6 > d7 * d7)
                return;
            if (x >= d0 - 16.0 - d2 * 2.0 && z >= d1 - 16.0 - d2 * 2.0 && x <= d0 + 16.0 + d2 * 2.0 && z <= d1 + 16.0 + d2 * 2.0) {
                int k2 = MathHelper::floor_double(x - d2) - chunkX * 16 - 1;
                int k = MathHelper::floor_double(x + d2) - chunkX * 16 + 1;
                int l2 = MathHelper::floor_double(y - d3) - 1;
                int l = MathHelper::floor_double(y + d3) + 1;
                int i3 = MathHelper::floor_double(z - d2) - chunkZ * 16 - 1;
                int i1 = MathHelper::floor_double(z + d2) - chunkZ * 16 + 1;
                if (k2 < 0)
                    k2 = 0;
                if (k > 16)
                    k = 16;
                if (l2 < 1)
                    l2 = 1;
                if (l > 248)
                    l = 248;
                if (i3 < 0)
                    i3 = 0;
                if (i1 > 16)
                    i1 = 16;
                bool flag3 = false;
                for (int j1 = k2; !flag3 && j1 < k; ++j1) {
                    for (int k1 = i3; !flag3 && k1 < i1; ++k1) {
                        for (int l1 = l + 1; !flag3 && l1 >= l2 - 1; --l1) {
                            if (l1 >= 0 && l1 < 256) {
                                const Block* block = primer->getBlockState(j1, l1, k1)->getBlock();
                                if (block == Blocks::flowing_water || block == Blocks::water)
                                    flag3 = true;
                                if (l1 != l2 - 1 && j1 != k2 && j1 != k - 1 && k1 != i3 && k1 != i1 - 1)
                                    l1 = l2;
                            }
                        }
                    }
                }
                if (!flag3) {
                    BlockPos mutablePos;
                    for (int j3 = k2; j3 < k; ++j3) {
                        const double d10 = (static_cast<double>(j3 + chunkX * 16) + 0.5 - x) / d2;
                        for (int i2 = i3; i2 < i1; ++i2) {
                            const double d8 = (static_cast<double>(i2 + chunkZ * 16) + 0.5 - z) / d2;
                            bool flag1 = false;
                            if (d10 * d10 + d8 * d8 < 1.0) {
                                for (int j2 = l; j2 > l2; --j2) {
                                    const double d9 = (static_cast<double>(j2 - 1) + 0.5 - y) / d3;
                                    if (d9 > -0.7 && d10 * d10 + d9 * d9 + d8 * d8 < 1.0) {
                                        const IBlockState* iblockstate1 = primer->getBlockState(j3, j2, i2);
                                        const IBlockState* iblockstate2 = primer->getBlockState(j3, j2 + 1, i2);
                                        if (iblockstate1->getBlock() == Blocks::grass || iblockstate1->getBlock() == Blocks::mycelium)
                                            flag1 = true;
                                        if (func_175793_a(iblockstate1, iblockstate2)) {
                                            if (j2 - 1 < 10) {
                                                primer->setBlockState(j3, j2, i2, Blocks::lava->getDefaultState());
                                            } else {
                                                primer->setBlockState(j3, j2, i2, Blocks::air->getDefaultState());
                                                if (iblockstate2->getBlock() == Blocks::sand) {
                                                    primer->setBlockState(j3, j2 + 1, i2,
                                                        iblockstate2->getValue(BlockSand::VARIANT) == BlockSand::EnumType::RED_SAND ? Blocks::red_sandstone->getDefaultState()
                                                                                                                                 : Blocks::sandstone->getDefaultState());
                                                }
                                                if (flag1 && primer->getBlockState(j3, j2 - 1, i2)->getBlock() == Blocks::dirt) {
                                                    mutablePos.set(j3 + chunkX * 16, 0, i2 + chunkZ * 16);
                                                    primer->setBlockState(j3, j2 - 1, i2,
                                                        worldObj->getBiomeGenForCoords(mutablePos)->topBlock->getBlock()->getDefaultState());
                                                }
                                            }
                                        }
                                    }
                                }
                            }
                        }
                    }
                    if (flag2)
                        break;
                }
            }
        }
    }
}

bool MapGenCaves::func_175793_a(const IBlockState* state, const IBlockState* above)
{
    const Block* block = state->getBlock();
    return block == Blocks::stone ? true
        : (block == Blocks::dirt ? true
        : (block == Blocks::grass ? true
        : (block == Blocks::hardened_clay ? true
        : (block == Blocks::stained_hardened_clay ? true
        : (block == Blocks::sandstone ? true
        : (block == Blocks::red_sandstone ? true
        : (block == Blocks::mycelium ? true
        : (block == Blocks::snow_layer ? true
        : (block == Blocks::sand || block == Blocks::gravel) && &above->getBlock()->getMaterial() != &Material::water))))))));
}

void MapGenCaves::recursiveGenerate(World*, int chunkX, int chunkZ, int originX, int originZ, ChunkPrimer* chunkPrimerIn)
{
    const int a = rand.nextInt(15) + 1;
    const int b = rand.nextInt(a) + 1;
    int i = rand.nextInt(b);
    if (rand.nextInt(7) != 0)
        i = 0;
    for (int j = 0; j < i; ++j) {
        const double d0 = static_cast<double>(chunkX * 16 + rand.nextInt(16));
        const int c = rand.nextInt(120) + 8;
        const double d1 = static_cast<double>(rand.nextInt(c));
        const double d2 = static_cast<double>(chunkZ * 16 + rand.nextInt(16));
        int k = 1;
        if (rand.nextInt(4) == 0) {
            const int64_t seed = rand.nextLong();
            func_180703_a(seed, originX, originZ, chunkPrimerIn, d0, d1, d2);
            k += rand.nextInt(4);
        }
        for (int l = 0; l < k; ++l) {
            const float f = rand.nextFloat() * kPi * 2.0f;
            const float f1 = (rand.nextFloat() - 0.5f) * 2.0f / 8.0f;
            const float e = rand.nextFloat() * 2.0f;
            float f2 = e + rand.nextFloat();
            if (rand.nextInt(10) == 0) {
                const float g = rand.nextFloat();
                f2 *= g * rand.nextFloat() * 3.0f + 1.0f;
            }
            const int64_t seed = rand.nextLong();
            func_180702_a(seed, originX, originZ, chunkPrimerIn, d0, d1, d2, f2, f, f1, 0, 0, 1.0);
        }
    }
}

// --- MapGenRavine ---

void MapGenRavine::func_180707_a(int64_t seed, int chunkX, int chunkZ, ChunkPrimer* primer, double x, double y, double z, float width, float yaw, float pitch,
    int step, int steps, double heightScale)
{
    Random random(seed);
    const double d0 = static_cast<double>(chunkX * 16 + 8);
    const double d1 = static_cast<double>(chunkZ * 16 + 8);
    float f = 0.0f;
    float f1 = 0.0f;
    if (steps <= 0) {
        const int i = range * 16 - 16;
        steps = i - random.nextInt(i / 4);
    }
    bool flag1 = false;
    if (step == -1) {
        step = steps / 2;
        flag1 = true;
    }
    float f2 = 1.0f;
    for (int j = 0; j < 256; ++j) {
        if (j == 0 || random.nextInt(3) == 0) {
            const float a = random.nextFloat();
            f2 = 1.0f + a * random.nextFloat() * 1.0f;
        }
        field_75046_d[static_cast<size_t>(j)] = f2 * f2;
    }
    for (; step < steps; ++step) {
        double d9 = 1.5 + static_cast<double>(MathHelper::sin(static_cast<float>(step) * kPi / static_cast<float>(steps)) * width * 1.0f);
        double d2 = d9 * heightScale;
        d9 = d9 * (static_cast<double>(random.nextFloat()) * 0.25 + 0.75);
        d2 = d2 * (static_cast<double>(random.nextFloat()) * 0.25 + 0.75);
        const float f3 = MathHelper::cos(pitch);
        const float f4 = MathHelper::sin(pitch);
        x += static_cast<double>(MathHelper::cos(yaw) * f3);
        y += static_cast<double>(f4);
        z += static_cast<double>(MathHelper::sin(yaw) * f3);
        pitch = pitch * 0.7f;
        pitch = pitch + f1 * 0.05f;
        yaw += f * 0.05f;
        f1 = f1 * 0.8f;
        f = f * 0.5f;
        f1 = f1 + wobble(random, 2.0f);
        f = f + wobble(random, 4.0f);
        if (flag1 || random.nextInt(4) != 0) {
            const double d3 = x - d0;
            const double d4 = z - d1;
            const double d5 = static_cast<double>(steps - step);
            const double d6 = static_cast<double>(width + 2.0f + 16.0f);
            if (d3 * d3 + d4 * d4 - d5 * d5 > d6 * d6)
                return;
            if (x >= d0 - 16.0 - d9 * 2.0 && z >= d1 - 16.0 - d9 * 2.0 && x <= d0 + 16.0 + d9 * 2.0 && z <= d1 + 16.0 + d9 * 2.0) {
                int k2 = MathHelper::floor_double(x - d9) - chunkX * 16 - 1;
                int k = MathHelper::floor_double(x + d9) - chunkX * 16 + 1;
                int l2 = MathHelper::floor_double(y - d2) - 1;
                int l = MathHelper::floor_double(y + d2) + 1;
                int i3 = MathHelper::floor_double(z - d9) - chunkZ * 16 - 1;
                int i1 = MathHelper::floor_double(z + d9) - chunkZ * 16 + 1;
                if (k2 < 0)
                    k2 = 0;
                if (k > 16)
                    k = 16;
                if (l2 < 1)
                    l2 = 1;
                if (l > 248)
                    l = 248;
                if (i3 < 0)
                    i3 = 0;
                if (i1 > 16)
                    i1 = 16;
                bool flag2 = false;
                for (int j1 = k2; !flag2 && j1 < k; ++j1) {
                    for (int k1 = i3; !flag2 && k1 < i1; ++k1) {
                        for (int l1 = l + 1; !flag2 && l1 >= l2 - 1; --l1) {
                            if (l1 >= 0 && l1 < 256) {
                                const Block* block = primer->getBlockState(j1, l1, k1)->getBlock();
                                if (block == Blocks::flowing_water || block == Blocks::water)
                                    flag2 = true;
                                if (l1 != l2 - 1 && j1 != k2 && j1 != k - 1 && k1 != i3 && k1 != i1 - 1)
                                    l1 = l2;
                            }
                        }
                    }
                }
                if (!flag2) {
                    BlockPos mutablePos;
                    for (int j3 = k2; j3 < k; ++j3) {
                        const double d10 = (static_cast<double>(j3 + chunkX * 16) + 0.5 - x) / d9;
                        for (int i2 = i3; i2 < i1; ++i2) {
                            const double d7 = (static_cast<double>(i2 + chunkZ * 16) + 0.5 - z) / d9;
                            bool flag = false;
                            if (d10 * d10 + d7 * d7 < 1.0) {
                                for (int j2 = l; j2 > l2; --j2) {
                                    const double d8 = (static_cast<double>(j2 - 1) + 0.5 - y) / d2;
                                    if ((d10 * d10 + d7 * d7) * static_cast<double>(field_75046_d[static_cast<size_t>(j2 - 1)]) + d8 * d8 / 6.0 < 1.0) {
                                        const Block* block1 = primer->getBlockState(j3, j2, i2)->getBlock();
                                        if (block1 == Blocks::grass)
                                            flag = true;
                                        if (block1 == Blocks::stone || block1 == Blocks::dirt || block1 == Blocks::grass) {
                                            if (j2 - 1 < 10) {
                                                primer->setBlockState(j3, j2, i2, Blocks::flowing_lava->getDefaultState());
                                            } else {
                                                primer->setBlockState(j3, j2, i2, Blocks::air->getDefaultState());
                                                if (flag && primer->getBlockState(j3, j2 - 1, i2)->getBlock() == Blocks::dirt) {
                                                    mutablePos.set(j3 + chunkX * 16, 0, i2 + chunkZ * 16);
                                                    primer->setBlockState(j3, j2 - 1, i2, worldObj->getBiomeGenForCoords(mutablePos)->topBlock);
                                                }
                                            }
                                        }
                                    }
                                }
                            }
                        }
                    }
                    if (flag1)
                        break;
                }
            }
        }
    }
}

void MapGenRavine::recursiveGenerate(World*, int chunkX, int chunkZ, int originX, int originZ, ChunkPrimer* chunkPrimerIn)
{
    if (rand.nextInt(50) == 0) {
        const double d0 = static_cast<double>(chunkX * 16 + rand.nextInt(16));
        const int a = rand.nextInt(40) + 8;
        const double d1 = static_cast<double>(rand.nextInt(a) + 20);
        const double d2 = static_cast<double>(chunkZ * 16 + rand.nextInt(16));
        const int i = 1;
        for (int j = 0; j < i; ++j) {
            const float f = rand.nextFloat() * kPi * 2.0f;
            const float f1 = (rand.nextFloat() - 0.5f) * 2.0f / 8.0f;
            const float e = rand.nextFloat() * 2.0f;
            const float f2 = (e + rand.nextFloat()) * 2.0f;
            const int64_t seed = rand.nextLong();
            func_180707_a(seed, originX, originZ, chunkPrimerIn, d0, d1, d2, f2, f, f1, 0, 0, 3.0);
        }
    }
}

// --- MapGenCavesHell ---

void MapGenCavesHell::func_180705_a(int64_t seed, int chunkX, int chunkZ, ChunkPrimer* primer, double x, double y, double z)
{
    func_180704_a(seed, chunkX, chunkZ, primer, x, y, z, 1.0f + rand.nextFloat() * 6.0f, 0.0f, 0.0f, -1, -1, 0.5);
}

void MapGenCavesHell::func_180704_a(int64_t seed, int chunkX, int chunkZ, ChunkPrimer* primer, double x, double y, double z, float width, float yaw, float pitch,
    int step, int steps, double heightScale)
{
    const double d0 = static_cast<double>(chunkX * 16 + 8);
    const double d1 = static_cast<double>(chunkZ * 16 + 8);
    float f = 0.0f;
    float f1 = 0.0f;
    Random random(seed);
    if (steps <= 0) {
        const int i = range * 16 - 16;
        steps = i - random.nextInt(i / 4);
    }
    bool flag1 = false;
    if (step == -1) {
        step = steps / 2;
        flag1 = true;
    }
    const int j = random.nextInt(steps / 2) + steps / 4;
    for (const bool flag = random.nextInt(6) == 0; step < steps; ++step) {
        const double d2 = 1.5 + static_cast<double>(MathHelper::sin(static_cast<float>(step) * kPi / static_cast<float>(steps)) * width * 1.0f);
        const double d3 = d2 * heightScale;
        const float f2 = MathHelper::cos(pitch);
        const float f3 = MathHelper::sin(pitch);
        x += static_cast<double>(MathHelper::cos(yaw) * f2);
        y += static_cast<double>(f3);
        z += static_cast<double>(MathHelper::sin(yaw) * f2);
        if (flag)
            pitch = pitch * 0.92f;
        else
            pitch = pitch * 0.7f;
        pitch = pitch + f1 * 0.1f;
        yaw += f * 0.1f;
        f1 = f1 * 0.9f;
        f = f * 0.75f;
        f1 = f1 + wobble(random, 2.0f);
        f = f + wobble(random, 4.0f);
        if (!flag1 && step == j && width > 1.0f) {
            const int64_t seedA = random.nextLong();
            const float widthA = random.nextFloat() * 0.5f + 0.5f;
            func_180704_a(seedA, chunkX, chunkZ, primer, x, y, z, widthA, yaw - kHalfPi, pitch / 3.0f, step, steps, 1.0);
            const int64_t seedB = random.nextLong();
            const float widthB = random.nextFloat() * 0.5f + 0.5f;
            func_180704_a(seedB, chunkX, chunkZ, primer, x, y, z, widthB, yaw + kHalfPi, pitch / 3.0f, step, steps, 1.0);
            return;
        }
        if (flag1 || random.nextInt(4) != 0) {
            const double d4 = x - d0;
            const double d5 = z - d1;
            const double d6 = static_cast<double>(steps - step);
            const double d7 = static_cast<double>(width + 2.0f + 16.0f);
            if (d4 * d4 + d5 * d5 - d6 * d6 > d7 * d7)
                return;
            if (x >= d0 - 16.0 - d2 * 2.0 && z >= d1 - 16.0 - d2 * 2.0 && x <= d0 + 16.0 + d2 * 2.0 && z <= d1 + 16.0 + d2 * 2.0) {
                int j2 = MathHelper::floor_double(x - d2) - chunkX * 16 - 1;
                int k = MathHelper::floor_double(x + d2) - chunkX * 16 + 1;
                int k2 = MathHelper::floor_double(y - d3) - 1;
                int l = MathHelper::floor_double(y + d3) + 1;
                int l2 = MathHelper::floor_double(z - d2) - chunkZ * 16 - 1;
                int i1 = MathHelper::floor_double(z + d2) - chunkZ * 16 + 1;
                if (j2 < 0)
                    j2 = 0;
                if (k > 16)
                    k = 16;
                if (k2 < 1)
                    k2 = 1;
                if (l > 120)
                    l = 120;
                if (l2 < 0)
                    l2 = 0;
                if (i1 > 16)
                    i1 = 16;
                bool flag2 = false;
                for (int j1 = j2; !flag2 && j1 < k; ++j1) {
                    for (int k1 = l2; !flag2 && k1 < i1; ++k1) {
                        for (int l1 = l + 1; !flag2 && l1 >= k2 - 1; --l1) {
                            if (l1 >= 0 && l1 < 128) {
                                const Block* block = primer->getBlockState(j1, l1, k1)->getBlock();
                                if (block == Blocks::flowing_lava || block == Blocks::lava)
                                    flag2 = true;
                                if (l1 != k2 - 1 && j1 != j2 && j1 != k - 1 && k1 != l2 && k1 != i1 - 1)
                                    l1 = k2;
                            }
                        }
                    }
                }
                if (!flag2) {
                    for (int i3 = j2; i3 < k; ++i3) {
                        const double d10 = (static_cast<double>(i3 + chunkX * 16) + 0.5 - x) / d2;
                        for (int j3 = l2; j3 < i1; ++j3) {
                            const double d8 = (static_cast<double>(j3 + chunkZ * 16) + 0.5 - z) / d2;
                            for (int i2 = l; i2 > k2; --i2) {
                                const double d9 = (static_cast<double>(i2 - 1) + 0.5 - y) / d3;
                                if (d9 > -0.7 && d10 * d10 + d9 * d9 + d8 * d8 < 1.0) {
                                    const Block* block1 = primer->getBlockState(i3, i2, j3)->getBlock();
                                    if (block1 == Blocks::netherrack || block1 == Blocks::dirt || block1 == Blocks::grass)
                                        primer->setBlockState(i3, i2, j3, Blocks::air->getDefaultState());
                                }
                            }
                        }
                    }
                    if (flag1)
                        break;
                }
            }
        }
    }
}

void MapGenCavesHell::recursiveGenerate(World*, int chunkX, int chunkZ, int originX, int originZ, ChunkPrimer* chunkPrimerIn)
{
    const int a = rand.nextInt(10) + 1;
    const int b = rand.nextInt(a) + 1;
    int i = rand.nextInt(b);
    if (rand.nextInt(5) != 0)
        i = 0;
    for (int j = 0; j < i; ++j) {
        const double d0 = static_cast<double>(chunkX * 16 + rand.nextInt(16));
        const double d1 = static_cast<double>(rand.nextInt(128));
        const double d2 = static_cast<double>(chunkZ * 16 + rand.nextInt(16));
        int k = 1;
        if (rand.nextInt(4) == 0) {
            const int64_t seed = rand.nextLong();
            func_180705_a(seed, originX, originZ, chunkPrimerIn, d0, d1, d2);
            k += rand.nextInt(4);
        }
        for (int l = 0; l < k; ++l) {
            const float f = rand.nextFloat() * kPi * 2.0f;
            const float f1 = (rand.nextFloat() - 0.5f) * 2.0f / 8.0f;
            const float e = rand.nextFloat() * 2.0f;
            const float f2 = e + rand.nextFloat();
            const int64_t seed = rand.nextLong();
            func_180704_a(seed, originX, originZ, chunkPrimerIn, d0, d1, d2, f2 * 2.0f, f, f1, 0, 0, 0.5);
        }
    }
}

} // namespace mc

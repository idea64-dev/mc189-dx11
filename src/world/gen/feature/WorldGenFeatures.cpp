// Original: the non-tree features of net/minecraft/world/gen/feature (and world/gen/GeneratorBushFeature.java).

#include "world/gen/feature/WorldGenerators.h"

#include "block/AllBlocks.h"
#include "init/Blocks.h"
#include "init/Items.h"
#include "item/ItemMisc.h"
#include "item/ItemStack.h"
#include "tileentity/TileEntities.h"
#include "util/MathHelper.h"
#include "util/Random.h"
#include "util/WeightedRandomChestContent.h"
#include "world/World.h"
#include "world/WorldProvider.h"
#include "world/biome/BiomeGenBase.h"

#include <array>
#include <cstdlib>

namespace mc {

namespace {

bool isMaterial(const Block* block, const Material& material)
{
    return &block->getMaterial() == &material;
}

// rand.nextInt(n) - rand.nextInt(n), drawn in Java's order.
int spread(Random& rand, int n)
{
    const int a = rand.nextInt(n);
    return a - rand.nextInt(n);
}

// position.add(rand.nextInt(xz) - rand.nextInt(xz), rand.nextInt(y) - rand.nextInt(y), rand.nextInt(xz) - rand.nextInt(xz))
BlockPos scatter(Random& rand, const BlockPos& position, int xz, int y)
{
    const int dx = spread(rand, xz);
    const int dy = spread(rand, y);
    const int dz = spread(rand, xz);
    return position.add(dx, dy, dz);
}

// Walks down through air and leaves.
BlockPos groundBelow(World* worldIn, BlockPos position)
{
    const Block* block;
    while ((block = worldIn->getBlockState(position)->getBlock(), isMaterial(block, Material::air) || isMaterial(block, Material::leaves))
        && position.getY() > 0)
        position = position.down();
    return position;
}

} // namespace

// --- plants ---

WorldGenTallGrass::WorldGenTallGrass(BlockTallGrass::EnumType type)
    : tallGrassState(Blocks::tallgrass->getDefaultState()->withProperty(BlockTallGrass::TYPE, type))
{
}

bool WorldGenTallGrass::generate(World* worldIn, Random& rand, const BlockPos& positionIn)
{
    const BlockPos position = groundBelow(worldIn, positionIn);
    for (int i = 0; i < 128; ++i) {
        const BlockPos blockpos = scatter(rand, position, 8, 4);
        if (worldIn->isAirBlock(blockpos) && Blocks::tallgrass->canBlockStay(worldIn, blockpos, tallGrassState))
            worldIn->setBlockState(blockpos, tallGrassState, 2);
    }
    return true;
}

bool WorldGenDoublePlant::generate(World* worldIn, Random& rand, const BlockPos& position)
{
    bool flag = false;
    for (int i = 0; i < 64; ++i) {
        const BlockPos blockpos = scatter(rand, position, 8, 4);
        if (worldIn->isAirBlock(blockpos) && (!worldIn->provider->getHasNoSky() || blockpos.getY() < 254)
            && Blocks::double_plant->canPlaceBlockAt(worldIn, blockpos)) {
            Blocks::double_plant->placeAt(worldIn, blockpos, field_150549_a, 2);
            flag = true;
        }
    }
    return flag;
}

void WorldGenFlowers::setGeneratedBlock(BlockFlower* flowerIn, BlockFlower::EnumFlowerType type)
{
    flower = flowerIn;
    field_175915_b = flowerIn->getDefaultState()->withProperty(flowerIn->getTypeProperty(), type);
}

bool WorldGenFlowers::generate(World* worldIn, Random& rand, const BlockPos& position)
{
    for (int i = 0; i < 64; ++i) {
        const BlockPos blockpos = scatter(rand, position, 8, 4);
        if (worldIn->isAirBlock(blockpos) && (!worldIn->provider->getHasNoSky() || blockpos.getY() < 255)
            && flower->canBlockStay(worldIn, blockpos, field_175915_b))
            worldIn->setBlockState(blockpos, field_175915_b, 2);
    }
    return true;
}

bool GeneratorBushFeature::generate(World* worldIn, Random& rand, const BlockPos& position)
{
    for (int i = 0; i < 64; ++i) {
        const BlockPos blockpos = scatter(rand, position, 8, 4);
        if (worldIn->isAirBlock(blockpos) && (!worldIn->provider->getHasNoSky() || blockpos.getY() < 255)
            && field_175908_a->canBlockStay(worldIn, blockpos, field_175908_a->getDefaultState()))
            worldIn->setBlockState(blockpos, field_175908_a->getDefaultState(), 2);
    }
    return true;
}

bool WorldGenDeadBush::generate(World* worldIn, Random& rand, const BlockPos& positionIn)
{
    const BlockPos position = groundBelow(worldIn, positionIn);
    for (int i = 0; i < 4; ++i) {
        const BlockPos blockpos = scatter(rand, position, 8, 4);
        if (worldIn->isAirBlock(blockpos) && Blocks::deadbush->canBlockStay(worldIn, blockpos, Blocks::deadbush->getDefaultState()))
            worldIn->setBlockState(blockpos, Blocks::deadbush->getDefaultState(), 2);
    }
    return true;
}

bool WorldGenCactus::generate(World* worldIn, Random& rand, const BlockPos& position)
{
    for (int i = 0; i < 10; ++i) {
        const BlockPos blockpos = scatter(rand, position, 8, 4);
        if (worldIn->isAirBlock(blockpos)) {
            const int j = 1 + rand.nextInt(rand.nextInt(3) + 1);
            for (int k = 0; k < j; ++k) {
                if (Blocks::cactus->canBlockStay(worldIn, blockpos))
                    worldIn->setBlockState(blockpos.up(k), Blocks::cactus->getDefaultState(), 2);
            }
        }
    }
    return true;
}

bool WorldGenReed::generate(World* worldIn, Random& rand, const BlockPos& position)
{
    for (int i = 0; i < 20; ++i) {
        const int dx = spread(rand, 4);
        const int dz = spread(rand, 4);
        const BlockPos blockpos = position.add(dx, 0, dz);
        if (worldIn->isAirBlock(blockpos)) {
            const BlockPos blockpos1 = blockpos.down();
            if (isMaterial(worldIn->getBlockState(blockpos1.west())->getBlock(), Material::water)
                || isMaterial(worldIn->getBlockState(blockpos1.east())->getBlock(), Material::water)
                || isMaterial(worldIn->getBlockState(blockpos1.north())->getBlock(), Material::water)
                || isMaterial(worldIn->getBlockState(blockpos1.south())->getBlock(), Material::water)) {
                const int j = 2 + rand.nextInt(rand.nextInt(3) + 1);
                for (int k = 0; k < j; ++k) {
                    if (Blocks::reeds->canBlockStay(worldIn, blockpos))
                        worldIn->setBlockState(blockpos.up(k), Blocks::reeds->getDefaultState(), 2);
                }
            }
        }
    }
    return true;
}

bool WorldGenWaterlily::generate(World* worldIn, Random& rand, const BlockPos& position)
{
    for (int i = 0; i < 10; ++i) {
        const int j = position.getX() + spread(rand, 8);
        const int k = position.getY() + spread(rand, 4);
        const int l = position.getZ() + spread(rand, 8);
        const BlockPos pos(j, k, l);
        if (worldIn->isAirBlock(pos) && Blocks::waterlily->canPlaceBlockAt(worldIn, pos))
            worldIn->setBlockState(pos, Blocks::waterlily->getDefaultState(), 2);
    }
    return true;
}

bool WorldGenPumpkin::generate(World* worldIn, Random& rand, const BlockPos& position)
{
    for (int i = 0; i < 64; ++i) {
        const BlockPos blockpos = scatter(rand, position, 8, 4);
        if (worldIn->isAirBlock(blockpos) && worldIn->getBlockState(blockpos.down())->getBlock() == Blocks::grass
            && Blocks::pumpkin->canPlaceBlockAt(worldIn, blockpos))
            worldIn->setBlockState(blockpos, Blocks::pumpkin->getDefaultState()->withProperty(BlockPumpkin::FACING, randomFacing(Plane::HORIZONTAL, rand)), 2);
    }
    return true;
}

bool WorldGenMelon::generate(World* worldIn, Random& rand, const BlockPos& position)
{
    for (int i = 0; i < 64; ++i) {
        const BlockPos blockpos = scatter(rand, position, 8, 4);
        if (Blocks::melon_block->canPlaceBlockAt(worldIn, blockpos) && worldIn->getBlockState(blockpos.down())->getBlock() == Blocks::grass)
            worldIn->setBlockState(blockpos, Blocks::melon_block->getDefaultState(), 2);
    }
    return true;
}

bool WorldGenVines::generate(World* worldIn, Random& rand, const BlockPos& positionIn)
{
    for (BlockPos position = positionIn; position.getY() < 128; position = position.up()) {
        if (worldIn->isAirBlock(position)) {
            for (const EnumFacing enumfacing : horizontalFacings()) {
                if (Blocks::vine->canPlaceBlockOnSide(worldIn, position, enumfacing)) {
                    const IBlockState* iblockstate = Blocks::vine->getDefaultState()
                                                         ->withProperty(BlockVine::NORTH, enumfacing == EnumFacing::NORTH)
                                                         ->withProperty(BlockVine::EAST, enumfacing == EnumFacing::EAST)
                                                         ->withProperty(BlockVine::SOUTH, enumfacing == EnumFacing::SOUTH)
                                                         ->withProperty(BlockVine::WEST, enumfacing == EnumFacing::WEST);
                    worldIn->setBlockState(position, iblockstate, 2);
                    break;
                }
            }
        } else {
            const int dx = spread(rand, 4);
            const int dz = spread(rand, 4);
            position = position.add(dx, 0, dz);
        }
    }
    return true;
}

// --- the Nether ---

bool WorldGenFire::generate(World* worldIn, Random& rand, const BlockPos& position)
{
    for (int i = 0; i < 64; ++i) {
        const BlockPos blockpos = scatter(rand, position, 8, 4);
        if (worldIn->isAirBlock(blockpos) && worldIn->getBlockState(blockpos.down())->getBlock() == Blocks::netherrack)
            worldIn->setBlockState(blockpos, Blocks::fire->getDefaultState(), 2);
    }
    return true;
}

namespace {

bool generateGlowstone(World* worldIn, Random& rand, const BlockPos& position)
{
    if (!worldIn->isAirBlock(position))
        return false;
    if (worldIn->getBlockState(position.up())->getBlock() != Blocks::netherrack)
        return false;
    worldIn->setBlockState(position, Blocks::glowstone->getDefaultState(), 2);
    for (int i = 0; i < 1500; ++i) {
        const int dx = spread(rand, 8);
        const int dy = -rand.nextInt(12);
        const int dz = spread(rand, 8);
        const BlockPos blockpos = position.add(dx, dy, dz);
        if (isMaterial(worldIn->getBlockState(blockpos)->getBlock(), Material::air)) {
            int j = 0;
            for (const EnumFacing enumfacing : kFacings) {
                if (worldIn->getBlockState(blockpos.offset(enumfacing))->getBlock() == Blocks::glowstone)
                    ++j;
                if (j > 1)
                    break;
            }
            if (j == 1)
                worldIn->setBlockState(blockpos, Blocks::glowstone->getDefaultState(), 2);
        }
    }
    return true;
}

} // namespace

bool WorldGenGlowStone1::generate(World* worldIn, Random& rand, const BlockPos& position)
{
    return generateGlowstone(worldIn, rand, position);
}

bool WorldGenGlowStone2::generate(World* worldIn, Random& rand, const BlockPos& position)
{
    return generateGlowstone(worldIn, rand, position);
}

bool WorldGenHellLava::generate(World* worldIn, Random& rand, const BlockPos& position)
{
    if (worldIn->getBlockState(position.up())->getBlock() != Blocks::netherrack)
        return false;
    const Block* here = worldIn->getBlockState(position)->getBlock();
    if (!isMaterial(here, Material::air) && here != Blocks::netherrack)
        return false;
    int i = 0;
    for (const BlockPos& p : { position.west(), position.east(), position.north(), position.south(), position.down() }) {
        if (worldIn->getBlockState(p)->getBlock() == Blocks::netherrack)
            ++i;
    }
    int j = 0;
    for (const BlockPos& p : { position.west(), position.east(), position.north(), position.south(), position.down() }) {
        if (worldIn->isAirBlock(p))
            ++j;
    }
    if ((!field_94524_b && i == 4 && j == 1) || i == 5) {
        worldIn->setBlockState(position, field_150553_a->getDefaultState(), 2);
        worldIn->forceBlockUpdateTick(field_150553_a, position, rand);
    }
    return true;
}

// --- discs ---

namespace {

// WorldGenClay / WorldGenSand / WorldGenIcePath: replaces the blocks matching `replace` in a disc of radius
// rand.nextInt(size - 2) + 2 and height 2 * halfHeight + 1.
template <class Replace>
void disc(World* worldIn, Random& rand, const BlockPos& position, int size, int halfHeight, const IBlockState* state, Replace replace)
{
    const int i = rand.nextInt(size - 2) + 2;
    const int j = halfHeight;
    for (int k = position.getX() - i; k <= position.getX() + i; ++k) {
        for (int l = position.getZ() - i; l <= position.getZ() + i; ++l) {
            const int i1 = k - position.getX();
            const int j1 = l - position.getZ();
            if (i1 * i1 + j1 * j1 <= i * i) {
                for (int k1 = position.getY() - j; k1 <= position.getY() + j; ++k1) {
                    const BlockPos blockpos(k, k1, l);
                    if (replace(worldIn->getBlockState(blockpos)->getBlock()))
                        worldIn->setBlockState(blockpos, state, 2);
                }
            }
        }
    }
}

} // namespace

bool WorldGenClay::generate(World* worldIn, Random& rand, const BlockPos& position)
{
    if (!isMaterial(worldIn->getBlockState(position)->getBlock(), Material::water))
        return false;
    disc(worldIn, rand, position, numberOfBlocks, 1, Blocks::clay->getDefaultState(),
        [](const Block* block) { return block == Blocks::dirt || block == Blocks::clay; });
    return true;
}

bool WorldGenSand::generate(World* worldIn, Random& rand, const BlockPos& position)
{
    if (!isMaterial(worldIn->getBlockState(position)->getBlock(), Material::water))
        return false;
    disc(worldIn, rand, position, radius, 2, block->getDefaultState(), [](const Block* b) { return b == Blocks::dirt || b == Blocks::grass; });
    return true;
}

bool WorldGenIcePath::generate(World* worldIn, Random& rand, const BlockPos& positionIn)
{
    BlockPos position = positionIn;
    while (worldIn->isAirBlock(position) && position.getY() > 2)
        position = position.down();
    if (worldIn->getBlockState(position)->getBlock() != Blocks::snow)
        return false;
    disc(worldIn, rand, position, basePathWidth, 1, Blocks::packed_ice->getDefaultState(),
        [](const Block* b) { return b == Blocks::dirt || b == Blocks::snow || b == Blocks::ice; });
    return true;
}

bool WorldGenIceSpike::generate(World* worldIn, Random& rand, const BlockPos& positionIn)
{
    BlockPos position = positionIn;
    while (worldIn->isAirBlock(position) && position.getY() > 2)
        position = position.down();
    if (worldIn->getBlockState(position)->getBlock() != Blocks::snow)
        return false;
    position = position.up(rand.nextInt(4));
    const int i = rand.nextInt(4) + 7;
    const int j = i / 4 + rand.nextInt(2);
    if (j > 1 && rand.nextInt(60) == 0)
        position = position.up(10 + rand.nextInt(30));
    const auto replaceable = [](const Block* b) { return isMaterial(b, Material::air) || b == Blocks::dirt || b == Blocks::snow || b == Blocks::ice; };
    for (int k = 0; k < i; ++k) {
        const float f = (1.0f - static_cast<float>(k) / static_cast<float>(i)) * static_cast<float>(j);
        const int l = MathHelper::ceiling_float_int(f);
        for (int i1 = -l; i1 <= l; ++i1) {
            const float f1 = static_cast<float>(MathHelper::abs_int(i1)) - 0.25f;
            for (int j1 = -l; j1 <= l; ++j1) {
                const float f2 = static_cast<float>(MathHelper::abs_int(j1)) - 0.25f;
                if (((i1 == 0 && j1 == 0) || f1 * f1 + f2 * f2 <= f * f) && ((i1 != -l && i1 != l && j1 != -l && j1 != l) || rand.nextFloat() <= 0.75f)) {
                    if (replaceable(worldIn->getBlockState(position.add(i1, k, j1))->getBlock()))
                        setBlockAndNotifyAdequately(worldIn, position.add(i1, k, j1), Blocks::packed_ice->getDefaultState());
                    if (k != 0 && l > 1) {
                        if (replaceable(worldIn->getBlockState(position.add(i1, -k, j1))->getBlock()))
                            setBlockAndNotifyAdequately(worldIn, position.add(i1, -k, j1), Blocks::packed_ice->getDefaultState());
                    }
                }
            }
        }
    }
    int k1 = j - 1;
    if (k1 < 0)
        k1 = 0;
    else if (k1 > 1)
        k1 = 1;
    for (int l1 = -k1; l1 <= k1; ++l1) {
        for (int i2 = -k1; i2 <= k1; ++i2) {
            BlockPos blockpos = position.add(l1, -1, i2);
            int j2 = 50;
            if (std::abs(l1) == 1 && std::abs(i2) == 1)
                j2 = rand.nextInt(5);
            while (blockpos.getY() > 50) {
                const Block* block1 = worldIn->getBlockState(blockpos)->getBlock();
                if (!isMaterial(block1, Material::air) && block1 != Blocks::dirt && block1 != Blocks::snow && block1 != Blocks::ice && block1 != Blocks::packed_ice)
                    break;
                setBlockAndNotifyAdequately(worldIn, blockpos, Blocks::packed_ice->getDefaultState());
                blockpos = blockpos.down();
                --j2;
                if (j2 <= 0) {
                    blockpos = blockpos.down(rand.nextInt(5) + 1);
                    j2 = rand.nextInt(5);
                }
            }
        }
    }
    return true;
}

bool WorldGenBlockBlob::generate(World* worldIn, Random& rand, const BlockPos& positionIn)
{
    BlockPos position = positionIn;
    while (true) {
        if (position.getY() > 3) {
            if (worldIn->isAirBlock(position.down())) {
                position = position.down();
                continue;
            }
            const Block* block = worldIn->getBlockState(position.down())->getBlock();
            if (block != Blocks::grass && block != Blocks::dirt && block != Blocks::stone) {
                position = position.down();
                continue;
            }
        }
        if (position.getY() <= 3)
            return false;
        const int i1 = field_150544_b;
        for (int i = 0; i1 >= 0 && i < 3; ++i) {
            const int j = i1 + rand.nextInt(2);
            const int k = i1 + rand.nextInt(2);
            const int l = i1 + rand.nextInt(2);
            const float f = static_cast<float>(j + k + l) * 0.333f + 0.5f;
            for (const BlockPos& blockpos : BlockPos::getAllInBox(position.add(-j, -k, -l), position.add(j, k, l))) {
                if (blockpos.distanceSq(position) <= static_cast<double>(f * f))
                    worldIn->setBlockState(blockpos, field_150545_a->getDefaultState(), 4);
            }
            const int dx = -(i1 + 1) + rand.nextInt(2 + i1 * 2);
            const int dy = 0 - rand.nextInt(2);
            const int dz = -(i1 + 1) + rand.nextInt(2 + i1 * 2);
            position = position.add(dx, dy, dz);
        }
        return true;
    }
}

bool WorldGenDesertWells::generate(World* worldIn, Random&, const BlockPos& positionIn)
{
    BlockPos position = positionIn;
    const IBlockState* const slab = Blocks::stone_slab->getDefaultState()
                                         ->withProperty(BlockStoneSlab::VARIANT, BlockStoneSlab::EnumType::SAND)
                                         ->withProperty(BlockSlab::HALF, BlockSlab::EnumBlockHalf::BOTTOM);
    const IBlockState* const sandstone = Blocks::sandstone->getDefaultState();
    const IBlockState* const water = Blocks::flowing_water->getDefaultState();
    while (worldIn->isAirBlock(position) && position.getY() > 2)
        position = position.down();
    const IBlockState* ground = worldIn->getBlockState(position);
    // Java: BlockStateHelper.forBlock(Blocks.sand).where(VARIANT, SAND)
    if (ground->getBlock() != Blocks::sand || ground->getValue(BlockSand::VARIANT) != BlockSand::EnumType::SAND)
        return false;
    for (int i = -2; i <= 2; ++i) {
        for (int j = -2; j <= 2; ++j) {
            if (worldIn->isAirBlock(position.add(i, -1, j)) && worldIn->isAirBlock(position.add(i, -2, j)))
                return false;
        }
    }
    for (int l = -1; l <= 0; ++l) {
        for (int l1 = -2; l1 <= 2; ++l1) {
            for (int k = -2; k <= 2; ++k)
                worldIn->setBlockState(position.add(l1, l, k), sandstone, 2);
        }
    }
    worldIn->setBlockState(position, water, 2);
    for (const EnumFacing enumfacing : kPlaneHorizontal)
        worldIn->setBlockState(position.offset(enumfacing), water, 2);
    for (int i1 = -2; i1 <= 2; ++i1) {
        for (int i2 = -2; i2 <= 2; ++i2) {
            if (i1 == -2 || i1 == 2 || i2 == -2 || i2 == 2)
                worldIn->setBlockState(position.add(i1, 1, i2), sandstone, 2);
        }
    }
    worldIn->setBlockState(position.add(2, 1, 0), slab, 2);
    worldIn->setBlockState(position.add(-2, 1, 0), slab, 2);
    worldIn->setBlockState(position.add(0, 1, 2), slab, 2);
    worldIn->setBlockState(position.add(0, 1, -2), slab, 2);
    for (int j1 = -1; j1 <= 1; ++j1) {
        for (int j2 = -1; j2 <= 1; ++j2)
            worldIn->setBlockState(position.add(j1, 4, j2), j1 == 0 && j2 == 0 ? sandstone : slab, 2);
    }
    for (int k1 = 1; k1 <= 3; ++k1) {
        worldIn->setBlockState(position.add(-1, k1, -1), sandstone, 2);
        worldIn->setBlockState(position.add(-1, k1, 1), sandstone, 2);
        worldIn->setBlockState(position.add(1, k1, -1), sandstone, 2);
        worldIn->setBlockState(position.add(1, k1, 1), sandstone, 2);
    }
    return true;
}

bool WorldGenLiquids::generate(World* worldIn, Random& rand, const BlockPos& position)
{
    if (worldIn->getBlockState(position.up())->getBlock() != Blocks::stone)
        return false;
    if (worldIn->getBlockState(position.down())->getBlock() != Blocks::stone)
        return false;
    const Block* here = worldIn->getBlockState(position)->getBlock();
    if (!isMaterial(here, Material::air) && here != Blocks::stone)
        return false;
    int i = 0;
    for (const BlockPos& p : { position.west(), position.east(), position.north(), position.south() }) {
        if (worldIn->getBlockState(p)->getBlock() == Blocks::stone)
            ++i;
    }
    int j = 0;
    for (const BlockPos& p : { position.west(), position.east(), position.north(), position.south() }) {
        if (worldIn->isAirBlock(p))
            ++j;
    }
    if (i == 3 && j == 1) {
        worldIn->setBlockState(position, block->getDefaultState(), 2);
        worldIn->forceBlockUpdateTick(block, position, rand);
    }
    return true;
}

WorldGenMinable::WorldGenMinable(const IBlockState* state, int blockCount)
    : WorldGenMinable(state, blockCount, [](const IBlockState* s) { return s && s->getBlock() == Blocks::stone; })
{
}

bool WorldGenMinable::generate(World* worldIn, Random& rand, const BlockPos& position)
{
    const float f = rand.nextFloat() * 3.1415927f;
    const float n = static_cast<float>(numberOfBlocks);
    const double d0 = static_cast<double>(static_cast<float>(position.getX() + 8) + MathHelper::sin(f) * n / 8.0f);
    const double d1 = static_cast<double>(static_cast<float>(position.getX() + 8) - MathHelper::sin(f) * n / 8.0f);
    const double d2 = static_cast<double>(static_cast<float>(position.getZ() + 8) + MathHelper::cos(f) * n / 8.0f);
    const double d3 = static_cast<double>(static_cast<float>(position.getZ() + 8) - MathHelper::cos(f) * n / 8.0f);
    const double d4 = static_cast<double>(position.getY() + rand.nextInt(3) - 2);
    const double d5 = static_cast<double>(position.getY() + rand.nextInt(3) - 2);
    for (int i = 0; i < numberOfBlocks; ++i) {
        const float f1 = static_cast<float>(i) / n;
        const double d6 = d0 + (d1 - d0) * static_cast<double>(f1);
        const double d7 = d4 + (d5 - d4) * static_cast<double>(f1);
        const double d8 = d2 + (d3 - d2) * static_cast<double>(f1);
        const double d9 = rand.nextDouble() * static_cast<double>(numberOfBlocks) / 16.0;
        const double d10 = static_cast<double>(MathHelper::sin(3.1415927f * f1) + 1.0f) * d9 + 1.0;
        const double d11 = static_cast<double>(MathHelper::sin(3.1415927f * f1) + 1.0f) * d9 + 1.0;
        const int j = MathHelper::floor_double(d6 - d10 / 2.0);
        const int k = MathHelper::floor_double(d7 - d11 / 2.0);
        const int l = MathHelper::floor_double(d8 - d10 / 2.0);
        const int i1 = MathHelper::floor_double(d6 + d10 / 2.0);
        const int j1 = MathHelper::floor_double(d7 + d11 / 2.0);
        const int k1 = MathHelper::floor_double(d8 + d10 / 2.0);
        for (int l1 = j; l1 <= i1; ++l1) {
            const double d12 = (static_cast<double>(l1) + 0.5 - d6) / (d10 / 2.0);
            if (d12 * d12 < 1.0) {
                for (int i2 = k; i2 <= j1; ++i2) {
                    const double d13 = (static_cast<double>(i2) + 0.5 - d7) / (d11 / 2.0);
                    if (d12 * d12 + d13 * d13 < 1.0) {
                        for (int j2 = l; j2 <= k1; ++j2) {
                            const double d14 = (static_cast<double>(j2) + 0.5 - d8) / (d10 / 2.0);
                            if (d12 * d12 + d13 * d13 + d14 * d14 < 1.0) {
                                const BlockPos blockpos(l1, i2, j2);
                                if (predicate(worldIn->getBlockState(blockpos)))
                                    worldIn->setBlockState(blockpos, oreBlock, 2);
                            }
                        }
                    }
                }
            }
        }
    }
    return true;
}

bool WorldGenLakes::generate(World* worldIn, Random& rand, const BlockPos& positionIn)
{
    BlockPos position;
    for (position = positionIn.add(-8, 0, -8); position.getY() > 5 && worldIn->isAirBlock(position); position = position.down()) {
    }
    if (position.getY() <= 4)
        return false;
    position = position.down(4);
    std::array<bool, 2048> aboolean {};
    const int i = rand.nextInt(4) + 4;
    for (int j = 0; j < i; ++j) {
        const double d0 = rand.nextDouble() * 6.0 + 3.0;
        const double d1 = rand.nextDouble() * 4.0 + 2.0;
        const double d2 = rand.nextDouble() * 6.0 + 3.0;
        const double d3 = rand.nextDouble() * (16.0 - d0 - 2.0) + 1.0 + d0 / 2.0;
        const double d4 = rand.nextDouble() * (8.0 - d1 - 4.0) + 2.0 + d1 / 2.0;
        const double d5 = rand.nextDouble() * (16.0 - d2 - 2.0) + 1.0 + d2 / 2.0;
        for (int l = 1; l < 15; ++l) {
            for (int i1 = 1; i1 < 15; ++i1) {
                for (int j1 = 1; j1 < 7; ++j1) {
                    const double d6 = (static_cast<double>(l) - d3) / (d0 / 2.0);
                    const double d7 = (static_cast<double>(j1) - d4) / (d1 / 2.0);
                    const double d8 = (static_cast<double>(i1) - d5) / (d2 / 2.0);
                    const double d9 = d6 * d6 + d7 * d7 + d8 * d8;
                    if (d9 < 1.0)
                        aboolean[static_cast<size_t>((l * 16 + i1) * 8 + j1)] = true;
                }
            }
        }
    }
    const auto at = [&](int x, int z, int y) { return aboolean[static_cast<size_t>((x * 16 + z) * 8 + y)]; };
    const auto border = [&](int x, int z, int y) {
        return !at(x, z, y)
            && ((x < 15 && at(x + 1, z, y)) || (x > 0 && at(x - 1, z, y)) || (z < 15 && at(x, z + 1, y)) || (z > 0 && at(x, z - 1, y))
                || (y < 7 && at(x, z, y + 1)) || (y > 0 && at(x, z, y - 1)));
    };
    for (int k1 = 0; k1 < 16; ++k1) {
        for (int l2 = 0; l2 < 16; ++l2) {
            for (int k = 0; k < 8; ++k) {
                if (border(k1, l2, k)) {
                    const Material& material = worldIn->getBlockState(position.add(k1, k, l2))->getBlock()->getMaterial();
                    if (k >= 4 && material.isLiquid())
                        return false;
                    if (k < 4 && !material.isSolid() && worldIn->getBlockState(position.add(k1, k, l2))->getBlock() != block)
                        return false;
                }
            }
        }
    }
    for (int l1 = 0; l1 < 16; ++l1) {
        for (int i3 = 0; i3 < 16; ++i3) {
            for (int i4 = 0; i4 < 8; ++i4) {
                if (at(l1, i3, i4))
                    worldIn->setBlockState(position.add(l1, i4, i3), i4 >= 4 ? Blocks::air->getDefaultState() : block->getDefaultState(), 2);
            }
        }
    }
    for (int i2 = 0; i2 < 16; ++i2) {
        for (int j3 = 0; j3 < 16; ++j3) {
            for (int j4 = 4; j4 < 8; ++j4) {
                if (at(i2, j3, j4)) {
                    const BlockPos blockpos = position.add(i2, j4 - 1, j3);
                    if (worldIn->getBlockState(blockpos)->getBlock() == Blocks::dirt && worldIn->getLightFor(EnumSkyBlock::SKY, position.add(i2, j4, j3)) > 0) {
                        BiomeGenBase* biomegenbase = worldIn->getBiomeGenForCoords(blockpos);
                        if (biomegenbase->topBlock->getBlock() == Blocks::mycelium)
                            worldIn->setBlockState(blockpos, Blocks::mycelium->getDefaultState(), 2);
                        else
                            worldIn->setBlockState(blockpos, Blocks::grass->getDefaultState(), 2);
                    }
                }
            }
        }
    }
    if (&block->getMaterial() == &Material::lava) {
        for (int j2 = 0; j2 < 16; ++j2) {
            for (int k3 = 0; k3 < 16; ++k3) {
                for (int k4 = 0; k4 < 8; ++k4) {
                    if (border(j2, k3, k4) && (k4 < 4 || rand.nextInt(2) != 0)
                        && worldIn->getBlockState(position.add(j2, k4, k3))->getBlock()->getMaterial().isSolid())
                        worldIn->setBlockState(position.add(j2, k4, k3), Blocks::stone->getDefaultState(), 2);
                }
            }
        }
    }
    if (&block->getMaterial() == &Material::water) {
        for (int k2 = 0; k2 < 16; ++k2) {
            for (int l3 = 0; l3 < 16; ++l3) {
                const int l4 = 4;
                if (worldIn->canBlockFreezeWater(position.add(k2, l4, l3)))
                    worldIn->setBlockState(position.add(k2, l4, l3), Blocks::ice->getDefaultState(), 2);
            }
        }
    }
    return true;
}

namespace {

const std::vector<WeightedRandomChestContent>& dungeonChestContent()
{
    static const std::vector<WeightedRandomChestContent> content {
        { Items::saddle, 0, 1, 1, 10 },
        { Items::iron_ingot, 0, 1, 4, 10 },
        { Items::bread, 0, 1, 1, 10 },
        { Items::wheat, 0, 1, 4, 10 },
        { Items::gunpowder, 0, 1, 4, 10 },
        { Items::string, 0, 1, 4, 10 },
        { Items::bucket, 0, 1, 1, 10 },
        { Items::golden_apple, 0, 1, 1, 1 },
        { Items::redstone, 0, 1, 4, 10 },
        { Items::record_13, 0, 1, 1, 4 },
        { Items::record_cat, 0, 1, 1, 4 },
        { Items::name_tag, 0, 1, 1, 10 },
        { Items::golden_horse_armor, 0, 1, 1, 2 },
        { Items::iron_horse_armor, 0, 1, 1, 5 },
        { Items::diamond_horse_armor, 0, 1, 1, 1 },
    };
    return content;
}

} // namespace

bool WorldGenDungeons::generate(World* worldIn, Random& rand, const BlockPos& position)
{
    const int j = rand.nextInt(2) + 2;
    const int k = -j - 1;
    const int l = j + 1;
    const int k1 = rand.nextInt(2) + 2;
    const int l1 = -k1 - 1;
    const int i2 = k1 + 1;
    int j2 = 0;
    for (int k2 = k; k2 <= l; ++k2) {
        for (int l2 = -1; l2 <= 4; ++l2) {
            for (int i3 = l1; i3 <= i2; ++i3) {
                const BlockPos blockpos = position.add(k2, l2, i3);
                const bool flag = worldIn->getBlockState(blockpos)->getBlock()->getMaterial().isSolid();
                if (l2 == -1 && !flag)
                    return false;
                if (l2 == 4 && !flag)
                    return false;
                if ((k2 == k || k2 == l || i3 == l1 || i3 == i2) && l2 == 0 && worldIn->isAirBlock(blockpos) && worldIn->isAirBlock(blockpos.up()))
                    ++j2;
            }
        }
    }
    if (j2 < 1 || j2 > 5)
        return false;
    for (int k3 = k; k3 <= l; ++k3) {
        for (int i4 = 3; i4 >= -1; --i4) {
            for (int k4 = l1; k4 <= i2; ++k4) {
                const BlockPos blockpos1 = position.add(k3, i4, k4);
                if (k3 != k && i4 != -1 && k4 != l1 && k3 != l && i4 != 4 && k4 != i2) {
                    if (worldIn->getBlockState(blockpos1)->getBlock() != Blocks::chest)
                        worldIn->setBlockToAir(blockpos1);
                } else if (blockpos1.getY() >= 0 && !worldIn->getBlockState(blockpos1.down())->getBlock()->getMaterial().isSolid()) {
                    worldIn->setBlockToAir(blockpos1);
                } else if (worldIn->getBlockState(blockpos1)->getBlock()->getMaterial().isSolid() && worldIn->getBlockState(blockpos1)->getBlock() != Blocks::chest) {
                    if (i4 == -1 && rand.nextInt(4) != 0)
                        worldIn->setBlockState(blockpos1, Blocks::mossy_cobblestone->getDefaultState(), 2);
                    else
                        worldIn->setBlockState(blockpos1, Blocks::cobblestone->getDefaultState(), 2);
                }
            }
        }
    }
    for (int l3 = 0; l3 < 2; ++l3) {
        for (int j4 = 0; j4 < 3; ++j4) {
            const int l4 = position.getX() + rand.nextInt(j * 2 + 1) - j;
            const int i5 = position.getY();
            const int j5 = position.getZ() + rand.nextInt(k1 * 2 + 1) - k1;
            const BlockPos blockpos2(l4, i5, j5);
            if (worldIn->isAirBlock(blockpos2)) {
                int j3 = 0;
                for (const EnumFacing enumfacing : kPlaneHorizontal) {
                    if (worldIn->getBlockState(blockpos2.offset(enumfacing))->getBlock()->getMaterial().isSolid())
                        ++j3;
                }
                if (j3 == 1) {
                    worldIn->setBlockState(blockpos2, Blocks::chest->correctFacing(worldIn, blockpos2, Blocks::chest->getDefaultState()), 2);
                    const auto list = WeightedRandomChestContent::func_177629_a(dungeonChestContent(), { Items::enchanted_book->getRandom(rand) });
                    if (auto* chest = dynamic_cast<TileEntityChest*>(worldIn->getTileEntity(blockpos2)))
                        WeightedRandomChestContent::generateChestContents(rand, list, *chest, 8);
                    break;
                }
            }
        }
    }
    worldIn->setBlockState(position, Blocks::mob_spawner->getDefaultState(), 2);
    if (auto* spawner = dynamic_cast<TileEntityMobSpawner*>(worldIn->getTileEntity(position)))
        spawner->getSpawnerBaseLogic().setEntityName(pickMobSpawner(rand));
    return true;
}

std::string WorldGenDungeons::pickMobSpawner(Random& rand)
{
    static const char* const SPAWNERTYPES[4] { "Skeleton", "Zombie", "Zombie", "Spider" };
    return SPAWNERTYPES[rand.nextInt(4)];
}

bool WorldGenSpikes::generate(World* worldIn, Random& rand, const BlockPos& position)
{
    if (!(worldIn->isAirBlock(position) && worldIn->getBlockState(position.down())->getBlock() == baseBlockRequired))
        return false;
    const int i = rand.nextInt(32) + 6;
    const int j = rand.nextInt(4) + 1;
    BlockPos mutablePos;
    for (int k = position.getX() - j; k <= position.getX() + j; ++k) {
        for (int l = position.getZ() - j; l <= position.getZ() + j; ++l) {
            const int i1 = k - position.getX();
            const int j1 = l - position.getZ();
            if (i1 * i1 + j1 * j1 <= j * j + 1 && worldIn->getBlockState(mutablePos.set(k, position.getY() - 1, l))->getBlock() != baseBlockRequired)
                return false;
        }
    }
    for (int l1 = position.getY(); l1 < position.getY() + i && l1 < 256; ++l1) {
        for (int i2 = position.getX() - j; i2 <= position.getX() + j; ++i2) {
            for (int j2 = position.getZ() - j; j2 <= position.getZ() + j; ++j2) {
                const int k2 = i2 - position.getX();
                const int k1 = j2 - position.getZ();
                if (k2 * k2 + k1 * k1 <= j * j + 1)
                    worldIn->setBlockState(BlockPos(i2, l1, j2), Blocks::obsidian->getDefaultState(), 2);
            }
        }
    }
    // Java: an EntityEnderCrystal on top with a random yaw -- entities come with stage 10; the draw keeps rand in
    // step.
    rand.nextFloat();
    worldIn->setBlockState(position.up(i), Blocks::bedrock->getDefaultState(), 2);
    return true;
}

bool WorldGenBigMushroom::generate(World* worldIn, Random& rand, const BlockPos& position)
{
    // Java keeps the type it picked: the decorator's shared generator then always grows that one.
    if (mushroomType == nullptr)
        mushroomType = rand.nextBoolean() ? Blocks::brown_mushroom_block : Blocks::red_mushroom_block;

    const int i = rand.nextInt(3) + 4;
    bool flag = true;

    if (position.getY() < 1 || position.getY() + i + 1 >= 256)
        return false;

    for (int j = position.getY(); j <= position.getY() + 1 + i; ++j) {
        int k = 3;
        if (j <= position.getY() + 3)
            k = 0;

        for (int l = position.getX() - k; l <= position.getX() + k && flag; ++l) {
            for (int i1 = position.getZ() - k; i1 <= position.getZ() + k && flag; ++i1) {
                if (j >= 0 && j < 256) {
                    const Block* block = worldIn->getBlockState(BlockPos(l, j, i1))->getBlock();
                    if (&block->getMaterial() != &Material::air && &block->getMaterial() != &Material::leaves)
                        flag = false;
                } else {
                    flag = false;
                }
            }
        }
    }

    if (!flag)
        return false;

    const Block* block1 = worldIn->getBlockState(position.down())->getBlock();
    if (block1 != Blocks::dirt && block1 != Blocks::grass && block1 != Blocks::mycelium)
        return false;

    using EnumType = BlockHugeMushroom::EnumType;
    int k2 = position.getY() + i;
    if (mushroomType == Blocks::red_mushroom_block)
        k2 = position.getY() + i - 3;

    for (int l2 = k2; l2 <= position.getY() + i; ++l2) {
        int j3 = 1;
        if (l2 < position.getY() + i)
            ++j3;
        if (mushroomType == Blocks::brown_mushroom_block)
            j3 = 3;

        const int k3 = position.getX() - j3;
        const int l3 = position.getX() + j3;
        const int j1 = position.getZ() - j3;
        const int k1 = position.getZ() + j3;

        for (int l1 = k3; l1 <= l3; ++l1) {
            for (int i2 = j1; i2 <= k1; ++i2) {
                int j2 = 5;
                if (l1 == k3)
                    --j2;
                else if (l1 == l3)
                    ++j2;
                if (i2 == j1)
                    j2 -= 3;
                else if (i2 == k1)
                    j2 += 3;

                EnumType type = BlockHugeMushroom::byMetadata(j2);

                if (mushroomType == Blocks::brown_mushroom_block || l2 < position.getY() + i) {
                    if ((l1 == k3 || l1 == l3) && (i2 == j1 || i2 == k1))
                        continue;
                    if (l1 == position.getX() - (j3 - 1) && i2 == j1)
                        type = EnumType::NORTH_WEST;
                    if (l1 == k3 && i2 == position.getZ() - (j3 - 1))
                        type = EnumType::NORTH_WEST;
                    if (l1 == position.getX() + (j3 - 1) && i2 == j1)
                        type = EnumType::NORTH_EAST;
                    if (l1 == l3 && i2 == position.getZ() - (j3 - 1))
                        type = EnumType::NORTH_EAST;
                    if (l1 == position.getX() - (j3 - 1) && i2 == k1)
                        type = EnumType::SOUTH_WEST;
                    if (l1 == k3 && i2 == position.getZ() + (j3 - 1))
                        type = EnumType::SOUTH_WEST;
                    if (l1 == position.getX() + (j3 - 1) && i2 == k1)
                        type = EnumType::SOUTH_EAST;
                    if (l1 == l3 && i2 == position.getZ() + (j3 - 1))
                        type = EnumType::SOUTH_EAST;
                }

                if (type == EnumType::CENTER && l2 < position.getY() + i)
                    type = EnumType::ALL_INSIDE;

                if (position.getY() >= position.getY() + i - 1 || type != EnumType::ALL_INSIDE) {
                    const BlockPos blockpos(l1, l2, i2);
                    if (!worldIn->getBlockState(blockpos)->getBlock()->isFullBlock())
                        setBlockAndNotifyAdequately(worldIn, blockpos, mushroomType->getDefaultState()->withProperty(BlockHugeMushroom::VARIANT, type));
                }
            }
        }
    }

    for (int i3 = 0; i3 < i; ++i3) {
        const Block* block2 = worldIn->getBlockState(position.up(i3))->getBlock();
        if (!block2->isFullBlock())
            setBlockAndNotifyAdequately(worldIn, position.up(i3), mushroomType->getDefaultState()->withProperty(BlockHugeMushroom::VARIANT, EnumType::STEM));
    }

    return true;
}

bool WorldGeneratorBonusChest::generate(World* worldIn, Random& rand, const BlockPos& positionIn)
{
    BlockPos position = positionIn;
    Block* block;
    while (((block = worldIn->getBlockState(position)->getBlock()), &block->getMaterial() == &Material::air || &block->getMaterial() == &Material::leaves) && position.getY() > 1)
        position = position.down();

    if (position.getY() < 1)
        return false;

    position = position.up();

    for (int i = 0; i < 4; ++i) {
        const int dx1 = rand.nextInt(4);
        const int dx = dx1 - rand.nextInt(4);
        const int dy1 = rand.nextInt(3);
        const int dy = dy1 - rand.nextInt(3);
        const int dz1 = rand.nextInt(4);
        const int dz = dz1 - rand.nextInt(4);
        const BlockPos blockpos = position.add(dx, dy, dz);

        if (worldIn->isAirBlock(blockpos) && World::doesBlockHaveSolidTopSurface(worldIn, blockpos.down())) {
            worldIn->setBlockState(blockpos, Blocks::chest->getDefaultState(), 2);
            if (auto* chest = dynamic_cast<TileEntityChest*>(worldIn->getTileEntity(blockpos)))
                WeightedRandomChestContent::generateChestContents(rand, chestItems, *chest, itemsToGenerateInBonusChest);

            const BlockPos blockpos1 = blockpos.east();
            const BlockPos blockpos2 = blockpos.west();
            const BlockPos blockpos3 = blockpos.north();
            const BlockPos blockpos4 = blockpos.south();

            if (worldIn->isAirBlock(blockpos2) && World::doesBlockHaveSolidTopSurface(worldIn, blockpos2.down()))
                worldIn->setBlockState(blockpos2, Blocks::torch->getDefaultState(), 2);
            if (worldIn->isAirBlock(blockpos1) && World::doesBlockHaveSolidTopSurface(worldIn, blockpos1.down()))
                worldIn->setBlockState(blockpos1, Blocks::torch->getDefaultState(), 2);
            if (worldIn->isAirBlock(blockpos3) && World::doesBlockHaveSolidTopSurface(worldIn, blockpos3.down()))
                worldIn->setBlockState(blockpos3, Blocks::torch->getDefaultState(), 2);
            if (worldIn->isAirBlock(blockpos4) && World::doesBlockHaveSolidTopSurface(worldIn, blockpos4.down()))
                worldIn->setBlockState(blockpos4, Blocks::torch->getDefaultState(), 2);
            return true;
        }
    }

    return false;
}

} // namespace mc

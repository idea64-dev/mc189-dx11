#include "world/biome/BiomeDecorator.h"

#include "block/AllBlocks.h"
#include "init/Blocks.h"
#include "util/Random.h"
#include "world/World.h"
#include "world/biome/BiomeGenBase.h"
#include "world/gen/feature/WorldGenerators.h"
#include "world/storage/WorldInfo.h"

#include <stdexcept>

namespace mc {

BiomeDecorator::BiomeDecorator()
    : clayGen(std::make_unique<WorldGenClay>(4))
    , sandGen(std::make_unique<WorldGenSand>(Blocks::sand, 7))
    , gravelAsSandGen(std::make_unique<WorldGenSand>(Blocks::gravel, 6))
    , yellowFlowerGen(std::make_unique<WorldGenFlowers>(Blocks::yellow_flower, BlockFlower::EnumFlowerType::DANDELION))
    , mushroomBrownGen(std::make_unique<GeneratorBushFeature>(Blocks::brown_mushroom))
    , mushroomRedGen(std::make_unique<GeneratorBushFeature>(Blocks::red_mushroom))
    , bigMushroomGen(std::make_unique<WorldGenBigMushroom>())
    , reedGen(std::make_unique<WorldGenReed>())
    , cactusGen(std::make_unique<WorldGenCactus>())
    , waterlilyGen(std::make_unique<WorldGenWaterlily>())
{
}

BiomeDecorator::~BiomeDecorator() = default;

void BiomeDecorator::decorate(World* worldIn, Random& random, BiomeGenBase* biome, const BlockPos& pos)
{
    if (currentWorld)
        throw std::runtime_error("Already decorating");
    currentWorld = worldIn;
    chunkProviderSettings = ChunkProviderSettings::Factory::jsonToFactory(worldIn->getWorldInfo().getGeneratorOptions());
    randomGenerator = &random;
    field_180294_c = pos;
    const ChunkProviderSettings::Factory& s = *chunkProviderSettings;
    dirtGen = std::make_unique<WorldGenMinable>(Blocks::dirt->getDefaultState(), s.dirtSize);
    gravelGen = std::make_unique<WorldGenMinable>(Blocks::gravel->getDefaultState(), s.gravelSize);
    graniteGen = std::make_unique<WorldGenMinable>(Blocks::stone->getDefaultState()->withProperty(BlockStone::VARIANT, BlockStone::EnumType::GRANITE), s.graniteSize);
    dioriteGen = std::make_unique<WorldGenMinable>(Blocks::stone->getDefaultState()->withProperty(BlockStone::VARIANT, BlockStone::EnumType::DIORITE), s.dioriteSize);
    andesiteGen = std::make_unique<WorldGenMinable>(Blocks::stone->getDefaultState()->withProperty(BlockStone::VARIANT, BlockStone::EnumType::ANDESITE), s.andesiteSize);
    coalGen = std::make_unique<WorldGenMinable>(Blocks::coal_ore->getDefaultState(), s.coalSize);
    ironGen = std::make_unique<WorldGenMinable>(Blocks::iron_ore->getDefaultState(), s.ironSize);
    goldGen = std::make_unique<WorldGenMinable>(Blocks::gold_ore->getDefaultState(), s.goldSize);
    redstoneGen = std::make_unique<WorldGenMinable>(Blocks::redstone_ore->getDefaultState(), s.redstoneSize);
    diamondGen = std::make_unique<WorldGenMinable>(Blocks::diamond_ore->getDefaultState(), s.diamondSize);
    lapisGen = std::make_unique<WorldGenMinable>(Blocks::lapis_ore->getDefaultState(), s.lapisSize);
    genDecorations(biome);
    currentWorld = nullptr;
    randomGenerator = nullptr;
}

void BiomeDecorator::genDecorations(BiomeGenBase* biomeGenBaseIn)
{
    Random& rand = *randomGenerator;
    World* world = currentWorld;
    const BlockPos& origin = field_180294_c;
    // rand.nextInt(16) + 8 twice, in order.
    const auto offset = [&rand](int& x, int& z) {
        x = rand.nextInt(16) + 8;
        z = rand.nextInt(16) + 8;
    };
    int x = 0;
    int z = 0;
    generateOres();
    for (int i = 0; i < sandPerChunk2; ++i) {
        offset(x, z);
        sandGen->generate(world, rand, world->getTopSolidOrLiquidBlock(origin.add(x, 0, z)));
    }
    for (int i1 = 0; i1 < clayPerChunk; ++i1) {
        offset(x, z);
        clayGen->generate(world, rand, world->getTopSolidOrLiquidBlock(origin.add(x, 0, z)));
    }
    for (int j1 = 0; j1 < sandPerChunk; ++j1) {
        offset(x, z);
        gravelAsSandGen->generate(world, rand, world->getTopSolidOrLiquidBlock(origin.add(x, 0, z)));
    }
    int k1 = treesPerChunk;
    if (rand.nextInt(10) == 0)
        ++k1;
    for (int j2 = 0; j2 < k1; ++j2) {
        offset(x, z);
        const std::shared_ptr<WorldGenAbstractTree> tree = biomeGenBaseIn->genBigTreeChance(rand);
        tree->func_175904_e();
        const BlockPos blockpos = world->getHeight(origin.add(x, 0, z));
        if (tree->generate(world, rand, blockpos))
            tree->func_180711_a(world, rand, blockpos);
    }
    for (int k2 = 0; k2 < bigMushroomsPerChunk; ++k2) {
        offset(x, z);
        bigMushroomGen->generate(world, rand, world->getHeight(origin.add(x, 0, z)));
    }
    for (int l2 = 0; l2 < flowersPerChunk; ++l2) {
        offset(x, z);
        const int j14 = world->getHeight(origin.add(x, 0, z)).getY() + 32;
        if (j14 > 0) {
            const int k17 = rand.nextInt(j14);
            const BlockPos blockpos1 = origin.add(x, k17, z);
            const BlockFlower::EnumFlowerType type = biomeGenBaseIn->pickRandomFlower(rand, blockpos1);
            BlockFlower* blockflower = BlockFlower::info(type).blockType == BlockFlower::EnumFlowerColor::YELLOW ? Blocks::yellow_flower : Blocks::red_flower;
            if (&blockflower->getMaterial() != &Material::air) {
                yellowFlowerGen->setGeneratedBlock(blockflower, type);
                yellowFlowerGen->generate(world, rand, blockpos1);
            }
        }
    }
    // The plant passes: a column up to twice the height (or height + 32) above a random spot.
    const auto randomInColumn = [&](int scale, int add, auto&& place) {
        offset(x, z);
        const int h = world->getHeight(origin.add(x, 0, z)).getY() * scale + add;
        if (h > 0) {
            const int y = rand.nextInt(h);
            place(origin.add(x, y, z));
        }
    };
    for (int i3 = 0; i3 < grassPerChunk; ++i3) {
        randomInColumn(2, 0, [&](const BlockPos& p) { biomeGenBaseIn->getRandomWorldGenForGrass(rand)->generate(world, rand, p); });
    }
    for (int j3 = 0; j3 < deadBushPerChunk; ++j3) {
        randomInColumn(2, 0, [&](const BlockPos& p) { WorldGenDeadBush().generate(world, rand, p); });
    }
    for (int k3 = 0; k3 < waterlilyPerChunk; ++k3) {
        randomInColumn(2, 0, [&](BlockPos blockpos4) {
            while (blockpos4.getY() > 0) {
                const BlockPos blockpos7 = blockpos4.down();
                if (!world->isAirBlock(blockpos7))
                    break;
                blockpos4 = blockpos7;
            }
            waterlilyGen->generate(world, rand, blockpos4);
        });
    }
    for (int l3 = 0; l3 < mushroomsPerChunk; ++l3) {
        if (rand.nextInt(4) == 0) {
            offset(x, z);
            mushroomBrownGen->generate(world, rand, world->getHeight(origin.add(x, 0, z)));
        }
        if (rand.nextInt(8) == 0)
            randomInColumn(2, 0, [&](const BlockPos& p) { mushroomRedGen->generate(world, rand, p); });
    }
    if (rand.nextInt(4) == 0)
        randomInColumn(2, 0, [&](const BlockPos& p) { mushroomBrownGen->generate(world, rand, p); });
    if (rand.nextInt(8) == 0)
        randomInColumn(2, 0, [&](const BlockPos& p) { mushroomRedGen->generate(world, rand, p); });
    for (int k4 = 0; k4 < reedsPerChunk; ++k4)
        randomInColumn(2, 0, [&](const BlockPos& p) { reedGen->generate(world, rand, p); });
    for (int l4 = 0; l4 < 10; ++l4)
        randomInColumn(2, 0, [&](const BlockPos& p) { reedGen->generate(world, rand, p); });
    if (rand.nextInt(32) == 0)
        randomInColumn(2, 0, [&](const BlockPos& p) { WorldGenPumpkin().generate(world, rand, p); });
    for (int j5 = 0; j5 < cactiPerChunk; ++j5)
        randomInColumn(2, 0, [&](const BlockPos& p) { cactusGen->generate(world, rand, p); });
    if (generateLakes) {
        for (int k5 = 0; k5 < 50; ++k5) {
            offset(x, z);
            const int i17 = rand.nextInt(248) + 8;
            if (i17 > 0) {
                const int k19 = rand.nextInt(i17);
                WorldGenLiquids(Blocks::flowing_water).generate(world, rand, origin.add(x, k19, z));
            }
        }
        for (int l5 = 0; l5 < 20; ++l5) {
            offset(x, z);
            const int a = rand.nextInt(240) + 8;
            const int b = rand.nextInt(a) + 8;
            const int j17 = rand.nextInt(b);
            WorldGenLiquids(Blocks::flowing_lava).generate(world, rand, origin.add(x, j17, z));
        }
    }
}

void BiomeDecorator::genStandardOre1(int blockCount, WorldGenerator& generator, int minHeight, int maxHeight)
{
    if (maxHeight < minHeight) {
        const int i = minHeight;
        minHeight = maxHeight;
        maxHeight = i;
    } else if (maxHeight == minHeight) {
        if (minHeight < 255)
            ++maxHeight;
        else
            --minHeight;
    }
    for (int j = 0; j < blockCount; ++j) {
        const int dx = randomGenerator->nextInt(16);
        const int dy = randomGenerator->nextInt(maxHeight - minHeight) + minHeight;
        const int dz = randomGenerator->nextInt(16);
        generator.generate(currentWorld, *randomGenerator, field_180294_c.add(dx, dy, dz));
    }
}

void BiomeDecorator::genStandardOre2(int blockCount, WorldGenerator& generator, int centerHeight, int spread)
{
    for (int i = 0; i < blockCount; ++i) {
        const int dx = randomGenerator->nextInt(16);
        const int a = randomGenerator->nextInt(spread);
        const int dy = a + randomGenerator->nextInt(spread) + centerHeight - spread;
        const int dz = randomGenerator->nextInt(16);
        generator.generate(currentWorld, *randomGenerator, field_180294_c.add(dx, dy, dz));
    }
}

void BiomeDecorator::generateOres()
{
    const ChunkProviderSettings::Factory& s = *chunkProviderSettings;
    genStandardOre1(s.dirtCount, *dirtGen, s.dirtMinHeight, s.dirtMaxHeight);
    genStandardOre1(s.gravelCount, *gravelGen, s.gravelMinHeight, s.gravelMaxHeight);
    genStandardOre1(s.dioriteCount, *dioriteGen, s.dioriteMinHeight, s.dioriteMaxHeight);
    genStandardOre1(s.graniteCount, *graniteGen, s.graniteMinHeight, s.graniteMaxHeight);
    genStandardOre1(s.andesiteCount, *andesiteGen, s.andesiteMinHeight, s.andesiteMaxHeight);
    genStandardOre1(s.coalCount, *coalGen, s.coalMinHeight, s.coalMaxHeight);
    genStandardOre1(s.ironCount, *ironGen, s.ironMinHeight, s.ironMaxHeight);
    genStandardOre1(s.goldCount, *goldGen, s.goldMinHeight, s.goldMaxHeight);
    genStandardOre1(s.redstoneCount, *redstoneGen, s.redstoneMinHeight, s.redstoneMaxHeight);
    genStandardOre1(s.diamondCount, *diamondGen, s.diamondMinHeight, s.diamondMaxHeight);
    genStandardOre2(s.lapisCount, *lapisGen, s.lapisCenterHeight, s.lapisSpread);
}

// --- BiomeEndDecorator ---

BiomeEndDecorator::BiomeEndDecorator()
    : spikeGen(std::make_unique<WorldGenSpikes>(Blocks::end_stone))
{
}

void BiomeEndDecorator::genDecorations(BiomeGenBase*)
{
    generateOres();
    if (randomGenerator->nextInt(5) == 0) {
        const int i = randomGenerator->nextInt(16) + 8;
        const int j = randomGenerator->nextInt(16) + 8;
        spikeGen->generate(currentWorld, *randomGenerator, currentWorld->getTopSolidOrLiquidBlock(field_180294_c.add(i, 0, j)));
    }
    if (field_180294_c.getX() == 0 && field_180294_c.getZ() == 0) {
        // Java: spawns the EntityDragon at (0, 128, 0) with a random yaw -- entities come with stage 10; the draw
        // keeps the generator in step.
        randomGenerator->nextFloat();
    }
}

} // namespace mc

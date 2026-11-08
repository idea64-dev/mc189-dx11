#include "world/biome/Biomes.h"

#include "block/AllBlocks.h"
#include "core/Java.h"
#include "core/JavaMath.h"
#include "init/Blocks.h"
#include "util/MathHelper.h"
#include "util/Random.h"
#include "world/World.h"
#include "world/biome/BiomeDecorator.h"
#include "world/chunk/Chunk.h"
#include "world/gen/NoiseGenerators.h"
#include "world/gen/feature/WorldGenerators.h"

#include <cmath>

namespace mc {

namespace {

// rand.nextInt(16) + 8 for x and z, then a y below height + 32: the double plant placement.
void placeDoublePlants(WorldGenDoublePlant& generator, World* worldIn, Random& rand, const BlockPos& pos, int count)
{
    for (int i = 0; i < count; ++i) {
        const int j = rand.nextInt(16) + 8;
        const int k = rand.nextInt(16) + 8;
        const int l = rand.nextInt(worldIn->getHeight(pos.add(j, 0, k)).getY() + 32);
        generator.generate(worldIn, rand, pos.add(j, l, k));
    }
}

const IBlockState* oldLog(BlockPlanks::EnumType type)
{
    return Blocks::log->getDefaultState()->withProperty(BlockOldLog::VARIANT, type);
}

const IBlockState* oldLeaves(BlockPlanks::EnumType type)
{
    return Blocks::leaves->getDefaultState()->withProperty(BlockOldLeaf::VARIANT, type)->withProperty(BlockLeaves::CHECK_DECAY, false);
}

const IBlockState* coloredClay(EnumDyeColor color)
{
    return Blocks::stained_hardened_clay->getDefaultState()->withProperty(BlockColored::COLOR, color);
}

} // namespace

// --- simple ones ---

BiomeGenOcean::BiomeGenOcean(int id)
    : BiomeGenBase(id)
{
    spawnableCreatureList.clear();
}

BiomeGenRiver::BiomeGenRiver(int id)
    : BiomeGenBase(id)
{
    spawnableCreatureList.clear();
}

BiomeGenHell::BiomeGenHell(int id)
    : BiomeGenBase(id)
{
    spawnableMonsterList.clear();
    spawnableCreatureList.clear();
    spawnableWaterCreatureList.clear();
    spawnableCaveCreatureList.clear();
    spawnableMonsterList = { { "Ghast", 50, 4, 4 }, { "PigZombie", 100, 4, 4 }, { "LavaSlime", 1, 4, 4 } };
}

BiomeGenEnd::BiomeGenEnd(int id)
    : BiomeGenBase(id)
{
    spawnableMonsterList.clear();
    spawnableCreatureList.clear();
    spawnableWaterCreatureList.clear();
    spawnableCaveCreatureList.clear();
    spawnableMonsterList = { { "Enderman", 10, 4, 4 } };
    topBlock = Blocks::dirt->getDefaultState();
    fillerBlock = Blocks::dirt->getDefaultState();
    theBiomeDecorator = std::make_unique<BiomeEndDecorator>();
}

BiomeGenMushroomIsland::BiomeGenMushroomIsland(int id)
    : BiomeGenBase(id)
{
    theBiomeDecorator->treesPerChunk = -100;
    theBiomeDecorator->flowersPerChunk = -100;
    theBiomeDecorator->grassPerChunk = -100;
    theBiomeDecorator->mushroomsPerChunk = 1;
    theBiomeDecorator->bigMushroomsPerChunk = 1;
    topBlock = Blocks::mycelium->getDefaultState();
    spawnableMonsterList.clear();
    spawnableCreatureList.clear();
    spawnableWaterCreatureList.clear();
    spawnableCreatureList = { { "MushroomCow", 8, 4, 8 } };
}

BiomeGenBeach::BiomeGenBeach(int id)
    : BiomeGenBase(id)
{
    spawnableCreatureList.clear();
    topBlock = Blocks::sand->getDefaultState();
    fillerBlock = Blocks::sand->getDefaultState();
    theBiomeDecorator->treesPerChunk = -999;
    theBiomeDecorator->deadBushPerChunk = 0;
    theBiomeDecorator->reedsPerChunk = 0;
    theBiomeDecorator->cactiPerChunk = 0;
}

BiomeGenStoneBeach::BiomeGenStoneBeach(int id)
    : BiomeGenBase(id)
{
    spawnableCreatureList.clear();
    topBlock = Blocks::stone->getDefaultState();
    fillerBlock = Blocks::stone->getDefaultState();
    theBiomeDecorator->treesPerChunk = -999;
    theBiomeDecorator->deadBushPerChunk = 0;
    theBiomeDecorator->reedsPerChunk = 0;
    theBiomeDecorator->cactiPerChunk = 0;
}

// --- BiomeGenPlains ---

BiomeGenPlains::BiomeGenPlains(int id)
    : BiomeGenBase(id)
{
    setTemperatureRainfall(0.8f, 0.4f);
    setHeight(height_LowPlains);
    spawnableCreatureList.push_back({ "EntityHorse", 5, 2, 6 });
    theBiomeDecorator->treesPerChunk = -999;
    theBiomeDecorator->flowersPerChunk = 4;
    theBiomeDecorator->grassPerChunk = 10;
}

BlockFlower::EnumFlowerType BiomeGenPlains::pickRandomFlower(Random& rand, const BlockPos& pos)
{
    using Type = BlockFlower::EnumFlowerType;
    const double d0 = GRASS_COLOR_NOISE().getValue(static_cast<double>(pos.getX()) / 200.0, static_cast<double>(pos.getZ()) / 200.0);
    if (d0 < -0.8) {
        switch (rand.nextInt(4)) {
        case 0: return Type::ORANGE_TULIP;
        case 1: return Type::RED_TULIP;
        case 2: return Type::PINK_TULIP;
        default: return Type::WHITE_TULIP;
        }
    }
    if (rand.nextInt(3) > 0) {
        const int i = rand.nextInt(3);
        return i == 0 ? Type::POPPY : (i == 1 ? Type::HOUSTONIA : Type::OXEYE_DAISY);
    }
    return Type::DANDELION;
}

void BiomeGenPlains::decorate(World* worldIn, Random& rand, const BlockPos& pos)
{
    const double d0 = GRASS_COLOR_NOISE().getValue(static_cast<double>(pos.getX() + 8) / 200.0, static_cast<double>(pos.getZ() + 8) / 200.0);
    if (d0 < -0.8) {
        theBiomeDecorator->flowersPerChunk = 15;
        theBiomeDecorator->grassPerChunk = 5;
    } else {
        theBiomeDecorator->flowersPerChunk = 4;
        theBiomeDecorator->grassPerChunk = 10;
        DOUBLE_PLANT_GENERATOR().setPlantType(BlockDoublePlant::EnumPlantType::GRASS);
        placeDoublePlants(DOUBLE_PLANT_GENERATOR(), worldIn, rand, pos, 7);
    }
    if (field_150628_aC) {
        DOUBLE_PLANT_GENERATOR().setPlantType(BlockDoublePlant::EnumPlantType::SUNFLOWER);
        placeDoublePlants(DOUBLE_PLANT_GENERATOR(), worldIn, rand, pos, 10);
    }
    BiomeGenBase::decorate(worldIn, rand, pos);
}

BiomeGenBase* BiomeGenPlains::createMutatedBiome(int id)
{
    BiomeGenPlains* biomegenplains = create<BiomeGenPlains>(id);
    biomegenplains->setBiomeName("Sunflower Plains");
    biomegenplains->field_150628_aC = true;
    biomegenplains->setColor(9286496);
    biomegenplains->field_150609_ah = 14273354;
    return biomegenplains;
}

// --- BiomeGenDesert ---

BiomeGenDesert::BiomeGenDesert(int id)
    : BiomeGenBase(id)
{
    spawnableCreatureList.clear();
    topBlock = Blocks::sand->getDefaultState();
    fillerBlock = Blocks::sand->getDefaultState();
    theBiomeDecorator->treesPerChunk = -999;
    theBiomeDecorator->deadBushPerChunk = 2;
    theBiomeDecorator->reedsPerChunk = 50;
    theBiomeDecorator->cactiPerChunk = 10;
}

void BiomeGenDesert::decorate(World* worldIn, Random& rand, const BlockPos& pos)
{
    BiomeGenBase::decorate(worldIn, rand, pos);
    if (rand.nextInt(1000) == 0) {
        const int i = rand.nextInt(16) + 8;
        const int j = rand.nextInt(16) + 8;
        const BlockPos blockpos = worldIn->getHeight(pos.add(i, 0, j)).up();
        WorldGenDesertWells().generate(worldIn, rand, blockpos);
    }
}

// --- BiomeGenHills ---

BiomeGenHills::BiomeGenHills(int id, bool extraTrees)
    : BiomeGenBase(id)
    , theWorldGenerator(std::make_unique<WorldGenMinable>(
          Blocks::monster_egg->getDefaultState()->withProperty(BlockSilverfish::VARIANT, BlockSilverfish::EnumType::STONE), 9))
    , field_150634_aD(std::make_shared<WorldGenTaiga2>(false))
{
    field_150638_aH = field_150635_aE;
    if (extraTrees) {
        theBiomeDecorator->treesPerChunk = 3;
        field_150638_aH = field_150636_aF;
    }
}

BiomeGenHills::~BiomeGenHills() = default;

std::shared_ptr<WorldGenAbstractTree> BiomeGenHills::genBigTreeChance(Random& rand)
{
    if (rand.nextInt(3) > 0)
        return field_150634_aD;
    return BiomeGenBase::genBigTreeChance(rand);
}

void BiomeGenHills::decorate(World* worldIn, Random& rand, const BlockPos& pos)
{
    BiomeGenBase::decorate(worldIn, rand, pos);
    int i = 3 + rand.nextInt(6);
    for (int j = 0; j < i; ++j) {
        const int k = rand.nextInt(16);
        const int l = rand.nextInt(28) + 4;
        const int i1 = rand.nextInt(16);
        const BlockPos blockpos = pos.add(k, l, i1);
        if (worldIn->getBlockState(blockpos)->getBlock() == Blocks::stone)
            worldIn->setBlockState(blockpos, Blocks::emerald_ore->getDefaultState(), 2);
    }
    for (i = 0; i < 7; ++i) {
        const int j1 = rand.nextInt(16);
        const int k1 = rand.nextInt(64);
        const int l1 = rand.nextInt(16);
        theWorldGenerator->generate(worldIn, rand, pos.add(j1, k1, l1));
    }
}

void BiomeGenHills::genTerrainBlocks(World* worldIn, Random& rand, ChunkPrimer& chunkPrimerIn, int x, int z, double noiseVal)
{
    topBlock = Blocks::grass->getDefaultState();
    fillerBlock = Blocks::dirt->getDefaultState();
    if ((noiseVal < -1.0 || noiseVal > 2.0) && field_150638_aH == field_150637_aG) {
        topBlock = Blocks::gravel->getDefaultState();
        fillerBlock = Blocks::gravel->getDefaultState();
    } else if (noiseVal > 1.0 && field_150638_aH != field_150636_aF) {
        topBlock = Blocks::stone->getDefaultState();
        fillerBlock = Blocks::stone->getDefaultState();
    }
    generateBiomeTerrain(worldIn, rand, chunkPrimerIn, x, z, noiseVal);
}

BiomeGenHills* BiomeGenHills::mutateHills(BiomeGenBase* base)
{
    field_150638_aH = field_150637_aG;
    func_150557_a(base->color, true);
    setBiomeName(base->biomeName + " M");
    setHeight(Height { base->minHeight, base->maxHeight });
    setTemperatureRainfall(base->temperature, base->rainfall);
    return this;
}

BiomeGenBase* BiomeGenHills::createMutatedBiome(int id)
{
    return create<BiomeGenHills>(id, false)->mutateHills(this);
}

// --- BiomeGenForest ---

std::shared_ptr<WorldGenForest> BiomeGenForest::field_150629_aC()
{
    static const auto generator = std::make_shared<WorldGenForest>(false, true);
    return generator;
}

std::shared_ptr<WorldGenForest> BiomeGenForest::field_150630_aD()
{
    static const auto generator = std::make_shared<WorldGenForest>(false, false);
    return generator;
}

std::shared_ptr<WorldGenCanopyTree> BiomeGenForest::field_150631_aE()
{
    static const auto generator = std::make_shared<WorldGenCanopyTree>(false);
    return generator;
}

BiomeGenForest::BiomeGenForest(int id, int type)
    : BiomeGenBase(id)
    , field_150632_aF(type)
{
    theBiomeDecorator->treesPerChunk = 10;
    theBiomeDecorator->grassPerChunk = 2;
    if (field_150632_aF == 1) {
        theBiomeDecorator->treesPerChunk = 6;
        theBiomeDecorator->flowersPerChunk = 100;
        theBiomeDecorator->grassPerChunk = 1;
    }
    setFillerBlockMetadata(5159473);
    setTemperatureRainfall(0.7f, 0.8f);
    if (field_150632_aF == 2) {
        field_150609_ah = 353825;
        color = 3175492;
        setTemperatureRainfall(0.6f, 0.6f);
    }
    if (field_150632_aF == 0)
        spawnableCreatureList.push_back({ "Wolf", 5, 4, 4 });
    if (field_150632_aF == 3)
        theBiomeDecorator->treesPerChunk = -999;
}

BiomeGenBase* BiomeGenForest::func_150557_a(int colorIn, bool darken)
{
    if (field_150632_aF == 2) {
        field_150609_ah = 353825;
        color = colorIn;
        if (darken)
            field_150609_ah = (field_150609_ah & 16711422) >> 1;
        return this;
    }
    return BiomeGenBase::func_150557_a(colorIn, darken);
}

std::shared_ptr<WorldGenAbstractTree> BiomeGenForest::genBigTreeChance(Random& rand)
{
    if (field_150632_aF == 3 && rand.nextInt(3) > 0)
        return field_150631_aE();
    if (field_150632_aF != 2 && rand.nextInt(5) != 0)
        return worldGeneratorTrees;
    return field_150630_aD();
}

BlockFlower::EnumFlowerType BiomeGenForest::pickRandomFlower(Random& rand, const BlockPos& pos)
{
    if (field_150632_aF == 1) {
        const double d0 = MathHelper::clamp_double(
            (1.0 + GRASS_COLOR_NOISE().getValue(static_cast<double>(pos.getX()) / 48.0, static_cast<double>(pos.getZ()) / 48.0)) / 2.0, 0.0, 0.9999);
        const auto type = static_cast<BlockFlower::EnumFlowerType>(java::d2i(d0 * 10.0));
        return type == BlockFlower::EnumFlowerType::BLUE_ORCHID ? BlockFlower::EnumFlowerType::POPPY : type;
    }
    return BiomeGenBase::pickRandomFlower(rand, pos);
}

void BiomeGenForest::decorate(World* worldIn, Random& rand, const BlockPos& pos)
{
    if (field_150632_aF == 3) {
        for (int i = 0; i < 4; ++i) {
            for (int j = 0; j < 4; ++j) {
                const int k = i * 4 + 1 + 8 + rand.nextInt(3);
                const int l = j * 4 + 1 + 8 + rand.nextInt(3);
                const BlockPos blockpos = worldIn->getHeight(pos.add(k, 0, l));
                if (rand.nextInt(20) == 0) {
                    WorldGenBigMushroom().generate(worldIn, rand, blockpos);
                } else {
                    const std::shared_ptr<WorldGenAbstractTree> tree = genBigTreeChance(rand);
                    tree->func_175904_e();
                    if (tree->generate(worldIn, rand, blockpos))
                        tree->func_180711_a(worldIn, rand, blockpos);
                }
            }
        }
    }
    int j1 = rand.nextInt(5) - 3;
    if (field_150632_aF == 1)
        j1 += 2;
    for (int k1 = 0; k1 < j1; ++k1) {
        const int l1 = rand.nextInt(3);
        if (l1 == 0)
            DOUBLE_PLANT_GENERATOR().setPlantType(BlockDoublePlant::EnumPlantType::SYRINGA);
        else if (l1 == 1)
            DOUBLE_PLANT_GENERATOR().setPlantType(BlockDoublePlant::EnumPlantType::ROSE);
        else if (l1 == 2)
            DOUBLE_PLANT_GENERATOR().setPlantType(BlockDoublePlant::EnumPlantType::PAEONIA);
        for (int i2 = 0; i2 < 5; ++i2) {
            const int j2 = rand.nextInt(16) + 8;
            const int k2 = rand.nextInt(16) + 8;
            const int i1 = rand.nextInt(worldIn->getHeight(pos.add(j2, 0, k2)).getY() + 32);
            if (DOUBLE_PLANT_GENERATOR().generate(worldIn, rand, BlockPos(pos.getX() + j2, i1, pos.getZ() + k2)))
                break;
        }
    }
    BiomeGenBase::decorate(worldIn, rand, pos);
}

int BiomeGenForest::getGrassColorAtPos(const BlockPos& pos)
{
    const int i = BiomeGenBase::getGrassColorAtPos(pos);
    return field_150632_aF == 3 ? (i & 16711422) + 2634762 >> 1 : i;
}

namespace {

// The two anonymous BiomeGenMutated subclasses of BiomeGenForest.createMutatedBiome.
class BiomeGenForestMutated : public BiomeGenMutated {
public:
    using BiomeGenMutated::BiomeGenMutated;
    void decorate(World* worldIn, Random& rand, const BlockPos& pos) override { baseBiome->decorate(worldIn, rand, pos); }
};

class BiomeGenBirchForestMutated : public BiomeGenMutated {
public:
    using BiomeGenMutated::BiomeGenMutated;
    std::shared_ptr<WorldGenAbstractTree> genBigTreeChance(Random& rand) override
    {
        if (rand.nextBoolean())
            return BiomeGenForest::field_150629_aC();
        return BiomeGenForest::field_150630_aD();
    }
};

} // namespace

BiomeGenBase* BiomeGenForest::createMutatedBiome(int id)
{
    if (biomeID == BiomeGenBase::forest->biomeID) {
        BiomeGenForest* biomegenforest = create<BiomeGenForest>(id, 1);
        biomegenforest->setHeight(Height { minHeight, maxHeight + 0.2f });
        biomegenforest->setBiomeName("Flower Forest");
        biomegenforest->func_150557_a(6976549, true);
        biomegenforest->setFillerBlockMetadata(8233509);
        return biomegenforest;
    }
    if (biomeID != BiomeGenBase::birchForest->biomeID && biomeID != BiomeGenBase::birchForestHills->biomeID)
        return create<BiomeGenForestMutated>(id, this);
    return create<BiomeGenBirchForestMutated>(id, this);
}

// --- BiomeGenTaiga ---

namespace {

std::shared_ptr<WorldGenTaiga1>& taigaTree1()
{
    static auto generator = std::make_shared<WorldGenTaiga1>();
    return generator;
}

std::shared_ptr<WorldGenTaiga2>& taigaTree2()
{
    static auto generator = std::make_shared<WorldGenTaiga2>(false);
    return generator;
}

std::shared_ptr<WorldGenMegaPineTree>& megaPine()
{
    static auto generator = std::make_shared<WorldGenMegaPineTree>(false, false);
    return generator;
}

std::shared_ptr<WorldGenMegaPineTree>& megaSpruce()
{
    static auto generator = std::make_shared<WorldGenMegaPineTree>(false, true);
    return generator;
}

WorldGenBlockBlob& mossyBlob()
{
    static WorldGenBlockBlob generator(Blocks::mossy_cobblestone, 0);
    return generator;
}

} // namespace

BiomeGenTaiga::BiomeGenTaiga(int id, int type)
    : BiomeGenBase(id)
    , field_150644_aH(type)
{
    spawnableCreatureList.push_back({ "Wolf", 8, 4, 4 });
    theBiomeDecorator->treesPerChunk = 10;
    if (type != 1 && type != 2) {
        theBiomeDecorator->grassPerChunk = 1;
        theBiomeDecorator->mushroomsPerChunk = 1;
    } else {
        theBiomeDecorator->grassPerChunk = 7;
        theBiomeDecorator->deadBushPerChunk = 1;
        theBiomeDecorator->mushroomsPerChunk = 3;
    }
}

std::shared_ptr<WorldGenAbstractTree> BiomeGenTaiga::genBigTreeChance(Random& rand)
{
    if ((field_150644_aH == 1 || field_150644_aH == 2) && rand.nextInt(3) == 0) {
        if (field_150644_aH != 2 && rand.nextInt(13) != 0)
            return megaPine();
        return megaSpruce();
    }
    if (rand.nextInt(3) == 0)
        return taigaTree1();
    return taigaTree2();
}

std::shared_ptr<WorldGenerator> BiomeGenTaiga::getRandomWorldGenForGrass(Random& rand)
{
    return std::make_shared<WorldGenTallGrass>(rand.nextInt(5) > 0 ? BlockTallGrass::EnumType::FERN : BlockTallGrass::EnumType::GRASS);
}

void BiomeGenTaiga::decorate(World* worldIn, Random& rand, const BlockPos& pos)
{
    if (field_150644_aH == 1 || field_150644_aH == 2) {
        const int i = rand.nextInt(3);
        for (int j = 0; j < i; ++j) {
            const int k = rand.nextInt(16) + 8;
            const int l = rand.nextInt(16) + 8;
            mossyBlob().generate(worldIn, rand, worldIn->getHeight(pos.add(k, 0, l)));
        }
    }
    DOUBLE_PLANT_GENERATOR().setPlantType(BlockDoublePlant::EnumPlantType::FERN);
    placeDoublePlants(DOUBLE_PLANT_GENERATOR(), worldIn, rand, pos, 7);
    BiomeGenBase::decorate(worldIn, rand, pos);
}

void BiomeGenTaiga::genTerrainBlocks(World* worldIn, Random& rand, ChunkPrimer& chunkPrimerIn, int x, int z, double noiseVal)
{
    if (field_150644_aH == 1 || field_150644_aH == 2) {
        topBlock = Blocks::grass->getDefaultState();
        fillerBlock = Blocks::dirt->getDefaultState();
        if (noiseVal > 1.75)
            topBlock = Blocks::dirt->getDefaultState()->withProperty(BlockDirt::VARIANT, BlockDirt::DirtType::COARSE_DIRT);
        else if (noiseVal > -0.95)
            topBlock = Blocks::dirt->getDefaultState()->withProperty(BlockDirt::VARIANT, BlockDirt::DirtType::PODZOL);
    }
    generateBiomeTerrain(worldIn, rand, chunkPrimerIn, x, z, noiseVal);
}

BiomeGenBase* BiomeGenTaiga::createMutatedBiome(int id)
{
    if (biomeID == BiomeGenBase::megaTaiga->biomeID) {
        return create<BiomeGenTaiga>(id, 2)
            ->func_150557_a(5858897, true)
            ->setBiomeName("Mega Spruce Taiga")
            ->setFillerBlockMetadata(5159473)
            ->setTemperatureRainfall(0.25f, 0.8f)
            ->setHeight(Height { minHeight, maxHeight });
    }
    return BiomeGenBase::createMutatedBiome(id);
}

// --- BiomeGenSwamp ---

BiomeGenSwamp::BiomeGenSwamp(int id)
    : BiomeGenBase(id)
{
    theBiomeDecorator->treesPerChunk = 2;
    theBiomeDecorator->flowersPerChunk = 1;
    theBiomeDecorator->deadBushPerChunk = 1;
    theBiomeDecorator->mushroomsPerChunk = 8;
    theBiomeDecorator->reedsPerChunk = 10;
    theBiomeDecorator->clayPerChunk = 1;
    theBiomeDecorator->waterlilyPerChunk = 4;
    theBiomeDecorator->sandPerChunk2 = 0;
    theBiomeDecorator->sandPerChunk = 0;
    theBiomeDecorator->grassPerChunk = 5;
    waterColorMultiplier = 14745518;
    spawnableMonsterList.push_back({ "Slime", 1, 1, 1 });
}

std::shared_ptr<WorldGenAbstractTree> BiomeGenSwamp::genBigTreeChance(Random&)
{
    return worldGeneratorSwamp;
}

int BiomeGenSwamp::getGrassColorAtPos(const BlockPos& pos)
{
    const double d0 = GRASS_COLOR_NOISE().getValue(static_cast<double>(pos.getX()) * 0.0225, static_cast<double>(pos.getZ()) * 0.0225);
    return d0 < -0.1 ? 5011004 : 6975545;
}

void BiomeGenSwamp::genTerrainBlocks(World* worldIn, Random& rand, ChunkPrimer& chunkPrimerIn, int x, int z, double noiseVal)
{
    const double d0 = GRASS_COLOR_NOISE().getValue(static_cast<double>(x) * 0.25, static_cast<double>(z) * 0.25);
    if (d0 > 0.0) {
        const int i = x & 15;
        const int j = z & 15;
        for (int k = 255; k >= 0; --k) {
            if (&chunkPrimerIn.getBlockState(j, k, i)->getBlock()->getMaterial() != &Material::air) {
                if (k == 62 && chunkPrimerIn.getBlockState(j, k, i)->getBlock() != Blocks::water) {
                    chunkPrimerIn.setBlockState(j, k, i, Blocks::water->getDefaultState());
                    if (d0 < 0.12)
                        chunkPrimerIn.setBlockState(j, k + 1, i, Blocks::waterlily->getDefaultState());
                }
                break;
            }
        }
    }
    generateBiomeTerrain(worldIn, rand, chunkPrimerIn, x, z, noiseVal);
}

// --- BiomeGenSnow ---

BiomeGenSnow::BiomeGenSnow(int id, bool spikes)
    : BiomeGenBase(id)
    , field_150615_aC(spikes)
    , field_150616_aD(std::make_unique<WorldGenIceSpike>())
    , field_150617_aE(std::make_unique<WorldGenIcePath>(4))
{
    if (spikes)
        topBlock = Blocks::snow->getDefaultState();
    spawnableCreatureList.clear();
}

BiomeGenSnow::~BiomeGenSnow() = default;

void BiomeGenSnow::decorate(World* worldIn, Random& rand, const BlockPos& pos)
{
    if (field_150615_aC) {
        for (int i = 0; i < 3; ++i) {
            const int j = rand.nextInt(16) + 8;
            const int k = rand.nextInt(16) + 8;
            field_150616_aD->generate(worldIn, rand, worldIn->getHeight(pos.add(j, 0, k)));
        }
        for (int l = 0; l < 2; ++l) {
            const int i1 = rand.nextInt(16) + 8;
            const int j1 = rand.nextInt(16) + 8;
            field_150617_aE->generate(worldIn, rand, worldIn->getHeight(pos.add(i1, 0, j1)));
        }
    }
    BiomeGenBase::decorate(worldIn, rand, pos);
}

std::shared_ptr<WorldGenAbstractTree> BiomeGenSnow::genBigTreeChance(Random&)
{
    return std::make_shared<WorldGenTaiga2>(false);
}

BiomeGenBase* BiomeGenSnow::createMutatedBiome(int id)
{
    BiomeGenBase* biomegenbase = create<BiomeGenSnow>(id, true)
                                     ->func_150557_a(13828095, true)
                                     ->setBiomeName(biomeName + " Spikes")
                                     ->setEnableSnow()
                                     ->setTemperatureRainfall(0.0f, 0.5f)
                                     ->setHeight(Height { minHeight + 0.1f, maxHeight + 0.1f });
    biomegenbase->minHeight = minHeight + 0.3f;
    biomegenbase->maxHeight = maxHeight + 0.4f;
    return biomegenbase;
}

// --- BiomeGenJungle ---

BiomeGenJungle::BiomeGenJungle(int id, bool edge)
    : BiomeGenBase(id)
    , field_150614_aC(edge)
{
    theBiomeDecorator->treesPerChunk = edge ? 2 : 50;
    theBiomeDecorator->grassPerChunk = 25;
    theBiomeDecorator->flowersPerChunk = 4;
    if (!edge)
        spawnableMonsterList.push_back({ "Ozelot", 2, 1, 1 });
    spawnableCreatureList.push_back({ "Chicken", 10, 4, 4 });
}

std::shared_ptr<WorldGenAbstractTree> BiomeGenJungle::genBigTreeChance(Random& rand)
{
    const IBlockState* const jungleLog = oldLog(BlockPlanks::EnumType::JUNGLE);
    const IBlockState* const jungleLeaves = oldLeaves(BlockPlanks::EnumType::JUNGLE);
    if (rand.nextInt(10) == 0)
        return worldGeneratorBigTree;
    if (rand.nextInt(2) == 0)
        return std::make_shared<WorldGenShrub>(jungleLog, oldLeaves(BlockPlanks::EnumType::OAK));
    if (!field_150614_aC && rand.nextInt(3) == 0)
        return std::make_shared<WorldGenMegaJungle>(false, 10, 20, jungleLog, jungleLeaves);
    return std::make_shared<WorldGenTrees>(false, 4 + rand.nextInt(7), jungleLog, jungleLeaves, true);
}

std::shared_ptr<WorldGenerator> BiomeGenJungle::getRandomWorldGenForGrass(Random& rand)
{
    return std::make_shared<WorldGenTallGrass>(rand.nextInt(4) == 0 ? BlockTallGrass::EnumType::FERN : BlockTallGrass::EnumType::GRASS);
}

void BiomeGenJungle::decorate(World* worldIn, Random& rand, const BlockPos& pos)
{
    BiomeGenBase::decorate(worldIn, rand, pos);
    const int i = rand.nextInt(16) + 8;
    int j = rand.nextInt(16) + 8;
    int k = rand.nextInt(worldIn->getHeight(pos.add(i, 0, j)).getY() * 2);
    WorldGenMelon().generate(worldIn, rand, pos.add(i, k, j));
    WorldGenVines worldgenvines;
    for (j = 0; j < 50; ++j) {
        k = rand.nextInt(16) + 8;
        const int i1 = rand.nextInt(16) + 8;
        worldgenvines.generate(worldIn, rand, pos.add(k, 128, i1));
    }
}

// --- BiomeGenSavanna ---

namespace {

std::shared_ptr<WorldGenSavannaTree>& savannaTree()
{
    static auto generator = std::make_shared<WorldGenSavannaTree>(false);
    return generator;
}

class BiomeGenSavannaMutated : public BiomeGenMutated {
public:
    BiomeGenSavannaMutated(int id, BiomeGenBase* base)
        : BiomeGenMutated(id, base)
    {
        theBiomeDecorator->treesPerChunk = 2;
        theBiomeDecorator->flowersPerChunk = 2;
        theBiomeDecorator->grassPerChunk = 5;
    }

    void genTerrainBlocks(World* worldIn, Random& rand, ChunkPrimer& chunkPrimerIn, int x, int z, double noiseVal) override
    {
        topBlock = Blocks::grass->getDefaultState();
        fillerBlock = Blocks::dirt->getDefaultState();
        if (noiseVal > 1.75) {
            topBlock = Blocks::stone->getDefaultState();
            fillerBlock = Blocks::stone->getDefaultState();
        } else if (noiseVal > -0.5) {
            topBlock = Blocks::dirt->getDefaultState()->withProperty(BlockDirt::VARIANT, BlockDirt::DirtType::COARSE_DIRT);
        }
        generateBiomeTerrain(worldIn, rand, chunkPrimerIn, x, z, noiseVal);
    }

    void decorate(World* worldIn, Random& rand, const BlockPos& pos) override { theBiomeDecorator->decorate(worldIn, rand, this, pos); }
};

} // namespace

BiomeGenSavanna::BiomeGenSavanna(int id)
    : BiomeGenBase(id)
{
    spawnableCreatureList.push_back({ "EntityHorse", 1, 2, 6 });
    theBiomeDecorator->treesPerChunk = 1;
    theBiomeDecorator->flowersPerChunk = 4;
    theBiomeDecorator->grassPerChunk = 20;
}

std::shared_ptr<WorldGenAbstractTree> BiomeGenSavanna::genBigTreeChance(Random& rand)
{
    if (rand.nextInt(5) > 0)
        return savannaTree();
    return worldGeneratorTrees;
}

BiomeGenBase* BiomeGenSavanna::createMutatedBiome(int id)
{
    BiomeGenBase* biomegenbase = create<BiomeGenSavannaMutated>(id, this);
    biomegenbase->temperature = (temperature + 1.0f) * 0.5f;
    biomegenbase->minHeight = minHeight * 0.5f + 0.3f;
    biomegenbase->maxHeight = maxHeight * 0.5f + 1.2f;
    return biomegenbase;
}

void BiomeGenSavanna::decorate(World* worldIn, Random& rand, const BlockPos& pos)
{
    DOUBLE_PLANT_GENERATOR().setPlantType(BlockDoublePlant::EnumPlantType::GRASS);
    placeDoublePlants(DOUBLE_PLANT_GENERATOR(), worldIn, rand, pos, 7);
    BiomeGenBase::decorate(worldIn, rand, pos);
}

// --- BiomeGenMesa ---

BiomeGenMesa::BiomeGenMesa(int id, bool bryce, bool forest)
    : BiomeGenBase(id)
    , field_150626_aH(bryce)
    , field_150620_aI(forest)
{
    setDisableRain();
    setTemperatureRainfall(2.0f, 0.0f);
    spawnableCreatureList.clear();
    topBlock = Blocks::sand->getDefaultState()->withProperty(BlockSand::VARIANT, BlockSand::EnumType::RED_SAND);
    fillerBlock = Blocks::stained_hardened_clay->getDefaultState();
    theBiomeDecorator->treesPerChunk = -999;
    theBiomeDecorator->deadBushPerChunk = 20;
    theBiomeDecorator->reedsPerChunk = 3;
    theBiomeDecorator->cactiPerChunk = 5;
    theBiomeDecorator->flowersPerChunk = 0;
    if (forest)
        theBiomeDecorator->treesPerChunk = 5;
}

BiomeGenMesa::~BiomeGenMesa() = default;

std::shared_ptr<WorldGenAbstractTree> BiomeGenMesa::genBigTreeChance(Random&)
{
    return worldGeneratorTrees;
}

void BiomeGenMesa::genTerrainBlocks(World* worldIn, Random& rand, ChunkPrimer& chunkPrimerIn, int x, int z, double noiseVal)
{
    if (field_150621_aC.empty() || field_150622_aD != worldIn->getSeed())
        func_150619_a(worldIn->getSeed());
    if (!field_150623_aE || !field_150624_aF || field_150622_aD != worldIn->getSeed()) {
        Random random(field_150622_aD);
        field_150623_aE = std::make_unique<NoiseGeneratorPerlin>(random, 4);
        field_150624_aF = std::make_unique<NoiseGeneratorPerlin>(random, 1);
    }
    field_150622_aD = worldIn->getSeed();
    double d4 = 0.0;
    if (field_150626_aH) {
        const int i = (x & -16) + (z & 15);
        const int j = (z & -16) + (x & 15);
        const double d0 = std::min(std::abs(noiseVal), field_150623_aE->getValue(static_cast<double>(i) * 0.25, static_cast<double>(j) * 0.25));
        if (d0 > 0.0) {
            const double d1 = 0.001953125;
            const double d2 = std::abs(field_150624_aF->getValue(static_cast<double>(i) * d1, static_cast<double>(j) * d1));
            d4 = d0 * d0 * 2.5;
            const double d3 = std::ceil(d2 * 50.0) + 14.0;
            if (d4 > d3)
                d4 = d3;
            d4 = d4 + 64.0;
        }
    }
    const int j1 = x & 15;
    const int k1 = z & 15;
    const int l1 = worldIn->getSeaLevel();
    const IBlockState* iblockstate = Blocks::stained_hardened_clay->getDefaultState();
    const IBlockState* iblockstate3 = fillerBlock;
    const int k = java::d2i(noiseVal / 3.0 + 3.0 + rand.nextDouble() * 0.25);
    const bool flag = java::mathCos(noiseVal / 3.0 * 3.141592653589793) > 0.0;
    int l = -1;
    bool flag1 = false;
    const int d4i = java::d2i(d4);
    for (int i1 = 255; i1 >= 0; --i1) {
        if (&chunkPrimerIn.getBlockState(k1, i1, j1)->getBlock()->getMaterial() == &Material::air && i1 < d4i)
            chunkPrimerIn.setBlockState(k1, i1, j1, Blocks::stone->getDefaultState());
        if (i1 <= rand.nextInt(5)) {
            chunkPrimerIn.setBlockState(k1, i1, j1, Blocks::bedrock->getDefaultState());
        } else {
            const IBlockState* iblockstate1 = chunkPrimerIn.getBlockState(k1, i1, j1);
            if (&iblockstate1->getBlock()->getMaterial() == &Material::air) {
                l = -1;
            } else if (iblockstate1->getBlock() == Blocks::stone) {
                if (l == -1) {
                    flag1 = false;
                    if (k <= 0) {
                        iblockstate = nullptr;
                        iblockstate3 = Blocks::stone->getDefaultState();
                    } else if (i1 >= l1 - 4 && i1 <= l1 + 1) {
                        iblockstate = Blocks::stained_hardened_clay->getDefaultState();
                        iblockstate3 = fillerBlock;
                    }
                    if (i1 < l1 && (!iblockstate || &iblockstate->getBlock()->getMaterial() == &Material::air))
                        iblockstate = Blocks::water->getDefaultState();
                    l = k + std::max(0, i1 - l1);
                    if (i1 < l1 - 1) {
                        chunkPrimerIn.setBlockState(k1, i1, j1, iblockstate3);
                        if (iblockstate3->getBlock() == Blocks::stained_hardened_clay)
                            chunkPrimerIn.setBlockState(k1, i1, j1, iblockstate3->getBlock()->getDefaultState()->withProperty(BlockColored::COLOR, EnumDyeColor::ORANGE));
                    } else if (field_150620_aI && i1 > 86 + k * 2) {
                        if (flag)
                            chunkPrimerIn.setBlockState(k1, i1, j1, Blocks::dirt->getDefaultState()->withProperty(BlockDirt::VARIANT, BlockDirt::DirtType::COARSE_DIRT));
                        else
                            chunkPrimerIn.setBlockState(k1, i1, j1, Blocks::grass->getDefaultState());
                    } else if (i1 <= l1 + 3 + k) {
                        chunkPrimerIn.setBlockState(k1, i1, j1, topBlock);
                        flag1 = true;
                    } else {
                        const IBlockState* iblockstate4;
                        if (i1 >= 64 && i1 <= 127)
                            iblockstate4 = flag ? Blocks::hardened_clay->getDefaultState() : func_180629_a(x, i1, z);
                        else
                            iblockstate4 = coloredClay(EnumDyeColor::ORANGE);
                        chunkPrimerIn.setBlockState(k1, i1, j1, iblockstate4);
                    }
                } else if (l > 0) {
                    --l;
                    if (flag1)
                        chunkPrimerIn.setBlockState(k1, i1, j1, coloredClay(EnumDyeColor::ORANGE));
                    else
                        chunkPrimerIn.setBlockState(k1, i1, j1, func_180629_a(x, i1, z));
                }
            }
        }
    }
}

void BiomeGenMesa::func_150619_a(int64_t seed)
{
    field_150621_aC.assign(64, Blocks::hardened_clay->getDefaultState());
    Random random(seed);
    field_150625_aG = std::make_unique<NoiseGeneratorPerlin>(random, 1);
    for (int l1 = 0; l1 < 64; ++l1) {
        l1 += random.nextInt(5) + 1;
        if (l1 < 64)
            field_150621_aC[static_cast<size_t>(l1)] = coloredClay(EnumDyeColor::ORANGE);
    }
    const auto bands = [&](int widthBase, int widthRange, EnumDyeColor color) {
        const int count = random.nextInt(4) + 2;
        for (int i = 0; i < count; ++i) {
            const int j = random.nextInt(widthRange) + widthBase;
            const int k = random.nextInt(64);
            for (int l = 0; k + l < 64 && l < j; ++l)
                field_150621_aC[static_cast<size_t>(k + l)] = coloredClay(color);
        }
    };
    bands(1, 3, EnumDyeColor::YELLOW);
    bands(2, 3, EnumDyeColor::BROWN);
    bands(1, 3, EnumDyeColor::RED);
    const int k3 = random.nextInt(3) + 3;
    int j4 = 0;
    for (int l4 = 0; l4 < k3; ++l4) {
        const int i5 = 1;
        j4 += random.nextInt(16) + 4;
        for (int k1 = 0; j4 + k1 < 64 && k1 < i5; ++k1) {
            field_150621_aC[static_cast<size_t>(j4 + k1)] = coloredClay(EnumDyeColor::WHITE);
            if (j4 + k1 > 1 && random.nextBoolean())
                field_150621_aC[static_cast<size_t>(j4 + k1 - 1)] = coloredClay(EnumDyeColor::SILVER);
            if (j4 + k1 < 63 && random.nextBoolean())
                field_150621_aC[static_cast<size_t>(j4 + k1 + 1)] = coloredClay(EnumDyeColor::SILVER);
        }
    }
}

const IBlockState* BiomeGenMesa::func_180629_a(int x, int y, int) const
{
    // Java samples with x twice (a vanilla quirk).
    const int i = static_cast<int>(java::round(field_150625_aG->getValue(static_cast<double>(x) * 1.0 / 512.0, static_cast<double>(x) * 1.0 / 512.0) * 2.0));
    return field_150621_aC[static_cast<size_t>((y + i + 64) % 64)];
}

BiomeGenBase* BiomeGenMesa::createMutatedBiome(int id)
{
    const bool flag = biomeID == BiomeGenBase::mesa->biomeID;
    BiomeGenMesa* biomegenmesa = create<BiomeGenMesa>(id, flag, field_150620_aI);
    if (!flag) {
        biomegenmesa->setHeight(height_LowHills);
        biomegenmesa->setBiomeName(biomeName + " M");
    } else {
        biomegenmesa->setBiomeName(biomeName + " (Bryce)");
    }
    biomegenmesa->func_150557_a(color, true);
    return biomegenmesa;
}

// --- BiomeGenMutated ---

BiomeGenMutated::BiomeGenMutated(int id, BiomeGenBase* biome)
    : BiomeGenBase(id)
    , baseBiome(biome)
{
    func_150557_a(biome->color, true);
    biomeName = biome->biomeName + " M";
    topBlock = biome->topBlock;
    fillerBlock = biome->fillerBlock;
    fillerBlockMetadata = biome->fillerBlockMetadata;
    minHeight = biome->minHeight;
    maxHeight = biome->maxHeight;
    temperature = biome->temperature;
    rainfall = biome->rainfall;
    waterColorMultiplier = biome->waterColorMultiplier;
    enableSnow = biome->enableSnow;
    enableRain = biome->enableRain;
    spawnableCreatureList = biome->spawnableCreatureList;
    spawnableMonsterList = biome->spawnableMonsterList;
    spawnableCaveCreatureList = biome->spawnableCaveCreatureList;
    spawnableWaterCreatureList = biome->spawnableWaterCreatureList;
    temperature = biome->temperature;
    rainfall = biome->rainfall;
    minHeight = biome->minHeight + 0.1f;
    maxHeight = biome->maxHeight + 0.2f;
}

void BiomeGenMutated::decorate(World* worldIn, Random& rand, const BlockPos& pos)
{
    baseBiome->theBiomeDecorator->decorate(worldIn, rand, this, pos);
}

void BiomeGenMutated::genTerrainBlocks(World* worldIn, Random& rand, ChunkPrimer& chunkPrimerIn, int x, int z, double noiseVal)
{
    baseBiome->genTerrainBlocks(worldIn, rand, chunkPrimerIn, x, z, noiseVal);
}

} // namespace mc

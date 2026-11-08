#include "world/biome/BiomeGenBase.h"

#include "block/AllBlocks.h"
#include "core/Log.h"
#include "init/Blocks.h"
#include "util/MathHelper.h"
#include "util/Random.h"
#include "world/Colorizer.h"
#include "world/World.h"
#include "world/biome/BiomeDecorator.h"
#include "world/biome/Biomes.h"
#include "world/chunk/Chunk.h"
#include "world/gen/NoiseGenerators.h"
#include "world/gen/feature/WorldGenerators.h"

#include <algorithm>
#include <stdexcept>

namespace mc {

const BiomeGenBase::Height BiomeGenBase::height_Default { 0.1f, 0.2f };
const BiomeGenBase::Height BiomeGenBase::height_ShallowWaters { -0.5f, 0.0f };
const BiomeGenBase::Height BiomeGenBase::height_Oceans { -1.0f, 0.1f };
const BiomeGenBase::Height BiomeGenBase::height_DeepOceans { -1.8f, 0.1f };
const BiomeGenBase::Height BiomeGenBase::height_LowPlains { 0.125f, 0.05f };
const BiomeGenBase::Height BiomeGenBase::height_MidPlains { 0.2f, 0.2f };
const BiomeGenBase::Height BiomeGenBase::height_LowHills { 0.45f, 0.3f };
const BiomeGenBase::Height BiomeGenBase::height_HighPlateaus { 1.5f, 0.025f };
const BiomeGenBase::Height BiomeGenBase::height_MidHills { 1.0f, 0.5f };
const BiomeGenBase::Height BiomeGenBase::height_Shores { 0.0f, 0.025f };
const BiomeGenBase::Height BiomeGenBase::height_RockyWaters { 0.1f, 0.8f };
const BiomeGenBase::Height BiomeGenBase::height_LowIslands { 0.2f, 0.3f };
const BiomeGenBase::Height BiomeGenBase::height_PartiallySubmerged { -0.2f, 0.1f };

std::array<BiomeGenBase*, 256> BiomeGenBase::biomeList {};
std::vector<BiomeGenBase*> BiomeGenBase::explorationBiomesList;
std::unordered_map<std::string, BiomeGenBase*> BiomeGenBase::BIOME_ID_MAP;

std::vector<std::unique_ptr<BiomeGenBase>>& BiomeGenBase::ownedBiomes()
{
    static std::vector<std::unique_ptr<BiomeGenBase>> biomes;
    return biomes;
}

NoiseGeneratorPerlin& BiomeGenBase::temperatureNoise()
{
    static NoiseGeneratorPerlin noise = [] {
        Random random(1234LL);
        return NoiseGeneratorPerlin(random, 1);
    }();
    return noise;
}

NoiseGeneratorPerlin& BiomeGenBase::GRASS_COLOR_NOISE()
{
    static NoiseGeneratorPerlin noise = [] {
        Random random(2345LL);
        return NoiseGeneratorPerlin(random, 1);
    }();
    return noise;
}

WorldGenDoublePlant& BiomeGenBase::DOUBLE_PLANT_GENERATOR()
{
    static WorldGenDoublePlant generator;
    return generator;
}

BiomeGenBase::BiomeGenBase(int id)
    : topBlock(Blocks::grass->getDefaultState())
    , fillerBlock(Blocks::dirt->getDefaultState())
    , minHeight(height_Default.rootHeight)
    , maxHeight(height_Default.variation)
    , temperature(0.5f)
    , rainfall(0.5f)
    , waterColorMultiplier(16777215)
    , biomeID(id)
    , worldGeneratorTrees(std::make_shared<WorldGenTrees>(false))
    , worldGeneratorBigTree(std::make_shared<WorldGenBigTree>(false))
    , worldGeneratorSwamp(std::make_shared<WorldGenSwamp>())
{
    biomeList[static_cast<size_t>(id)] = this;
    theBiomeDecorator = createBiomeDecorator();
    spawnableCreatureList = { { "Sheep", 12, 4, 4 }, { "Rabbit", 10, 3, 3 }, { "Pig", 10, 4, 4 }, { "Chicken", 10, 4, 4 }, { "Cow", 8, 4, 4 } };
    spawnableMonsterList = { { "Spider", 100, 4, 4 }, { "Zombie", 100, 4, 4 }, { "Skeleton", 100, 4, 4 }, { "Creeper", 100, 4, 4 }, { "Slime", 100, 4, 4 },
        { "Enderman", 10, 1, 4 }, { "Witch", 5, 1, 1 } };
    spawnableWaterCreatureList = { { "Squid", 10, 4, 4 } };
    spawnableCaveCreatureList = { { "Bat", 10, 8, 8 } };
}

BiomeGenBase::~BiomeGenBase() = default;

std::unique_ptr<BiomeDecorator> BiomeGenBase::createBiomeDecorator()
{
    return std::make_unique<BiomeDecorator>();
}

BiomeGenBase* BiomeGenBase::setTemperatureRainfall(float temperatureIn, float rainfallIn)
{
    if (temperatureIn > 0.1f && temperatureIn < 0.2f)
        throw std::invalid_argument("Please avoid temperatures in the range 0.1 - 0.2 because of snow");
    temperature = temperatureIn;
    rainfall = rainfallIn;
    return this;
}

BiomeGenBase* BiomeGenBase::setHeight(const Height& heights)
{
    minHeight = heights.rootHeight;
    maxHeight = heights.variation;
    return this;
}

BiomeGenBase* BiomeGenBase::setDisableRain()
{
    enableRain = false;
    return this;
}

std::shared_ptr<WorldGenAbstractTree> BiomeGenBase::genBigTreeChance(Random& rand)
{
    if (rand.nextInt(10) == 0)
        return worldGeneratorBigTree;
    return worldGeneratorTrees;
}

std::shared_ptr<WorldGenerator> BiomeGenBase::getRandomWorldGenForGrass(Random&)
{
    return std::make_shared<WorldGenTallGrass>(BlockTallGrass::EnumType::GRASS);
}

BlockFlower::EnumFlowerType BiomeGenBase::pickRandomFlower(Random& rand, const BlockPos&)
{
    return rand.nextInt(3) > 0 ? BlockFlower::EnumFlowerType::DANDELION : BlockFlower::EnumFlowerType::POPPY;
}

BiomeGenBase* BiomeGenBase::setEnableSnow()
{
    enableSnow = true;
    return this;
}

BiomeGenBase* BiomeGenBase::setBiomeName(std::string name)
{
    biomeName = std::move(name);
    return this;
}

BiomeGenBase* BiomeGenBase::setFillerBlockMetadata(int meta)
{
    fillerBlockMetadata = meta;
    return this;
}

BiomeGenBase* BiomeGenBase::setColor(int colorIn)
{
    func_150557_a(colorIn, false);
    return this;
}

BiomeGenBase* BiomeGenBase::func_150563_c(int value)
{
    field_150609_ah = value;
    return this;
}

BiomeGenBase* BiomeGenBase::func_150557_a(int colorIn, bool darken)
{
    color = colorIn;
    field_150609_ah = darken ? (colorIn & 16711422) >> 1 : colorIn;
    return this;
}

int BiomeGenBase::getSkyColorByTemp(float temperatureIn)
{
    temperatureIn = temperatureIn / 3.0f;
    temperatureIn = MathHelper::clamp_float(temperatureIn, -1.0f, 1.0f);
    return MathHelper::hsvToRGB(0.62222224f - temperatureIn * 0.05f, 0.5f + temperatureIn * 0.1f, 1.0f);
}

const std::vector<BiomeGenBase::SpawnListEntry>& BiomeGenBase::getSpawnableList(EnumCreatureType creatureType) const
{
    switch (creatureType) {
    case EnumCreatureType::MONSTER: return spawnableMonsterList;
    case EnumCreatureType::CREATURE: return spawnableCreatureList;
    case EnumCreatureType::WATER_CREATURE: return spawnableWaterCreatureList;
    case EnumCreatureType::AMBIENT: return spawnableCaveCreatureList;
    }
    static const std::vector<SpawnListEntry> empty;
    return empty;
}

int BiomeGenBase::getIntRainfall() const
{
    return java::f2i(rainfall * 65536.0f);
}

float BiomeGenBase::getFloatTemperature(const BlockPos& pos) const
{
    if (pos.getY() > 64) {
        const float f = static_cast<float>(temperatureNoise().getValue(static_cast<double>(pos.getX()) * 1.0 / 8.0, static_cast<double>(pos.getZ()) * 1.0 / 8.0) * 4.0);
        return temperature - (f + static_cast<float>(pos.getY()) - 64.0f) * 0.05f / 30.0f;
    }
    return temperature;
}

void BiomeGenBase::decorate(World* worldIn, Random& rand, const BlockPos& pos)
{
    theBiomeDecorator->decorate(worldIn, rand, this, pos);
}

int BiomeGenBase::getGrassColorAtPos(const BlockPos& pos)
{
    const double d0 = static_cast<double>(MathHelper::clamp_float(getFloatTemperature(pos), 0.0f, 1.0f));
    const double d1 = static_cast<double>(MathHelper::clamp_float(getFloatRainfall(), 0.0f, 1.0f));
    return ColorizerGrass::getGrassColor(d0, d1);
}

int BiomeGenBase::getFoliageColorAtPos(const BlockPos& pos)
{
    const double d0 = static_cast<double>(MathHelper::clamp_float(getFloatTemperature(pos), 0.0f, 1.0f));
    const double d1 = static_cast<double>(MathHelper::clamp_float(getFloatRainfall(), 0.0f, 1.0f));
    return ColorizerFoliage::getFoliageColor(d0, d1);
}

void BiomeGenBase::genTerrainBlocks(World* worldIn, Random& rand, ChunkPrimer& chunkPrimerIn, int x, int z, double noiseVal)
{
    generateBiomeTerrain(worldIn, rand, chunkPrimerIn, x, z, noiseVal);
}

void BiomeGenBase::generateBiomeTerrain(World* worldIn, Random& rand, ChunkPrimer& chunkPrimerIn, int x, int z, double noiseVal)
{
    const int i = worldIn->getSeaLevel();
    const IBlockState* iblockstate = topBlock;
    const IBlockState* iblockstate1 = fillerBlock;
    int j = -1;
    const int k = java::d2i(noiseVal / 3.0 + 3.0 + rand.nextDouble() * 0.25);
    const int l = x & 15;
    const int i1 = z & 15;
    BlockPos mutablePos;
    for (int j1 = 255; j1 >= 0; --j1) {
        if (j1 <= rand.nextInt(5)) {
            chunkPrimerIn.setBlockState(i1, j1, l, Blocks::bedrock->getDefaultState());
        } else {
            const IBlockState* iblockstate2 = chunkPrimerIn.getBlockState(i1, j1, l);
            if (&iblockstate2->getBlock()->getMaterial() == &Material::air) {
                j = -1;
            } else if (iblockstate2->getBlock() == Blocks::stone) {
                if (j == -1) {
                    if (k <= 0) {
                        iblockstate = nullptr;
                        iblockstate1 = Blocks::stone->getDefaultState();
                    } else if (j1 >= i - 4 && j1 <= i + 1) {
                        iblockstate = topBlock;
                        iblockstate1 = fillerBlock;
                    }
                    if (j1 < i && (!iblockstate || &iblockstate->getBlock()->getMaterial() == &Material::air)) {
                        if (getFloatTemperature(mutablePos.set(x, j1, z)) < 0.15f)
                            iblockstate = Blocks::ice->getDefaultState();
                        else
                            iblockstate = Blocks::water->getDefaultState();
                    }
                    j = k;
                    if (j1 >= i - 1) {
                        // Java: setBlockState with a null state stores air there.
                        chunkPrimerIn.setBlockState(i1, j1, l, iblockstate ? iblockstate : Blocks::air->getDefaultState());
                    } else if (j1 < i - 7 - k) {
                        iblockstate = nullptr;
                        iblockstate1 = Blocks::stone->getDefaultState();
                        chunkPrimerIn.setBlockState(i1, j1, l, Blocks::gravel->getDefaultState());
                    } else {
                        chunkPrimerIn.setBlockState(i1, j1, l, iblockstate1);
                    }
                } else if (j > 0) {
                    --j;
                    chunkPrimerIn.setBlockState(i1, j1, l, iblockstate1);
                    if (j == 0 && iblockstate1->getBlock() == Blocks::sand) {
                        j = rand.nextInt(4) + std::max(0, j1 - 63);
                        iblockstate1 = iblockstate1->getValue(BlockSand::VARIANT) == BlockSand::EnumType::RED_SAND ? Blocks::red_sandstone->getDefaultState()
                                                                                                                   : Blocks::sandstone->getDefaultState();
                    }
                }
            }
        }
    }
}

BiomeGenBase* BiomeGenBase::createMutatedBiome(int id)
{
    return create<BiomeGenMutated>(id, this);
}

bool BiomeGenBase::isEqualTo(const BiomeGenBase* biome) const
{
    return biome == this ? true : (!biome ? false : getBiomeClass() == biome->getBiomeClass());
}

BiomeGenBase::TempCategory BiomeGenBase::getTempCategory() const
{
    return static_cast<double>(temperature) < 0.2 ? TempCategory::COLD : (static_cast<double>(temperature) < 1.0 ? TempCategory::MEDIUM : TempCategory::WARM);
}

BiomeGenBase* BiomeGenBase::getBiomeFromBiomeList(int biomeId, BiomeGenBase* biome)
{
    if (biomeId >= 0 && biomeId <= static_cast<int>(biomeList.size())) {
        // Java indexes the 256-entry array, so 256 throws.
        BiomeGenBase* biomegenbase = biomeList.at(static_cast<size_t>(biomeId));
        return biomegenbase ? biomegenbase : biome;
    }
    Log::warn("Biome ID is out of bounds: {}, defaulting to 0 (Ocean)", biomeId);
    return ocean;
}

void BiomeGenBase::registerBiomes()
{
    if (ocean)
        return;
    ocean = create<BiomeGenOcean>(0)->setColor(112)->setBiomeName("Ocean")->setHeight(height_Oceans);
    plains = create<BiomeGenPlains>(1)->setColor(9286496)->setBiomeName("Plains");
    desert = create<BiomeGenDesert>(2)->setColor(16421912)->setBiomeName("Desert")->setDisableRain()->setTemperatureRainfall(2.0f, 0.0f)->setHeight(height_LowPlains);
    extremeHills = create<BiomeGenHills>(3, false)->setColor(6316128)->setBiomeName("Extreme Hills")->setHeight(height_MidHills)->setTemperatureRainfall(0.2f, 0.3f);
    forest = create<BiomeGenForest>(4, 0)->setColor(353825)->setBiomeName("Forest");
    taiga = create<BiomeGenTaiga>(5, 0)->setColor(747097)->setBiomeName("Taiga")->setFillerBlockMetadata(5159473)->setTemperatureRainfall(0.25f, 0.8f)->setHeight(height_MidPlains);
    swampland = create<BiomeGenSwamp>(6)->setColor(522674)->setBiomeName("Swampland")->setFillerBlockMetadata(9154376)->setHeight(height_PartiallySubmerged)->setTemperatureRainfall(0.8f, 0.9f);
    river = create<BiomeGenRiver>(7)->setColor(255)->setBiomeName("River")->setHeight(height_ShallowWaters);
    hell = create<BiomeGenHell>(8)->setColor(16711680)->setBiomeName("Hell")->setDisableRain()->setTemperatureRainfall(2.0f, 0.0f);
    sky = create<BiomeGenEnd>(9)->setColor(8421631)->setBiomeName("The End")->setDisableRain();
    frozenOcean = create<BiomeGenOcean>(10)->setColor(9474208)->setBiomeName("FrozenOcean")->setEnableSnow()->setHeight(height_Oceans)->setTemperatureRainfall(0.0f, 0.5f);
    frozenRiver = create<BiomeGenRiver>(11)->setColor(10526975)->setBiomeName("FrozenRiver")->setEnableSnow()->setHeight(height_ShallowWaters)->setTemperatureRainfall(0.0f, 0.5f);
    icePlains = create<BiomeGenSnow>(12, false)->setColor(16777215)->setBiomeName("Ice Plains")->setEnableSnow()->setTemperatureRainfall(0.0f, 0.5f)->setHeight(height_LowPlains);
    iceMountains = create<BiomeGenSnow>(13, false)->setColor(10526880)->setBiomeName("Ice Mountains")->setEnableSnow()->setHeight(height_LowHills)->setTemperatureRainfall(0.0f, 0.5f);
    mushroomIsland = create<BiomeGenMushroomIsland>(14)->setColor(16711935)->setBiomeName("MushroomIsland")->setTemperatureRainfall(0.9f, 1.0f)->setHeight(height_LowIslands);
    mushroomIslandShore = create<BiomeGenMushroomIsland>(15)->setColor(10486015)->setBiomeName("MushroomIslandShore")->setTemperatureRainfall(0.9f, 1.0f)->setHeight(height_Shores);
    beach = create<BiomeGenBeach>(16)->setColor(16440917)->setBiomeName("Beach")->setTemperatureRainfall(0.8f, 0.4f)->setHeight(height_Shores);
    desertHills = create<BiomeGenDesert>(17)->setColor(13786898)->setBiomeName("DesertHills")->setDisableRain()->setTemperatureRainfall(2.0f, 0.0f)->setHeight(height_LowHills);
    forestHills = create<BiomeGenForest>(18, 0)->setColor(2250012)->setBiomeName("ForestHills")->setHeight(height_LowHills);
    taigaHills = create<BiomeGenTaiga>(19, 0)->setColor(1456435)->setBiomeName("TaigaHills")->setFillerBlockMetadata(5159473)->setTemperatureRainfall(0.25f, 0.8f)->setHeight(height_LowHills);
    extremeHillsEdge = create<BiomeGenHills>(20, true)->setColor(7501978)->setBiomeName("Extreme Hills Edge")->setHeight(height_MidHills.attenuate())->setTemperatureRainfall(0.2f, 0.3f);
    jungle = create<BiomeGenJungle>(21, false)->setColor(5470985)->setBiomeName("Jungle")->setFillerBlockMetadata(5470985)->setTemperatureRainfall(0.95f, 0.9f);
    jungleHills = create<BiomeGenJungle>(22, false)->setColor(2900485)->setBiomeName("JungleHills")->setFillerBlockMetadata(5470985)->setTemperatureRainfall(0.95f, 0.9f)->setHeight(height_LowHills);
    jungleEdge = create<BiomeGenJungle>(23, true)->setColor(6458135)->setBiomeName("JungleEdge")->setFillerBlockMetadata(5470985)->setTemperatureRainfall(0.95f, 0.8f);
    deepOcean = create<BiomeGenOcean>(24)->setColor(48)->setBiomeName("Deep Ocean")->setHeight(height_DeepOceans);
    stoneBeach = create<BiomeGenStoneBeach>(25)->setColor(10658436)->setBiomeName("Stone Beach")->setTemperatureRainfall(0.2f, 0.3f)->setHeight(height_RockyWaters);
    coldBeach = create<BiomeGenBeach>(26)->setColor(16445632)->setBiomeName("Cold Beach")->setTemperatureRainfall(0.05f, 0.3f)->setHeight(height_Shores)->setEnableSnow();
    birchForest = create<BiomeGenForest>(27, 2)->setBiomeName("Birch Forest")->setColor(3175492);
    birchForestHills = create<BiomeGenForest>(28, 2)->setBiomeName("Birch Forest Hills")->setColor(2055986)->setHeight(height_LowHills);
    roofedForest = create<BiomeGenForest>(29, 3)->setColor(4215066)->setBiomeName("Roofed Forest");
    coldTaiga = create<BiomeGenTaiga>(30, 0)->setColor(3233098)->setBiomeName("Cold Taiga")->setFillerBlockMetadata(5159473)->setEnableSnow()->setTemperatureRainfall(-0.5f, 0.4f)->setHeight(height_MidPlains)->func_150563_c(16777215);
    coldTaigaHills = create<BiomeGenTaiga>(31, 0)->setColor(2375478)->setBiomeName("Cold Taiga Hills")->setFillerBlockMetadata(5159473)->setEnableSnow()->setTemperatureRainfall(-0.5f, 0.4f)->setHeight(height_LowHills)->func_150563_c(16777215);
    megaTaiga = create<BiomeGenTaiga>(32, 1)->setColor(5858897)->setBiomeName("Mega Taiga")->setFillerBlockMetadata(5159473)->setTemperatureRainfall(0.3f, 0.8f)->setHeight(height_MidPlains);
    megaTaigaHills = create<BiomeGenTaiga>(33, 1)->setColor(4542270)->setBiomeName("Mega Taiga Hills")->setFillerBlockMetadata(5159473)->setTemperatureRainfall(0.3f, 0.8f)->setHeight(height_LowHills);
    extremeHillsPlus = create<BiomeGenHills>(34, true)->setColor(5271632)->setBiomeName("Extreme Hills+")->setHeight(height_MidHills)->setTemperatureRainfall(0.2f, 0.3f);
    savanna = create<BiomeGenSavanna>(35)->setColor(12431967)->setBiomeName("Savanna")->setTemperatureRainfall(1.2f, 0.0f)->setDisableRain()->setHeight(height_LowPlains);
    savannaPlateau = create<BiomeGenSavanna>(36)->setColor(10984804)->setBiomeName("Savanna Plateau")->setTemperatureRainfall(1.0f, 0.0f)->setDisableRain()->setHeight(height_HighPlateaus);
    mesa = create<BiomeGenMesa>(37, false, false)->setColor(14238997)->setBiomeName("Mesa");
    mesaPlateau_F = create<BiomeGenMesa>(38, false, true)->setColor(11573093)->setBiomeName("Mesa Plateau F")->setHeight(height_HighPlateaus);
    mesaPlateau = create<BiomeGenMesa>(39, false, false)->setColor(13274213)->setBiomeName("Mesa Plateau")->setHeight(height_HighPlateaus);
    field_180279_ad = ocean;

    plains->createMutation();
    desert->createMutation();
    forest->createMutation();
    taiga->createMutation();
    swampland->createMutation();
    icePlains->createMutation();
    jungle->createMutation();
    jungleEdge->createMutation();
    coldTaiga->createMutation();
    savanna->createMutation();
    savannaPlateau->createMutation();
    mesa->createMutation();
    mesaPlateau_F->createMutation();
    mesaPlateau->createMutation();
    birchForest->createMutation();
    birchForestHills->createMutation();
    roofedForest->createMutation();
    megaTaiga->createMutation();
    extremeHills->createMutation();
    extremeHillsPlus->createMutation();
    megaTaiga->createMutatedBiome(megaTaigaHills->biomeID + 128)->setBiomeName("Redwood Taiga Hills M");

    for (BiomeGenBase* biomegenbase : biomeList) {
        if (biomegenbase) {
            if (BIOME_ID_MAP.contains(biomegenbase->biomeName))
                throw std::logic_error("Biome \"" + biomegenbase->biomeName + "\" is defined as both ID " + std::to_string(BIOME_ID_MAP[biomegenbase->biomeName]->biomeID)
                    + " and " + std::to_string(biomegenbase->biomeID));
            BIOME_ID_MAP[biomegenbase->biomeName] = biomegenbase;
            if (biomegenbase->biomeID < 128)
                explorationBiomesList.push_back(biomegenbase);
        }
    }
    for (BiomeGenBase* removed : { hell, sky, frozenOcean, extremeHillsEdge })
        std::erase(explorationBiomesList, removed);
    temperatureNoise();
    GRASS_COLOR_NOISE();
    DOUBLE_PLANT_GENERATOR();
}

} // namespace mc

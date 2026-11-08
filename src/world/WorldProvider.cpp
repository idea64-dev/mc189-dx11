#include "world/WorldProvider.h"

#include "block/BlockGrass.h"
#include "core/JavaMath.h"
#include "init/Blocks.h"
#include "util/MathHelper.h"
#include "world/World.h"
#include "world/WorldType.h"
#include "world/biome/BiomeGenBase.h"
#include "world/biome/WorldChunkManager.h"
#include "world/chunk/Chunk.h"
#include "world/border/WorldBorder.h"
#include "world/gen/ChunkProviderDebug.h"
#include "world/gen/ChunkProviderEnd.h"
#include "world/gen/ChunkProviderFlat.h"
#include "world/gen/ChunkProviderGenerate.h"
#include "world/gen/ChunkProviderHell.h"
#include "world/gen/FlatGeneratorInfo.h"
#include "world/storage/WorldInfo.h"

namespace mc {

std::unique_ptr<WorldProvider> WorldProvider::getProviderForDimension(int dimension)
{
    switch (dimension) {
    case -1: return std::make_unique<WorldProviderHell>();
    case 0: return std::make_unique<WorldProviderSurface>();
    case 1: return std::make_unique<WorldProviderEnd>();
    default: return nullptr;
    }
}

WorldProvider::WorldProvider() = default;
WorldProvider::~WorldProvider() = default;

void WorldProvider::registerWorldChunkManager()
{
    const WorldType* worldtype = worldObj->getWorldInfo().getTerrainType();
    if (worldtype == &WorldType::FLAT) {
        const FlatGeneratorInfo flatgeneratorinfo = FlatGeneratorInfo::createFlatGeneratorFromString(worldObj->getWorldInfo().getGeneratorOptions());
        worldChunkMgr = std::make_unique<WorldChunkManagerHell>(
            BiomeGenBase::getBiomeFromBiomeList(flatgeneratorinfo.getBiome(), BiomeGenBase::field_180279_ad), 0.5f);
    } else if (worldtype == &WorldType::DEBUG_WORLD) {
        worldChunkMgr = std::make_unique<WorldChunkManagerHell>(BiomeGenBase::plains, 0.0f);
    } else {
        worldChunkMgr = std::make_unique<WorldChunkManager>(worldObj);
    }
}

std::unique_ptr<IChunkProvider> WorldProvider::createChunkGenerator()
{
    if (terrainType == &WorldType::FLAT)
        return std::make_unique<ChunkProviderFlat>(worldObj, worldObj->getSeed(), worldObj->getWorldInfo().isMapFeaturesEnabled(), generatorSettings);
    if (terrainType == &WorldType::DEBUG_WORLD)
        return std::make_unique<ChunkProviderDebug>(worldObj);
    return std::make_unique<ChunkProviderGenerate>(worldObj, worldObj->getSeed(), worldObj->getWorldInfo().isMapFeaturesEnabled(), generatorSettings);
}

void WorldProvider::registerWorld(World* worldIn)
{
    worldObj = worldIn;
    terrainType = worldIn->getWorldInfo().getTerrainType();
    generatorSettings = worldIn->getWorldInfo().getGeneratorOptions();
    registerWorldChunkManager();
    generateLightBrightnessTable();
}

void WorldProvider::generateLightBrightnessTable()
{
    const float f = 0.0f;
    for (int i = 0; i <= 15; ++i) {
        const float f1 = 1.0f - static_cast<float>(i) / 15.0f;
        lightBrightnessTable[static_cast<size_t>(i)] = (1.0f - f1) / (f1 * 3.0f + 1.0f) * (1.0f - f) + f;
    }
}

bool WorldProvider::canCoordinateBeSpawn(int x, int z) const
{
    return worldObj->getGroundAboveSeaLevel(BlockPos(x, 0, z)) == Blocks::grass;
}

float WorldProvider::calculateCelestialAngle(int64_t worldTime, float partialTicks) const
{
    const int i = static_cast<int>(worldTime % 24000LL);
    float f = (static_cast<float>(i) + partialTicks) / 24000.0f - 0.25f;
    if (f < 0.0f)
        ++f;
    if (f > 1.0f)
        --f;
    f = 1.0f - static_cast<float>((java::mathCos(static_cast<double>(f) * 3.141592653589793) + 1.0) / 2.0);
    f = f + (f - f) / 3.0f;
    return f;
}

int WorldProvider::getMoonPhase(int64_t worldTime) const
{
    return static_cast<int>((worldTime / 24000LL % 8LL + 8LL) % 8LL);
}

std::optional<std::array<float, 4>> WorldProvider::calcSunriseSunsetColors(float celestialAngle, float) const
{
    const float f = 0.4f;
    const float f1 = MathHelper::cos(celestialAngle * 3.1415927f * 2.0f) - 0.0f;
    const float f2 = -0.0f;
    if (f1 >= f2 - f && f1 <= f2 + f) {
        const float f3 = (f1 - f2) / f * 0.5f + 0.5f;
        float f4 = 1.0f - (1.0f - MathHelper::sin(f3 * 3.1415927f)) * 0.99f;
        f4 = f4 * f4;
        return std::array<float, 4> { f3 * 0.3f + 0.7f, f3 * f3 * 0.7f + 0.2f, f3 * f3 * 0.0f + 0.2f, f4 };
    }
    return std::nullopt;
}

Vec3 WorldProvider::getFogColor(float celestialAngle, float) const
{
    float f = MathHelper::cos(celestialAngle * 3.1415927f * 2.0f) * 2.0f + 0.5f;
    f = MathHelper::clamp_float(f, 0.0f, 1.0f);
    float f1 = 0.7529412f;
    float f2 = 0.84705883f;
    float f3 = 1.0f;
    f1 = f1 * (f * 0.94f + 0.06f);
    f2 = f2 * (f * 0.94f + 0.06f);
    f3 = f3 * (f * 0.91f + 0.09f);
    return Vec3(static_cast<double>(f1), static_cast<double>(f2), static_cast<double>(f3));
}

int WorldProvider::getAverageGroundLevel() const
{
    return terrainType == &WorldType::FLAT ? 4 : worldObj->getSeaLevel() + 1;
}

double WorldProvider::getVoidFogYFactor() const
{
    return terrainType == &WorldType::FLAT ? 1.0 : 0.03125;
}

std::unique_ptr<WorldBorder> WorldProvider::getWorldBorder() const
{
    return std::make_unique<WorldBorder>();
}

// --- Nether ---

namespace {

class NetherWorldBorder : public WorldBorder {
public:
    double getCenterX() const override { return WorldBorder::getCenterX() / 8.0; }
    double getCenterZ() const override { return WorldBorder::getCenterZ() / 8.0; }
};

} // namespace

void WorldProviderHell::registerWorldChunkManager()
{
    worldChunkMgr = std::make_unique<WorldChunkManagerHell>(BiomeGenBase::hell, 0.0f);
    isHellWorld = true;
    hasNoSky = true;
    dimensionId = -1;
}

void WorldProviderHell::generateLightBrightnessTable()
{
    const float f = 0.1f;
    for (int i = 0; i <= 15; ++i) {
        const float f1 = 1.0f - static_cast<float>(i) / 15.0f;
        lightBrightnessTable[static_cast<size_t>(i)] = (1.0f - f1) / (f1 * 3.0f + 1.0f) * (1.0f - f) + f;
    }
}

std::unique_ptr<WorldBorder> WorldProviderHell::getWorldBorder() const
{
    return std::make_unique<NetherWorldBorder>();
}

std::unique_ptr<IChunkProvider> WorldProviderHell::createChunkGenerator()
{
    return std::make_unique<ChunkProviderHell>(worldObj, worldObj->getWorldInfo().isMapFeaturesEnabled(), worldObj->getSeed());
}

// --- End ---

std::unique_ptr<IChunkProvider> WorldProviderEnd::createChunkGenerator()
{
    return std::make_unique<ChunkProviderEnd>(worldObj, worldObj->getSeed());
}

void WorldProviderEnd::registerWorldChunkManager()
{
    worldChunkMgr = std::make_unique<WorldChunkManagerHell>(BiomeGenBase::sky, 0.0f);
    dimensionId = 1;
    hasNoSky = true;
}

Vec3 WorldProviderEnd::getFogColor(float celestialAngle, float) const
{
    const int i = 10518688;
    float f = MathHelper::cos(celestialAngle * 3.1415927f * 2.0f) * 2.0f + 0.5f;
    f = MathHelper::clamp_float(f, 0.0f, 1.0f);
    float f1 = static_cast<float>(i >> 16 & 255) / 255.0f;
    float f2 = static_cast<float>(i >> 8 & 255) / 255.0f;
    float f3 = static_cast<float>(i & 255) / 255.0f;
    f1 = f1 * (f * 0.0f + 0.15f);
    f2 = f2 * (f * 0.0f + 0.15f);
    f3 = f3 * (f * 0.0f + 0.15f);
    return Vec3(static_cast<double>(f1), static_cast<double>(f2), static_cast<double>(f3));
}

bool WorldProviderEnd::canCoordinateBeSpawn(int x, int z) const
{
    return worldObj->getGroundAboveSeaLevel(BlockPos(x, 0, z))->getMaterial().blocksMovement();
}

} // namespace mc

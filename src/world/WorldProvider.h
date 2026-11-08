#pragma once

// Original: net/minecraft/world/WorldProvider.java, WorldProviderSurface.java, WorldProviderHell.java and
// WorldProviderEnd.java.
// Not ported yet: the biome source (registerWorldChunkManager / getWorldChunkManager) and the chunk generators
// (createChunkGenerator) -- world generation, stage 6.

#include "util/BlockPos.h"
#include "util/Vec3.h"

#include <array>
#include <memory>
#include <optional>
#include <string>

namespace mc {

class World;
class WorldBorder;
class WorldType;

class WorldProvider {
public:
    static constexpr float moonPhaseFactors[8] { 1.0f, 0.75f, 0.5f, 0.25f, 0.0f, 0.25f, 0.5f, 0.75f };

    virtual ~WorldProvider() = default;

    // -1 Nether, 0 Overworld, 1 End; null otherwise.
    static std::unique_ptr<WorldProvider> getProviderForDimension(int dimension);

    void registerWorld(World* worldIn);
    virtual bool canCoordinateBeSpawn(int x, int z) const;
    virtual float calculateCelestialAngle(int64_t worldTime, float partialTicks) const;
    int getMoonPhase(int64_t worldTime) const;
    virtual bool isSurfaceWorld() const { return true; }
    // Java returns null outside sunrise and sunset.
    virtual std::optional<std::array<float, 4>> calcSunriseSunsetColors(float celestialAngle, float partialTicks) const;
    virtual Vec3 getFogColor(float celestialAngle, float partialTicks) const;
    virtual bool canRespawnHere() const { return true; }
    virtual float getCloudHeight() const { return 128.0f; }
    virtual bool isSkyColored() const { return true; }
    // Java returns null except in the End.
    virtual std::optional<BlockPos> getSpawnCoordinate() const { return std::nullopt; }
    virtual int getAverageGroundLevel() const;
    double getVoidFogYFactor() const;
    virtual bool doesXZShowFog(int, int) const { return false; }
    virtual std::string getDimensionName() const = 0;
    virtual std::string getInternalNameSuffix() const = 0;
    bool doesWaterVaporize() const { return isHellWorld; }
    bool getHasNoSky() const { return hasNoSky; }
    const std::array<float, 16>& getLightBrightnessTable() const { return lightBrightnessTable; }
    int getDimensionId() const { return dimensionId; }
    virtual std::unique_ptr<WorldBorder> getWorldBorder() const;

protected:
    virtual void generateLightBrightnessTable();
    // Java: also creates the biome source (stage 6); here only the dimension flags.
    virtual void registerWorldChunkManager() { }

    World* worldObj = nullptr;
    bool isHellWorld = false;
    bool hasNoSky = false;
    std::array<float, 16> lightBrightnessTable {};
    int dimensionId = 0;

private:
    const WorldType* terrainType = nullptr;
    std::string generatorSettings;
};

class WorldProviderSurface : public WorldProvider {
public:
    std::string getDimensionName() const override { return "Overworld"; }
    std::string getInternalNameSuffix() const override { return ""; }
};

class WorldProviderHell : public WorldProvider {
public:
    Vec3 getFogColor(float, float) const override { return Vec3(0.20000000298023224, 0.029999999329447746, 0.029999999329447746); }
    bool isSurfaceWorld() const override { return false; }
    bool canCoordinateBeSpawn(int, int) const override { return false; }
    float calculateCelestialAngle(int64_t, float) const override { return 0.5f; }
    bool canRespawnHere() const override { return false; }
    bool doesXZShowFog(int, int) const override { return true; }
    std::string getDimensionName() const override { return "Nether"; }
    std::string getInternalNameSuffix() const override { return "_nether"; }
    std::unique_ptr<WorldBorder> getWorldBorder() const override;

protected:
    void registerWorldChunkManager() override;
    void generateLightBrightnessTable() override;
};

class WorldProviderEnd : public WorldProvider {
public:
    float calculateCelestialAngle(int64_t, float) const override { return 0.0f; }
    std::optional<std::array<float, 4>> calcSunriseSunsetColors(float, float) const override { return std::nullopt; }
    Vec3 getFogColor(float celestialAngle, float partialTicks) const override;
    bool isSkyColored() const override { return false; }
    bool canRespawnHere() const override { return false; }
    bool isSurfaceWorld() const override { return false; }
    float getCloudHeight() const override { return 8.0f; }
    bool canCoordinateBeSpawn(int x, int z) const override;
    std::optional<BlockPos> getSpawnCoordinate() const override { return BlockPos(100, 50, 0); }
    int getAverageGroundLevel() const override { return 50; }
    bool doesXZShowFog(int, int) const override { return true; }
    std::string getDimensionName() const override { return "The End"; }
    std::string getInternalNameSuffix() const override { return "_end"; }

protected:
    void registerWorldChunkManager() override;
};

} // namespace mc

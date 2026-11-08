#pragma once

// Original: net/minecraft/world/World.java
// Ported so far: chunk and block access, block state changes, height and light queries, the light engine
// (checkLight / checkLightFor), sky light from the time of day and weather strength, spawn and world info.
// The rest arrives with the systems it serves: biomes (getBiomeGenForCoords, stage 6), ray tracing and
// collisions (stage 8), ticking, weather, redstone, neighbour notifications to blocks and tile entities
// (stage 9), entities (stage 10), sounds and particles (stages 13 and 15), map storage, villages and the
// scoreboard (stage 12).

#include "profiler/Profiler.h"
#include "util/BlockPos.h"
#include "util/Random.h"
#include "world/EnumSkyBlock.h"

#include <array>
#include <cstdint>
#include <memory>
#include <vector>

namespace mc {

class Block;
class Chunk;
class IBlockState;
class IChunkProvider;
class ISaveHandler;
class IWorldAccess;
class WorldBorder;
class WorldInfo;
class WorldProvider;
class WorldSettings;
class WorldType;

class World {
public:
    virtual ~World();

    World(const World&) = delete;
    World& operator=(const World&) = delete;

    virtual World* init() { return this; }
    virtual void initialize(const WorldSettings& settings);
    virtual void setInitialSpawnLocation() { setSpawnPoint(BlockPos(8, 64, 8)); }
    Block* getGroundAboveSeaLevel(const BlockPos& pos);
    bool isAirBlock(const BlockPos& pos);
    bool isBlockLoaded(const BlockPos& pos, bool allowEmpty = true);
    bool isAreaLoaded(const BlockPos& center, int radius, bool allowEmpty = true);
    bool isAreaLoaded(const BlockPos& from, const BlockPos& to, bool allowEmpty = true);
    Chunk* getChunkFromBlockCoords(const BlockPos& pos);
    Chunk* getChunkFromChunkCoords(int chunkX, int chunkZ);
    // Flags: 1 notify neighbours, 2 send to clients / re-render, 4 (client) no re-render.
    virtual bool setBlockState(const BlockPos& pos, const IBlockState* newState, int flags);
    bool setBlockState(const BlockPos& pos, const IBlockState* state) { return setBlockState(pos, state, 3); }
    bool setBlockToAir(const BlockPos& pos);
    void markBlockForUpdate(const BlockPos& pos);
    void markBlocksDirtyVertical(int x1, int z1, int x2, int z2);
    void markBlockRangeForRenderUpdate(const BlockPos& rangeMin, const BlockPos& rangeMax);
    void markBlockRangeForRenderUpdate(int x1, int y1, int z1, int x2, int y2, int z2);
    bool canSeeSky(const BlockPos& pos);
    bool canBlockSeeSky(const BlockPos& pos);
    int getLight(const BlockPos& pos);
    int getLightFromNeighbors(const BlockPos& pos) { return getLight(pos, true); }
    int getLight(const BlockPos& pos, bool checkNeighbors);
    BlockPos getHeight(const BlockPos& pos);
    int getChunksLowestHorizon(int x, int z);
    int getLightFromNeighborsFor(EnumSkyBlock type, const BlockPos& pos);
    int getLightFor(EnumSkyBlock type, const BlockPos& pos);
    void setLightFor(EnumSkyBlock type, const BlockPos& pos, int lightValue);
    void notifyLightSet(const BlockPos& pos);
    int getCombinedLight(const BlockPos& pos, int lightValue);
    float getLightBrightness(const BlockPos& pos);
    const IBlockState* getBlockState(const BlockPos& pos);
    bool isDaytime() const { return skylightSubtracted < 4; }
    void addWorldAccess(IWorldAccess* worldAccess) { worldAccesses.push_back(worldAccess); }
    void removeWorldAccess(IWorldAccess* worldAccess);
    int calculateSkylightSubtracted(float partialTicks);
    float getCelestialAngle(float partialTicks);
    int getMoonPhase();
    float getCurrentMoonPhaseFactor();
    float getCelestialAngleRadians(float partialTicks);
    BlockPos getPrecipitationHeight(const BlockPos& pos);
    BlockPos getTopSolidOrLiquidBlock(const BlockPos& pos);
    void calculateInitialSkylight();
    bool checkLight(const BlockPos& pos);
    bool checkLightFor(EnumSkyBlock lightType, const BlockPos& pos);
    int getSeaLevel() const { return seaLevel; }
    void setSeaLevel(int value) { seaLevel = value; }
    const WorldType* getWorldType() const;
    void setTotalWorldTime(int64_t worldTime);
    int64_t getSeed() const;
    int64_t getTotalWorldTime() const;
    int64_t getWorldTime() const;
    void setWorldTime(int64_t time);
    BlockPos getSpawnPoint();
    void setSpawnPoint(const BlockPos& pos);
    IChunkProvider* getChunkProvider() const { return chunkProvider.get(); }
    ISaveHandler* getSaveHandler() const { return saveHandler.get(); }
    // Throws MinecraftException when another instance took over the save.
    void checkSessionLock();
    WorldInfo& getWorldInfo() const { return *worldInfo; }
    float getThunderStrength(float delta);
    void setThunderStrength(float strength);
    float getRainStrength(float delta);
    void setRainStrength(float strength);
    bool isThundering();
    bool isRaining();
    int getHeight() const { return 256; }
    int getActualHeight() const;
    Random& setRandomSeed(int x, int y, int z);
    double getHorizon() const;
    int getSkylightSubtracted() const { return skylightSubtracted; }
    void setSkylightSubtracted(int value) { skylightSubtracted = value; }
    WorldBorder& getWorldBorder() const { return *worldBorder; }
    bool isSpawnChunk(int x, int z);

    Random rand;
    const std::unique_ptr<WorldProvider> provider;
    Profiler& theProfiler;
    const bool isRemote;

protected:
    World(std::shared_ptr<ISaveHandler> saveHandlerIn, std::shared_ptr<WorldInfo> info, std::unique_ptr<WorldProvider> providerIn, Profiler& profilerIn,
        bool client);

    virtual std::unique_ptr<IChunkProvider> createChunkProvider() = 0;
    virtual int getRenderDistanceChunks() = 0;
    virtual bool isChunkLoaded(int x, int z, bool allowEmpty);

    int seaLevel = 63;
    int updateLCG;
    float prevRainingStrength = 0.0f;
    float rainingStrength = 0.0f;
    float prevThunderingStrength = 0.0f;
    float thunderingStrength = 0.0f;
    std::vector<IWorldAccess*> worldAccesses;
    std::unique_ptr<IChunkProvider> chunkProvider;
    const std::shared_ptr<ISaveHandler> saveHandler;
    std::shared_ptr<WorldInfo> worldInfo;
    bool findingSpawnPoint = false;

private:
    bool isValid(const BlockPos& pos) const;
    bool isAreaLoaded(int xStart, int yStart, int zStart, int xEnd, int yEnd, int zEnd, bool allowEmpty);
    int getRawLight(const BlockPos& pos, EnumSkyBlock lightType);

    int skylightSubtracted = 0;
    const std::unique_ptr<WorldBorder> worldBorder;
    std::vector<int32_t> lightUpdateBlockList;
};

} // namespace mc

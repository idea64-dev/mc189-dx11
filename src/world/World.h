#pragma once

// Original: net/minecraft/world/World.java
// Ported so far: chunk and block access, block state changes with the block callbacks (neighbour
// notifications, comparators), tile entities, height and light queries, the light engine (checkLight /
// checkLightFor), redstone power queries, freezing and snow checks, biomes, sky light from the time of day and
// weather strength, spawn and world info. Scheduled block ticks and block events are WorldServer's (the base
// versions do nothing or run at once, as in Java).
// The rest arrives with the systems it serves: ray tracing and collisions (stage 8), ticking and weather
// (stage 9), entities (stage 10), map storage, villages and the scoreboard (stage 12).

#include "core/JavaHashMap.h"
#include "profiler/Profiler.h"
#include "util/BlockPos.h"
#include "util/Random.h"
#include "util/UUID.h"
#include "util/Vec3.h"
#include "world/ChunkCoordIntPair.h"
#include "world/EnumSkyBlock.h"
#include "world/IBlockAccess.h"

#include <array>
#include <functional>
#include <optional>
#include <unordered_map>
#include <cstdint>
#include <memory>
#include <string>
#include <vector>

namespace mc {

template <>
struct JavaHash<ChunkCoordIntPair> {
    int32_t operator()(const ChunkCoordIntPair& key) const { return key.hashCode(); }
};

class BiomeGenBase;
class Block;
class Chunk;
class Entity;
class EntityPlayer;
class Material;
class GameRules;
class IBlockState;
class IChunkProvider;
class ISaveHandler;
class IWorldAccess;
class ItemStack;
class MapStorage;
struct NextTickListEntry;
class TileEntity;
class WorldBorder;
class WorldChunkManager;
class WorldInfo;
class WorldProvider;
class WorldSettings;
class WorldType;
enum class EnumDifficulty;

class World : public IBlockAccess {
public:
    ~World() override;

    World(const World&) = delete;
    World& operator=(const World&) = delete;

    virtual World* init() { return this; }
    BiomeGenBase* getBiomeGenForCoords(const BlockPos& pos) override;
    WorldChunkManager* getWorldChunkManager() const;
    virtual void initialize(const WorldSettings& settings);
    virtual void setInitialSpawnLocation() { setSpawnPoint(BlockPos(8, 64, 8)); }
    Block* getGroundAboveSeaLevel(const BlockPos& pos);
    bool isAirBlock(const BlockPos& pos) override;
    bool isBlockLoaded(const BlockPos& pos, bool allowEmpty = true);
    bool isAreaLoaded(const BlockPos& center, int radius, bool allowEmpty = true);
    bool isAreaLoaded(const BlockPos& from, const BlockPos& to, bool allowEmpty = true);
    // Java: isAreaLoaded(StructureBoundingBox, allowEmpty)
    bool isAreaLoaded(int minX, int minY, int minZ, int maxX, int maxY, int maxZ, bool allowEmpty);
    Chunk* getChunkFromBlockCoords(const BlockPos& pos);
    Chunk* getChunkFromChunkCoords(int chunkX, int chunkZ);
    // Flags: 1 notify neighbours, 2 send to clients / re-render, 4 (client) no re-render.
    virtual bool setBlockState(const BlockPos& pos, const IBlockState* newState, int flags);
    bool setBlockState(const BlockPos& pos, const IBlockState* state) { return setBlockState(pos, state, 3); }
    bool setBlockToAir(const BlockPos& pos);
    bool destroyBlock(const BlockPos& pos, bool dropBlock);
    void markBlockForUpdate(const BlockPos& pos);
    void notifyNeighborsRespectDebug(const BlockPos& pos, Block* blockType);
    void markBlocksDirtyVertical(int x1, int z1, int x2, int z2);
    void markBlockRangeForRenderUpdate(const BlockPos& rangeMin, const BlockPos& rangeMax);
    void markBlockRangeForRenderUpdate(int x1, int y1, int z1, int x2, int y2, int z2);
    void notifyNeighborsOfStateChange(const BlockPos& pos, Block* blockType);
    void notifyNeighborsOfStateExcept(const BlockPos& pos, Block* blockType, EnumFacing skipSide);
    void notifyBlockOfStateChange(const BlockPos& pos, Block* blockIn);
    virtual bool isBlockTickPending(const BlockPos&, Block*) { return false; }
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
    int getCombinedLight(const BlockPos& pos, int lightValue) override;
    float getLightBrightness(const BlockPos& pos);
    const IBlockState* getBlockState(const BlockPos& pos) override;
    bool isDaytime() const { return skylightSubtracted < 4; }
    std::optional<MovingObjectPosition> rayTraceBlocks(const Vec3& start, const Vec3& end) { return rayTraceBlocks(start, end, false, false, false); }
    std::optional<MovingObjectPosition> rayTraceBlocks(const Vec3& start, const Vec3& end, bool stopOnLiquid)
    {
        return rayTraceBlocks(start, end, stopOnLiquid, false, false);
    }
    std::optional<MovingObjectPosition> rayTraceBlocks(Vec3 vec31, const Vec3& vec32, bool stopOnLiquid, bool ignoreBlockWithoutBoundingBox,
        bool returnLastUncollidableBlock);
    void playSoundAtEntity(Entity* entityIn, const std::string& name, float volume, float pitch);
    void playSoundToNearExcept(EntityPlayer* player, const std::string& name, float volume, float pitch);
    void playSoundEffect(double x, double y, double z, const std::string& soundName, float volume, float pitch);
    virtual void playSound(double, double, double, const std::string&, float, float, bool) { }
    void playRecord(const BlockPos& pos, const std::string& name);
    // Java: spawnParticle(EnumParticleTypes, ...) -- by particle id here.
    void spawnParticle(int particleID, bool ignoreRange, double xCoord, double yCoord, double zCoord, double xOffset, double yOffset, double zOffset,
        const std::vector<int>& parameters = {});
    virtual bool addWeatherEffect(const std::shared_ptr<Entity>& entityIn);
    virtual bool spawnEntityInWorld(const std::shared_ptr<Entity>& entityIn);
    virtual void removeEntity(Entity* entityIn);
    void removePlayerEntityDangerously(Entity* entityIn);
    void addWorldAccess(IWorldAccess* worldAccess) { worldAccesses.push_back(worldAccess); }
    std::vector<AxisAlignedBB> getCollidingBoundingBoxes(Entity* entityIn, const AxisAlignedBB& bb);
    bool isInsideBorder(const WorldBorder& worldBorderIn, const Entity* entityIn) const;
    std::vector<AxisAlignedBB> getCollisionBoxes(const AxisAlignedBB& bb);
    void removeWorldAccess(IWorldAccess* worldAccess);
    int calculateSkylightSubtracted(float partialTicks);
    float getCelestialAngle(float partialTicks);
    int getMoonPhase();
    float getCurrentMoonPhaseFactor();
    float getCelestialAngleRadians(float partialTicks);
    BlockPos getPrecipitationHeight(const BlockPos& pos);
    BlockPos getTopSolidOrLiquidBlock(const BlockPos& pos);
    virtual void scheduleUpdate(const BlockPos&, Block*, int) { }
    virtual void updateBlockTick(const BlockPos&, Block*, int, int) { }
    virtual void scheduleBlockUpdate(const BlockPos&, Block*, int, int) { }
    // Whether the world keeps scheduled ticks (WorldServer); the others leave a chunk's TileTicks as loaded.
    virtual bool hasScheduledTicks() const { return false; }
    // Java: tickUpdates -- WorldServer runs the due scheduled ticks; false here.
    virtual bool tickUpdates(bool) { return false; }
    // Java: getPendingBlockUpdates -- the scheduled ticks inside the chunk (removing them when asked); none here.
    virtual std::vector<NextTickListEntry> getPendingBlockUpdates(Chunk* chunkIn, bool remove);
    // Java: tick() -- the weather here; WorldServer and WorldClient add the rest.
    virtual void tick();
    bool addTileEntity(const std::shared_ptr<TileEntity>& tile);
    void addTileEntities(const std::vector<std::shared_ptr<TileEntity>>& tileEntityCollection);
    // Java: updateEntities() -- here the tile entity part; entities come with stage 10.
    virtual void updateEntities();
    void updateEntity(Entity* ent) { updateEntityWithOptionalForce(ent, true); }
    virtual void updateEntityWithOptionalForce(Entity* entityIn, bool forceUpdate);
    bool checkNoEntityCollision(const AxisAlignedBB& bb, const Entity* entityIn = nullptr);
    bool checkBlockCollision(const AxisAlignedBB& bb);
    bool isAnyLiquid(const AxisAlignedBB& bb);
    bool isFlammableWithin(const AxisAlignedBB& bb);
    bool handleMaterialAcceleration(const AxisAlignedBB& bb, const Material& materialIn, Entity* entityIn);
    bool isMaterialInBB(const AxisAlignedBB& bb, const Material& materialIn);
    bool isAABBInMaterial(const AxisAlignedBB& bb, const Material& materialIn);
    TileEntity* getTileEntity(const BlockPos& pos) override;
    std::shared_ptr<TileEntity> getTileEntityShared(const BlockPos& pos);
    void setTileEntity(const BlockPos& pos, std::shared_ptr<TileEntity> tileEntityIn);
    void removeTileEntity(const BlockPos& pos);
    void markTileEntityForRemoval(const std::shared_ptr<TileEntity>& tileEntityIn);
    bool isBlockFullCube(const BlockPos& pos);
    static bool doesBlockHaveSolidTopSurface(IBlockAccess* blockAccess, const BlockPos& pos);
    bool isBlockNormalCube(const BlockPos& pos, bool defaultValue);
    void calculateInitialSkylight();
    void forceBlockUpdateTick(Block* blockType, const BlockPos& pos, Random& random);
    bool canBlockFreezeWater(const BlockPos& pos) { return canBlockFreeze(pos, false); }
    bool canBlockFreezeNoWater(const BlockPos& pos) { return canBlockFreeze(pos, true); }
    bool canBlockFreeze(const BlockPos& pos, bool noWaterAdj);
    bool canSnowAt(const BlockPos& pos, bool checkLight);
    bool checkLight(const BlockPos& pos);
    bool checkLightFor(EnumSkyBlock lightType, const BlockPos& pos);
    void markChunkDirty(const BlockPos& pos, TileEntity* unusedTileEntity);
    // The entities of the loaded chunks whose box meets bb, except entityIn; Java's predicate defaults to
    // EntitySelectors.NOT_SPECTATING.
    std::vector<Entity*> getEntitiesWithinAABBExcludingEntity(const Entity* entityIn, const AxisAlignedBB& bb);
    std::vector<Entity*> getEntitiesInAABBexcluding(const Entity* entityIn, const AxisAlignedBB& boundingBox, const std::function<bool(Entity*)>& predicate);
    // Java: getEntitiesWithinAABB(Class, aabb[, filter]) -- the entities castable to T.
    template <class T>
    std::vector<T*> getEntitiesWithinAABB(const AxisAlignedBB& aabb, const std::function<bool(T*)>& filter = {});
    virtual Entity* getEntityByID(int id);
    const std::vector<std::shared_ptr<Entity>>& getLoadedEntityList() const { return loadedEntityList; }
    void loadEntities(const std::vector<std::shared_ptr<Entity>>& entityCollection);
    void unloadEntities(const std::vector<std::shared_ptr<Entity>>& entityCollection);
    bool canBlockBePlaced(Block* blockIn, const BlockPos& pos, bool skipCollisionCheck, EnumFacing side, Entity* entityIn, const ItemStack* itemStackIn);
    EntityPlayer* getClosestPlayerToEntity(const Entity* entityIn, double distance);
    EntityPlayer* getClosestPlayer(double x, double y, double z, double distance);
    bool isAnyPlayerWithinRangeAt(double x, double y, double z, double range);
    EntityPlayer* getPlayerEntityByName(const std::string& name);
    EntityPlayer* getPlayerEntityByUUID(const UUID& uuid);
    void joinEntityInSurroundings(const std::shared_ptr<Entity>& entityIn);
    virtual bool isBlockModifiable(EntityPlayer*, const BlockPos&) { return true; }
    virtual void setEntityState(Entity*, int8_t) { }
    virtual void updateAllPlayersSleepingFlag() { }
    // The players as shared pointers, in join order.
    const std::vector<std::shared_ptr<EntityPlayer>>& getPlayerEntities() const { return playerEntities; }
    int getSeaLevel() const { return seaLevel; }
    void setSeaLevel(int value) { seaLevel = value; }
    int getStrongPower(const BlockPos& pos, EnumFacing direction) override;
    const WorldType* getWorldType() const override;
    int getStrongPower(const BlockPos& pos);
    bool isSidePowered(const BlockPos& pos, EnumFacing side) { return getRedstonePower(pos, side) > 0; }
    int getRedstonePower(const BlockPos& pos, EnumFacing facing);
    bool isBlockPowered(const BlockPos& pos);
    int isBlockIndirectlyGettingPowered(const BlockPos& pos);
    void setTotalWorldTime(int64_t worldTime);
    int64_t getSeed() const;
    int64_t getTotalWorldTime() const;
    int64_t getWorldTime() const;
    void setWorldTime(int64_t time);
    BlockPos getSpawnPoint();
    void setSpawnPoint(const BlockPos& pos);
    IChunkProvider* getChunkProvider() const { return chunkProvider.get(); }
    // Java: the base runs the event at once; WorldServer queues it for the end of the tick.
    virtual void addBlockEvent(const BlockPos& pos, Block* blockIn, int eventID, int eventParam);
    ISaveHandler* getSaveHandler() const { return saveHandler.get(); }
    // Throws MinecraftException when another instance took over the save.
    void checkSessionLock();
    WorldInfo& getWorldInfo() const { return *worldInfo; }
    GameRules& getGameRules() const;
    float getThunderStrength(float delta);
    void setThunderStrength(float strength);
    float getRainStrength(float delta);
    void setRainStrength(float strength);
    bool isThundering();
    bool isRaining();
    bool isRainingAt(const BlockPos& strikePosition);
    bool isBlockinHighHumidity(const BlockPos& pos);
    void playBroadcastSound(int soundID, const BlockPos& pos, int data);
    void playAuxSFX(int sfxType, const BlockPos& pos, int data) { playAuxSFXAtEntity(nullptr, sfxType, pos, data); }
    void playAuxSFXAtEntity(EntityPlayer* player, int sfxType, const BlockPos& pos, int data);
    // Puts out a fire on the clicked side; true if there was one.
    bool extinguishFire(EntityPlayer* player, const BlockPos& pos, EnumFacing side);
    int getHeight() const { return 256; }
    int getActualHeight() const;
    Random& setRandomSeed(int x, int y, int z);
    bool extendedLevelsInChunkCache() override { return false; }
    double getHorizon() const;
    void sendBlockBreakProgress(int breakerId, const BlockPos& pos, int progress);
    void updateComparatorOutputLevel(const BlockPos& pos, Block* blockIn);
    EnumDifficulty getDifficulty() const;
    int getSkylightSubtracted() const { return skylightSubtracted; }
    void setSkylightSubtracted(int value) { skylightSubtracted = value; }
    bool isFindingSpawnPoint() const { return findingSpawnPoint; }
    WorldBorder& getWorldBorder() const { return *worldBorder; }
    bool isSpawnChunk(int x, int z);
    // Client sky and fog colours (Java: @SideOnly(CLIENT)), in world/WorldSky.cpp.
    Vec3 getSkyColor(const Entity* entityIn, float partialTicks);
    float getSunBrightness(float partialTicks);
    Vec3 getCloudColour(float partialTicks);
    Vec3 getFogColor(float partialTicks);
    float getStarBrightness(float partialTicks);
    int getLastLightningBolt() const { return lastLightningBolt; }
    void setLastLightningBolt(int lastLightningBoltIn) { lastLightningBolt = lastLightningBoltIn; }
    // Java: getMapStorage -- the data/*.dat files (structures, villages, maps); null on the client.
    MapStorage* getMapStorage() const { return mapStorage.get(); }
    // Java: setAllowedSpawnTypes -- which mobs the spawner may make.
    void setAllowedSpawnTypes(bool hostile, bool peaceful)
    {
        spawnHostileMobs = hostile;
        spawnPeacefulMobs = peaceful;
    }

    Random rand;
    const std::unique_ptr<WorldProvider> provider;
    Profiler& theProfiler;
    const bool isRemote;
    // Set while forceBlockUpdateTick runs: WorldServer then ticks scheduled updates at once.
    bool scheduledUpdatesAreImmediate = false;
    bool spawnHostileMobs = true;
    bool spawnPeacefulMobs = true;

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
    int lastLightningBolt = 0;
    // Java: cloudColour, 0xFFFFFF.
    int64_t cloudColour = 16777215LL;
    // Shared between the dimensions of a server, as Java's reference.
    std::shared_ptr<MapStorage> mapStorage;
    void calculateInitialWeather();
    virtual void updateWeather();
    virtual void updateBlocks() { setActivePlayerChunksAndCheckLight(); }
    void setActivePlayerChunksAndCheckLight();
    void playMoodSoundAndCheckLight(int x, int z, Chunk* chunkIn);
    // Java: a HashSet, iterated in its order by the block ticks.
    JavaHashMap<ChunkCoordIntPair, bool> activeChunkSet;
    int ambientTickCountdown;
    virtual void onEntityAdded(Entity* entityIn);
    virtual void onEntityRemoved(Entity* entityIn);

    std::vector<std::shared_ptr<Entity>> loadedEntityList;
    std::vector<std::shared_ptr<Entity>> unloadedEntityList;
    std::vector<std::shared_ptr<EntityPlayer>> playerEntities;
    std::vector<std::shared_ptr<Entity>> weatherEffects;
    // Java: entitiesById, an IntHashMap kept by WorldServer and WorldClient.
    std::unordered_map<int, Entity*> entitiesById;
    std::vector<std::shared_ptr<TileEntity>> loadedTileEntityList;
    std::vector<std::shared_ptr<TileEntity>> tickableTileEntities;

private:
    bool isValid(const BlockPos& pos) const;
    int getRawLight(const BlockPos& pos, EnumSkyBlock lightType);
    bool isWater(const BlockPos& pos);
    // Java: EntitySelectors.NOT_SPECTATING
    static bool notSpectating(Entity* entity);

    int skylightSubtracted = 0;
    const std::unique_ptr<WorldBorder> worldBorder;
    std::vector<int32_t> lightUpdateBlockList;
    std::vector<std::shared_ptr<TileEntity>> addedTileEntityList;
    std::vector<std::shared_ptr<TileEntity>> tileEntitiesToBeRemoved;
    bool processingLoadedTiles = false;
};

template <class T>
std::vector<T*> World::getEntitiesWithinAABB(const AxisAlignedBB& aabb, const std::function<bool(T*)>& filter)
{
    std::vector<T*> list;
    for (Entity* entity : getEntitiesInAABBexcluding(nullptr, aabb, {})) {
        T* t = dynamic_cast<T*>(entity);
        if (t && (filter ? filter(t) : notSpectating(entity)))
            list.push_back(t);
    }
    return list;
}

} // namespace mc

#pragma once

// Original: net/minecraft/world/chunk/Chunk.java, EmptyChunk.java and ChunkPrimer.java
// Ported so far: block storage, height maps, sky and block light (with the world's light engine), biome ids,
// population flags and the network fill (fillChunk).
// Entities exist from stage 10 and scheduled ticks with WorldServer; until then a chunk keeps their NBT as it
// was loaded so that saving writes it back unchanged (getPendingEntityNBT / getPendingTileTickNBT).

#include "core/JavaHashMap.h"
#include "util/BlockPos.h"
#include "util/Vec3.h"
#include "util/Random.h"
#include "world/ChunkCoordIntPair.h"
#include "world/EnumSkyBlock.h"
#include "world/chunk/storage/ExtendedBlockStorage.h"

#include <array>
#include <functional>
#include <cstdint>
#include <memory>
#include <span>
#include <vector>

namespace mc {

class BiomeGenBase;
class Block;
class Entity;
class IBlockState;
class IChunkProvider;
class NBTTagCompound;
class TileEntity;
class World;
class WorldChunkManager;

template <>
struct JavaHash<BlockPos> {
    int32_t operator()(const BlockPos& pos) const { return pos.hashCode(); }
};

class ChunkPrimer {
public:
    ChunkPrimer() { data.fill(0); }
    const IBlockState* getBlockState(int x, int y, int z) const { return getBlockState(x << 12 | z << 8 | y); }
    // Throws std::out_of_range outside the chunk.
    const IBlockState* getBlockState(int index) const;
    void setBlockState(int x, int y, int z, const IBlockState* state) { setBlockState(x << 12 | z << 8 | y, state); }
    void setBlockState(int index, const IBlockState* state);

private:
    std::array<int16_t, 65536> data;
};

class Chunk {
public:
    enum class EnumCreateEntityType { IMMEDIATE, QUEUED, CHECK };
    using TileEntityMap = JavaHashMap<BlockPos, std::shared_ptr<TileEntity>>;

    Chunk(World* worldIn, int x, int z);
    Chunk(World* worldIn, const ChunkPrimer& primer, int x, int z);
    virtual ~Chunk();

    Chunk(const Chunk&) = delete;
    Chunk& operator=(const Chunk&) = delete;

    virtual bool isAtLocation(int x, int z) const { return x == xPosition && z == zPosition; }
    int getHeight(const BlockPos& pos) const { return getHeightValue(pos.getX() & 15, pos.getZ() & 15); }
    virtual int getHeightValue(int x, int z) const { return heightMap[static_cast<size_t>(z << 4 | x)]; }
    int getTopFilledSegment() const;
    std::array<std::unique_ptr<ExtendedBlockStorage>, 16>& getBlockStorageArray() { return storageArrays; }
    virtual void generateHeightMap();
    virtual void generateSkylightMap();
    virtual int getBlockLightOpacity(const BlockPos& pos) const;
    Block* getBlock(int x, int y, int z) const { return getBlock0(x & 15, y, z & 15); }
    virtual Block* getBlock(const BlockPos& pos) const;
    const IBlockState* getBlockState(const BlockPos& pos) const;
    virtual int getBlockMetadata(const BlockPos& pos) const;
    // The previous state, or null when nothing changed.
    const IBlockState* setBlockState(const BlockPos& pos, const IBlockState* state);
    virtual int getLightFor(EnumSkyBlock type, const BlockPos& pos) const;
    virtual void setLightFor(EnumSkyBlock type, const BlockPos& pos, int value);
    virtual int getLightSubtracted(const BlockPos& pos, int amount) const;
    virtual void addEntity(const std::shared_ptr<Entity>& entityIn);
    virtual void removeEntity(Entity* entityIn);
    virtual void removeEntityAtIndex(Entity* entityIn, int index);
    virtual void getEntitiesWithinAABBForEntity(const Entity* entityIn, const AxisAlignedBB& aabb, std::vector<Entity*>& listToFill,
        const std::function<bool(Entity*)>& predicate);
    // Java: ClassInheritanceMultiMap[16], here plain lists in insertion order (the class index only speeds up
    // getEntitiesOfTypeWithinAAAB).
    std::array<std::vector<std::shared_ptr<Entity>>, 16>& getEntityLists() { return entityLists; }
    virtual bool canSeeSky(const BlockPos& pos) const;
    virtual std::shared_ptr<TileEntity> getTileEntity(const BlockPos& pos, EnumCreateEntityType type);
    virtual void addTileEntity(const std::shared_ptr<TileEntity>& tileEntityIn);
    virtual void addTileEntity(const BlockPos& pos, const std::shared_ptr<TileEntity>& tileEntityIn);
    virtual void removeTileEntity(const BlockPos& pos);
    virtual void onChunkLoad();
    virtual void onChunkUnload();
    virtual void setChunkModified() { isModified = true; }
    virtual bool needsSaving(bool saveAll) const;
    virtual Random getRandomWithSeed(int64_t seed) const;
    virtual bool isEmpty() const { return false; }
    void populateChunk(IChunkProvider* provider1, IChunkProvider* provider2, int x, int z);
    BlockPos getPrecipitationHeight(const BlockPos& pos);
    // Java: func_150804_b -- per tick: gap lighting, light population, queued tile entities.
    void func_150804_b(bool skipRecheckGaps);
    bool isPopulated() const { return field_150815_m && terrainPopulated && lightPopulated; }
    ChunkCoordIntPair getChunkCoordIntPair() const { return ChunkCoordIntPair(xPosition, zPosition); }
    virtual bool getAreLevelsEmpty(int startY, int endY) const;
    void setStorageArrays(std::array<std::unique_ptr<ExtendedBlockStorage>, 16> newStorageArrays);
    // Java: fillChunk(byte[], int, boolean) -- a chunk sent by the server (S21PacketChunkData).
    void fillChunk(std::span<const uint8_t> data, int mask, bool fullChunk);
    // Unset entries (255) are filled from the biome source.
    BiomeGenBase* getBiome(const BlockPos& pos, WorldChunkManager* chunkManager);
    // The raw id, 255 for unset.
    int getBiomeId(const BlockPos& pos) const { return blockBiomeArray[static_cast<size_t>((pos.getZ() & 15) << 4 | (pos.getX() & 15))]; }
    std::array<uint8_t, 256>& getBiomeArray() { return blockBiomeArray; }
    void setBiomeArray(std::span<const uint8_t> biomeArray);
    void resetRelightChecks() { queuedLightChecks = 0; }
    void enqueueRelightChecks();
    // Java: func_150809_p -- populates the light of a freshly generated chunk.
    void func_150809_p();
    bool isLoaded() const { return isChunkLoaded; }
    void setChunkLoaded(bool loaded) { isChunkLoaded = loaded; }
    World* getWorld() const { return worldObj; }
    std::array<int, 256>& getHeightMap() { return heightMap; }
    void setHeightMap(std::span<const int> newHeightMap);
    bool isTerrainPopulated() const { return terrainPopulated; }
    void setTerrainPopulated(bool value) { terrainPopulated = value; }
    bool isLightPopulated() const { return lightPopulated; }
    void setLightPopulated(bool value) { lightPopulated = value; }
    void setModified(bool modified) { isModified = modified; }
    void setHasEntities(bool value) { hasEntities = value; }
    void setLastSaveTime(int64_t saveTime) { lastSaveTime = saveTime; }
    int getLowestHeight() const { return heightMapMinimum; }
    int64_t getInhabitedTime() const { return inhabitedTime; }
    void setInhabitedTime(int64_t value) { inhabitedTime = value; }

    TileEntityMap& getTileEntityMap() { return chunkTileEntityMap; }
    // The Entities and TileTicks lists as loaded, kept until those systems are ported.
    std::vector<std::shared_ptr<NBTTagCompound>>& getPendingEntityNBT() { return pendingEntities; }
    std::vector<std::shared_ptr<NBTTagCompound>>& getPendingTileTickNBT() { return pendingTileTicks; }

    const int xPosition;
    const int zPosition;

private:
    void propagateSkylightOcclusion(int x, int z);
    void recheckGaps(bool onlyOne);
    void checkSkylightNeighborHeight(int x, int z, int maxValue);
    void updateSkylightNeighborHeight(int x, int z, int startY, int endY);
    void relightBlock(int x, int y, int z);
    int getBlockLightOpacity(int x, int y, int z) const;
    Block* getBlock0(int x, int y, int z) const;
    int getBlockMetadata(int x, int y, int z) const;
    void func_177441_y();
    void func_180700_a(EnumFacing facing);
    bool func_150811_f(int x, int z);

    std::array<std::unique_ptr<ExtendedBlockStorage>, 16> storageArrays;
    std::array<uint8_t, 256> blockBiomeArray;
    std::array<int, 256> precipitationHeightMap;
    std::array<bool, 256> updateSkylightColumns {};
    bool isChunkLoaded = false;
    World* const worldObj;
    std::array<int, 256> heightMap {};
    bool isGapLightingUpdated = false;
    bool terrainPopulated = false;
    bool lightPopulated = false;
    bool field_150815_m = false;
    bool isModified = false;
    bool hasEntities = false;
    int64_t lastSaveTime = 0;
    int heightMapMinimum = 0;
    int64_t inhabitedTime = 0;
    int queuedLightChecks = 4096;
    std::shared_ptr<TileEntity> createNewTileEntity(const BlockPos& pos);

    TileEntityMap chunkTileEntityMap;
    std::vector<BlockPos> tileEntityPosQueue;
    std::array<std::vector<std::shared_ptr<Entity>>, 16> entityLists;
    std::vector<std::shared_ptr<NBTTagCompound>> pendingEntities;
    std::vector<std::shared_ptr<NBTTagCompound>> pendingTileTicks;
};

class EmptyChunk : public Chunk {
public:
    EmptyChunk(World* worldIn, int x, int z)
        : Chunk(worldIn, x, z)
    {
    }

    int getHeightValue(int, int) const override { return 0; }
    void generateHeightMap() override { }
    void generateSkylightMap() override { }
    Block* getBlock(const BlockPos& pos) const override;
    int getBlockLightOpacity(const BlockPos&) const override { return 255; }
    int getBlockMetadata(const BlockPos&) const override { return 0; }
    int getLightFor(EnumSkyBlock type, const BlockPos&) const override { return defaultLightValue(type); }
    void setLightFor(EnumSkyBlock, const BlockPos&, int) override { }
    int getLightSubtracted(const BlockPos&, int) const override { return 0; }
    // Java: entities and tile entities are dropped silently (the client hands this chunk out for chunks it
    // has not received yet, the player spawns into one).
    void addEntity(const std::shared_ptr<Entity>&) override { }
    void removeEntity(Entity*) override { }
    void removeEntityAtIndex(Entity*, int) override { }
    void getEntitiesWithinAABBForEntity(const Entity*, const AxisAlignedBB&, std::vector<Entity*>&, const std::function<bool(Entity*)>&) override { }
    bool canSeeSky(const BlockPos&) const override { return false; }
    std::shared_ptr<TileEntity> getTileEntity(const BlockPos&, EnumCreateEntityType) override { return nullptr; }
    void addTileEntity(const std::shared_ptr<TileEntity>&) override { }
    void addTileEntity(const BlockPos&, const std::shared_ptr<TileEntity>&) override { }
    void removeTileEntity(const BlockPos&) override { }
    void onChunkLoad() override { }
    void onChunkUnload() override { }
    void setChunkModified() override { }
    bool needsSaving(bool) const override { return false; }
    bool isEmpty() const override { return true; }
    bool getAreLevelsEmpty(int, int) const override { return true; }
};

} // namespace mc

#pragma once

// Original: net/minecraft/world/chunk/Chunk.java, EmptyChunk.java and ChunkPrimer.java
// Ported so far: block storage, height maps, sky and block light (with the world's light engine), biome ids,
// population flags and the network fill (fillChunk).
// Tile entities and entities exist from stages 9 and 10; until then a chunk keeps their NBT as it was loaded so
// that saving writes it back unchanged (getPendingTileEntityNBT / getPendingEntityNBT). The block callbacks of
// setBlockState (breakBlock, onBlockAdded, tile entity creation) come with them as well.

#include "util/BlockPos.h"
#include "util/Random.h"
#include "world/ChunkCoordIntPair.h"
#include "world/EnumSkyBlock.h"
#include "world/chunk/storage/ExtendedBlockStorage.h"

#include <array>
#include <cstdint>
#include <memory>
#include <span>
#include <vector>

namespace mc {

class Block;
class IBlockState;
class IChunkProvider;
class NBTTagCompound;
class World;

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
    // Java: addEntity, removeEntity, removeEntityAtIndex, getEntitiesWithinAABBForEntity,
    // getEntitiesOfTypeWithinAAAB -- entities, stage 10.
    virtual bool canSeeSky(const BlockPos& pos) const;
    // Java: getTileEntity, addTileEntity, removeTileEntity -- tile entities, stage 9.
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
    // Java: getBiome(pos, WorldChunkManager) asks the biome source for unset entries -- stage 6. Here the raw id,
    // 255 for unset.
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

    // The Entities, TileEntities and TileTicks lists as loaded, kept until those systems are ported.
    std::vector<std::shared_ptr<NBTTagCompound>>& getPendingEntityNBT() { return pendingEntities; }
    std::vector<std::shared_ptr<NBTTagCompound>>& getPendingTileEntityNBT() { return pendingTileEntities; }
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
    std::vector<std::shared_ptr<NBTTagCompound>> pendingEntities;
    std::vector<std::shared_ptr<NBTTagCompound>> pendingTileEntities;
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
    bool canSeeSky(const BlockPos&) const override { return false; }
    void onChunkLoad() override { }
    void onChunkUnload() override { }
    void setChunkModified() override { }
    bool needsSaving(bool) const override { return false; }
    bool isEmpty() const override { return true; }
    bool getAreLevelsEmpty(int, int) const override { return true; }
};

} // namespace mc

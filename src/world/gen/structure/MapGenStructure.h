#pragma once

// Original: net/minecraft/world/gen/structure/MapGenStructure.java, MapGenStructureData.java and
// MapGenStructureIO.java -- the structure generators: starts are chosen in the carver pass of provideChunk
// (recursiveGenerate) and built chunk by chunk in populate (generateStructure); the starts are kept in
// data/<name>.dat.
// structureMap is a HashMap<Long, StructureStart> in Java and generateStructure iterates it, so the order of
// overlapping structures follows Java's bucket order (JavaHashMap).

#include "core/JavaHashMap.h"
#include "util/BlockPos.h"
#include "world/ChunkCoordIntPair.h"
#include "world/gen/MapGenBase.h"
#include "world/gen/structure/StructureComponent.h"
#include "world/storage/MapStorage.h"

#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <vector>

namespace mc {

class NBTTagCompound;

class MapGenStructureData : public WorldSavedData {
public:
    explicit MapGenStructureData(std::string name);

    void readFromNBT(const NBTTagCompound& nbt) override;
    void writeToNBT(NBTTagCompound& nbt) const override;
    void writeInstance(std::shared_ptr<NBTTagCompound> tagCompoundIn, int chunkX, int chunkZ);
    static std::string formatChunkCoords(int chunkX, int chunkZ) { return "[" + std::to_string(chunkX) + "," + std::to_string(chunkZ) + "]"; }
    const NBTTagCompound& getTagCompound() const { return *tagCompound; }

private:
    std::shared_ptr<NBTTagCompound> tagCompound;
};

class MapGenStructureIO {
public:
    using StartFactory = std::function<std::shared_ptr<StructureStart>()>;
    using ComponentFactory = std::function<std::shared_ptr<StructureComponent>()>;

    // The static initializer: the starts and every piece class; idempotent.
    static void registerAll();
    static void registerStructure(const std::string& structureName, StartFactory factory);
    static void registerStructureComponent(const std::string& componentName, ComponentFactory factory);
    // Null (with a warning) for unknown ids.
    static std::shared_ptr<StructureStart> getStructureStart(const NBTTagCompound& tagCompound, World* worldIn);
    static std::shared_ptr<StructureComponent> getStructureComponent(const NBTTagCompound& tagCompound, World* worldIn);
};

class MapGenStructure : public MapGenBase {
public:
    virtual std::string getStructureName() const = 0;
    bool generateStructure(World* worldIn, Random& randomIn, const ChunkCoordIntPair& chunkCoord);
    bool func_175795_b(const BlockPos& pos);
    bool isPositionInStructure(World* worldIn, const BlockPos& pos);
    // Java returns null when there is none.
    std::optional<BlockPos> getClosestStrongholdPos(World* worldIn, const BlockPos& pos);

protected:
    void recursiveGenerate(World* worldIn, int chunkX, int chunkZ, int originX, int originZ, ChunkPrimer* chunkPrimerIn) final;
    StructureStart* func_175797_c(const BlockPos& pos);
    // Java returns null by default; the stronghold lists its positions.
    virtual std::optional<std::vector<BlockPos>> getCoordList() { return std::nullopt; }
    virtual bool canSpawnStructureAtCoords(int chunkX, int chunkZ) = 0;
    virtual std::shared_ptr<StructureStart> getStructureStart(int chunkX, int chunkZ) = 0;

    JavaHashMap<int64_t, std::shared_ptr<StructureStart>> structureMap;

private:
    void initializeStructureData(World* worldIn);
    void setStructureStart(int chunkX, int chunkZ, const StructureStart& start);

    std::shared_ptr<MapGenStructureData> structureData;
};

} // namespace mc

#pragma once

// Original: net/minecraft/world/gen/structure/StructureComponent.java and StructureStart.java -- the pieces of a
// structure and the set of pieces grown from one start chunk.
// Pieces are shared (std::shared_ptr): the start owns them, and starts like the village and stronghold keep
// extra lists of the same pieces while they grow. Java's nullable coordBaseMode is an optional here.

#include "util/EnumFacing.h"
#include "util/WeightedRandomChestContent.h"
#include "world/ChunkCoordIntPair.h"
#include "world/gen/structure/StructureBoundingBox.h"

#include <memory>
#include <optional>
#include <string_view>
#include <vector>

namespace mc {

class Block;
class Material;
class IBlockState;
class NBTTagCompound;
class Random;
class World;
class StructureComponent;

using StructureComponentList = std::vector<std::shared_ptr<StructureComponent>>;

class StructureComponent {
public:
    // Java: StructureComponent.BlockSelector
    class BlockSelector {
    public:
        BlockSelector();
        virtual ~BlockSelector() = default;
        virtual void selectBlocks(Random& rand, int x, int y, int z, bool wall) = 0;
        const IBlockState* getBlockState() const;

    protected:
        // Java: Blocks.air.getDefaultState(); null until set, as the selectors are statics built before the
        // blocks exist.
        const IBlockState* blockstate = nullptr;
    };

    StructureComponent() = default;
    explicit StructureComponent(int type)
        : componentType(type)
    {
    }
    virtual ~StructureComponent() = default;

    std::shared_ptr<NBTTagCompound> createStructureBaseNBT() const;
    void readStructureBaseNBT(World* worldIn, const NBTTagCompound& tagCompound);
    // componentIn is the structure's first piece (Java passes it as a StructureComponent).
    virtual void buildComponent(StructureComponent* componentIn, StructureComponentList& listIn, Random& rand);
    virtual bool addComponentParts(World* worldIn, Random& randomIn, const StructureBoundingBox& structureBoundingBoxIn) = 0;
    const StructureBoundingBox& getBoundingBox() const { return boundingBox; }
    int getComponentType() const { return componentType; }
    // Null when none intersects.
    static std::shared_ptr<StructureComponent> findIntersecting(const StructureComponentList& listIn, const StructureBoundingBox& boundingboxIn);
    // Virtual: the stronghold start reports its portal room.
    virtual BlockPos getBoundingBoxCenter() const { return BlockPos(boundingBox.getCenter()); }
    virtual void func_181138_a(int x, int y, int z) { boundingBox.offset(x, y, z); }
    // The MapGenStructureIO id (Java: componentClassToNameMap.get(getClass())).
    virtual std::string_view getStructureName() const = 0;

protected:
    // Java: state.getBlock().getMaterial() == material
    static bool isMaterial(const IBlockState* state, const Material& material);
    // Java: this.coordBaseMode == facing (false while it is null).
    bool isMode(EnumFacing facing) const { return coordBaseMode && *coordBaseMode == facing; }
    virtual void writeStructureToNBT(NBTTagCompound& tagCompound) const = 0;
    virtual void readStructureFromNBT(const NBTTagCompound& tagCompound) = 0;
    bool isLiquidInStructureBoundingBox(World* worldIn, const StructureBoundingBox& boundingboxIn) const;
    int getXWithOffset(int x, int z) const;
    int getYWithOffset(int y) const;
    int getZWithOffset(int x, int z) const;
    int getMetadataWithOffset(const Block* blockIn, int meta) const;
    // Virtual like Java's: the village pieces swap in their desert blocks.
    virtual void setBlockState(World* worldIn, const IBlockState* blockstateIn, int x, int y, int z, const StructureBoundingBox& boundingboxIn);
    const IBlockState* getBlockStateFromPos(World* worldIn, int x, int y, int z, const StructureBoundingBox& boundingboxIn) const;
    void fillWithAir(World* worldIn, const StructureBoundingBox& structurebb, int minX, int minY, int minZ, int maxX, int maxY, int maxZ);
    virtual void fillWithBlocks(World* worldIn, const StructureBoundingBox& boundingboxIn, int xMin, int yMin, int zMin, int xMax, int yMax, int zMax,
        const IBlockState* boundaryBlockState, const IBlockState* insideBlockState, bool existingOnly);
    void fillWithRandomizedBlocks(World* worldIn, const StructureBoundingBox& boundingboxIn, int minX, int minY, int minZ, int maxX, int maxY, int maxZ,
        bool alwaysReplace, Random& rand, BlockSelector& blockselector);
    void func_175805_a(World* worldIn, const StructureBoundingBox& boundingboxIn, Random& rand, float chance, int minX, int minY, int minZ, int maxX, int maxY,
        int maxZ, const IBlockState* blockstate1, const IBlockState* blockstate2, bool existingOnly);
    void randomlyPlaceBlock(World* worldIn, const StructureBoundingBox& boundingboxIn, Random& rand, float chance, int x, int y, int z,
        const IBlockState* blockstateIn);
    void randomlyRareFillWithBlocks(World* worldIn, const StructureBoundingBox& boundingboxIn, int minX, int minY, int minZ, int maxX, int maxY, int maxZ,
        const IBlockState* blockstateIn, bool existingOnly);
    void clearCurrentPositionBlocksUpwards(World* worldIn, int x, int y, int z, const StructureBoundingBox& structurebb);
    virtual void replaceAirAndLiquidDownwards(World* worldIn, const IBlockState* blockstateIn, int x, int y, int z, const StructureBoundingBox& boundingboxIn);
    bool generateChestContents(World* worldIn, const StructureBoundingBox& boundingBoxIn, Random& rand, int x, int y, int z,
        const std::vector<WeightedRandomChestContent>& listIn, int max);
    bool generateDispenserContents(World* worldIn, const StructureBoundingBox& boundingBoxIn, Random& rand, int x, int y, int z, int meta,
        const std::vector<WeightedRandomChestContent>& listIn, int max);
    void placeDoorCurrentPosition(World* worldIn, const StructureBoundingBox& boundingBoxIn, Random& rand, int x, int y, int z, EnumFacing facing);

    StructureBoundingBox boundingBox;
    std::optional<EnumFacing> coordBaseMode;
    int componentType = 0;
};

class StructureStart {
public:
    StructureStart() = default;
    StructureStart(int chunkX, int chunkZ)
        : chunkPosX(chunkX)
        , chunkPosZ(chunkZ)
    {
    }
    virtual ~StructureStart() = default;

    const StructureBoundingBox& getBoundingBox() const { return boundingBox; }
    StructureComponentList& getComponents() { return components; }
    virtual void generateStructure(World* worldIn, Random& rand, const StructureBoundingBox& structurebb);
    std::shared_ptr<NBTTagCompound> writeStructureComponentsToNBT(int chunkX, int chunkZ) const;
    virtual void writeToNBT(NBTTagCompound&) const { }
    void readStructureComponentsFromNBT(World* worldIn, const NBTTagCompound& tagCompound);
    virtual void readFromNBT(const NBTTagCompound&) { }
    virtual bool isSizeableStructure() const { return true; }
    virtual bool func_175788_a(const ChunkCoordIntPair&) { return true; }
    virtual void func_175787_b(const ChunkCoordIntPair&) { }
    int getChunkPosX() const { return chunkPosX; }
    int getChunkPosZ() const { return chunkPosZ; }
    // The MapGenStructureIO id (Java: startClassToNameMap.get(getClass())).
    virtual std::string_view getStructureName() const = 0;

protected:
    void updateBoundingBox();
    void markAvailableHeight(World* worldIn, Random& rand, int minDepth);
    void setRandomHeight(World* worldIn, Random& rand, int minY, int maxY);

    StructureComponentList components;
    StructureBoundingBox boundingBox;

private:
    int chunkPosX = 0;
    int chunkPosZ = 0;
};

} // namespace mc

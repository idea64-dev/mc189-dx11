#pragma once

// Original: net/minecraft/world/gen/structure/StructureVillagePieces.java -- the village: a well, roads and
// the houses along them. Desert villages swap logs, cobblestone, planks, stairs and gravel for sandstone
// (Village::func_175847_a). The villagers are entities (stage 10).

#include "world/gen/structure/StructureComponent.h"

#include <memory>
#include <vector>

namespace mc {

class Block;
class WorldChunkManager;

namespace StructureVillagePieces {

class Start;
class Village;

void registerVillagePieces();

struct PieceWeight {
    using Creator = std::shared_ptr<Village> (*)(Start* start, StructureComponentList& list, Random& rand, int x, int y, int z, EnumFacing facing, int type);

    PieceWeight(Creator creator, int weight, int limit)
        : villagePieceClass(creator)
        , villagePieceWeight(weight)
        , villagePiecesLimit(limit)
    {
    }
    bool canSpawnMoreVillagePiecesOfType(int) const { return villagePiecesLimit == 0 || villagePiecesSpawned < villagePiecesLimit; }
    bool canSpawnMoreVillagePieces() const { return villagePiecesLimit == 0 || villagePiecesSpawned < villagePiecesLimit; }

    Creator villagePieceClass;
    const int villagePieceWeight;
    int villagePiecesSpawned = 0;
    int villagePiecesLimit;
};

using PieceWeightList = std::vector<std::shared_ptr<PieceWeight>>;

PieceWeightList getStructureVillageWeightedPieceList(Random& random, int size);

class Village : public StructureComponent {
public:
    Village() = default;
    Village(Start* start, int type);
    static bool canVillageGoDeeper(const StructureBoundingBox& structurebb) { return structurebb.minY > 10; }

protected:
    void writeStructureToNBT(NBTTagCompound& tagCompound) const override;
    void readStructureFromNBT(const NBTTagCompound& tagCompound) override;
    std::shared_ptr<StructureComponent> getNextComponentNN(Start* start, StructureComponentList& list, Random& rand, int y, int offset);
    std::shared_ptr<StructureComponent> getNextComponentPP(Start* start, StructureComponentList& list, Random& rand, int y, int offset);
    int getAverageGroundLevel(World* worldIn, const StructureBoundingBox& structurebb);
    void spawnVillagers(World* worldIn, const StructureBoundingBox& structurebb, int x, int y, int z, int count);
    virtual int func_180779_c(int index, int profession) { return profession; }
    const IBlockState* func_175847_a(const IBlockState* state) const;
    void setBlockState(World* worldIn, const IBlockState* blockstateIn, int x, int y, int z, const StructureBoundingBox& boundingboxIn) override;
    void fillWithBlocks(World* worldIn, const StructureBoundingBox& boundingboxIn, int xMin, int yMin, int zMin, int xMax, int yMax, int zMax,
        const IBlockState* boundaryBlockState, const IBlockState* insideBlockState, bool existingOnly) override;
    void replaceAirAndLiquidDownwards(World* worldIn, const IBlockState* blockstateIn, int x, int y, int z, const StructureBoundingBox& boundingboxIn) override;
    void func_175846_a(bool desert) { isDesertVillage = desert; }

    int field_143015_k = -1;

private:
    int villagersSpawned = 0;
    bool isDesertVillage = false;
};

// The pieces with the usual (start, type, rand, box, facing) constructor and a static finder.
#define MC_VILLAGE_PIECE(NAME, ID)                                                                                                                  \
public:                                                                                                                                             \
    NAME() = default;                                                                                                                               \
    NAME(Start* start, int type, Random& rand, const StructureBoundingBox& structurebb, EnumFacing facing);                                         \
    bool addComponentParts(World* worldIn, Random& randomIn, const StructureBoundingBox& structureBoundingBoxIn) override;                          \
    std::string_view getStructureName() const override { return ID; }

class Church : public Village {
    MC_VILLAGE_PIECE(Church, "ViST")
    static std::shared_ptr<Village> func_175854_a(Start* start, StructureComponentList& list, Random& rand, int x, int y, int z, EnumFacing facing, int type);

protected:
    int func_180779_c(int, int) override { return 2; }
};

class Field1 : public Village {
    MC_VILLAGE_PIECE(Field1, "ViDF")
    static std::shared_ptr<Village> func_175851_a(Start* start, StructureComponentList& list, Random& rand, int x, int y, int z, EnumFacing facing, int type);

protected:
    void writeStructureToNBT(NBTTagCompound& tagCompound) const override;
    void readStructureFromNBT(const NBTTagCompound& tagCompound) override;

private:
    static Block* func_151559_a(Random& rand);

    Block* cropTypeA = nullptr;
    Block* cropTypeB = nullptr;
    Block* cropTypeC = nullptr;
    Block* cropTypeD = nullptr;
};

class Field2 : public Village {
    MC_VILLAGE_PIECE(Field2, "ViF")
    static std::shared_ptr<Village> func_175852_a(Start* start, StructureComponentList& list, Random& rand, int x, int y, int z, EnumFacing facing, int type);

protected:
    void writeStructureToNBT(NBTTagCompound& tagCompound) const override;
    void readStructureFromNBT(const NBTTagCompound& tagCompound) override;

private:
    static Block* func_151560_a(Random& rand);

    Block* cropTypeA = nullptr;
    Block* cropTypeB = nullptr;
};

class Hall : public Village {
    MC_VILLAGE_PIECE(Hall, "ViPH")
    static std::shared_ptr<Village> func_175857_a(Start* start, StructureComponentList& list, Random& rand, int x, int y, int z, EnumFacing facing, int type);

protected:
    int func_180779_c(int index, int profession) override { return index == 0 ? 4 : Village::func_180779_c(index, profession); }
};

class House1 : public Village {
    MC_VILLAGE_PIECE(House1, "ViBH")
    static std::shared_ptr<Village> func_175850_a(Start* start, StructureComponentList& list, Random& rand, int x, int y, int z, EnumFacing facing, int type);

protected:
    int func_180779_c(int, int) override { return 1; }
};

class House2 : public Village {
    MC_VILLAGE_PIECE(House2, "ViS")
    static std::shared_ptr<Village> func_175855_a(Start* start, StructureComponentList& list, Random& rand, int x, int y, int z, EnumFacing facing, int type);

protected:
    void writeStructureToNBT(NBTTagCompound& tagCompound) const override;
    void readStructureFromNBT(const NBTTagCompound& tagCompound) override;
    int func_180779_c(int, int) override { return 3; }

private:
    bool hasMadeChest = false;
};

class House3 : public Village {
    MC_VILLAGE_PIECE(House3, "ViTRH")
    static std::shared_ptr<Village> func_175849_a(Start* start, StructureComponentList& list, Random& rand, int x, int y, int z, EnumFacing facing, int type);
};

class House4Garden : public Village {
    MC_VILLAGE_PIECE(House4Garden, "ViSH")
    static std::shared_ptr<Village> func_175858_a(Start* start, StructureComponentList& list, Random& rand, int x, int y, int z, EnumFacing facing, int type);

protected:
    void writeStructureToNBT(NBTTagCompound& tagCompound) const override;
    void readStructureFromNBT(const NBTTagCompound& tagCompound) override;

private:
    bool isRoofAccessible = false;
};

class WoodHut : public Village {
    MC_VILLAGE_PIECE(WoodHut, "ViSmH")
    static std::shared_ptr<Village> func_175853_a(Start* start, StructureComponentList& list, Random& rand, int x, int y, int z, EnumFacing facing, int type);

protected:
    void writeStructureToNBT(NBTTagCompound& tagCompound) const override;
    void readStructureFromNBT(const NBTTagCompound& tagCompound) override;

private:
    bool isTallHouse = false;
    int tablePosition = 0;
};

class Torch : public Village {
    MC_VILLAGE_PIECE(Torch, "ViL")
    static std::optional<StructureBoundingBox> func_175856_a(Start* start, StructureComponentList& list, Random& rand, int x, int y, int z, EnumFacing facing);
};

class Road : public Village {
public:
    Road() = default;
    Road(Start* start, int type)
        : Village(start, type)
    {
    }
};

class Path : public Road {
public:
    Path() = default;
    Path(Start* start, int type, Random& rand, const StructureBoundingBox& structurebb, EnumFacing facing);
    void buildComponent(StructureComponent* componentIn, StructureComponentList& listIn, Random& rand) override;
    static std::optional<StructureBoundingBox> func_175848_a(Start* start, StructureComponentList& list, Random& rand, int x, int y, int z, EnumFacing facing);
    bool addComponentParts(World* worldIn, Random& randomIn, const StructureBoundingBox& structureBoundingBoxIn) override;
    std::string_view getStructureName() const override { return "ViSR"; }

protected:
    void writeStructureToNBT(NBTTagCompound& tagCompound) const override;
    void readStructureFromNBT(const NBTTagCompound& tagCompound) override;

private:
    int length = 0;
};

class Well : public Village {
public:
    Well() = default;
    Well(Start* start, int type, Random& rand, int x, int z);
    void buildComponent(StructureComponent* componentIn, StructureComponentList& listIn, Random& rand) override;
    bool addComponentParts(World* worldIn, Random& randomIn, const StructureBoundingBox& structureBoundingBoxIn) override;
    std::string_view getStructureName() const override { return "ViW"; }
};

class Start : public Well {
public:
    Start() = default;
    Start(WorldChunkManager* chunkManagerIn, int type, Random& rand, int x, int z, PieceWeightList pieces, int size);
    WorldChunkManager* getWorldChunkManager() const { return worldChunkMngr; }
    std::string_view getStructureName() const override { return "ViStart"; }

    WorldChunkManager* worldChunkMngr = nullptr;
    bool inDesert = false;
    int terrainType = 0;
    std::shared_ptr<PieceWeight> structVillagePieceWeight;
    PieceWeightList structureVillageWeightedPieceList;
    StructureComponentList field_74932_i;
    StructureComponentList field_74930_j;
};

#undef MC_VILLAGE_PIECE

} // namespace StructureVillagePieces

} // namespace mc

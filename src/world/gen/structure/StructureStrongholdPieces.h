#pragma once

// Original: net/minecraft/world/gen/structure/StructureStrongholdPieces.java -- the stronghold: corridors,
// stairs, rooms, a library and the end portal room, grown from a spiral staircase (Stairs2).
// Java keeps the piece weights in statics that prepareStructurePieces resets for each stronghold; so does this.

#include "world/gen/structure/StructureComponent.h"

#include <climits>
#include <memory>
#include <optional>
#include <vector>

namespace mc {

namespace StructureStrongholdPieces {

class Stronghold;
class Stairs2;
class PortalRoom;

void registerStrongholdPieces();
void prepareStructurePieces();

struct PieceWeight {
    // Java: the piece class, dispatched by func_175954_a to the class's static finder.
    using Creator = std::shared_ptr<Stronghold> (*)(StructureComponentList& list, Random& rand, int x, int y, int z, EnumFacing facing, int type);

    PieceWeight(Creator creator, int weight, int limit, int minTypeExclusive = INT_MIN)
        : pieceClass(creator)
        , pieceWeight(weight)
        , instancesLimit(limit)
        , minType(minTypeExclusive)
    {
    }
    // Java overrides this in anonymous subclasses for the library (type > 4) and the portal room (type > 5).
    bool canSpawnMoreStructuresOfType(int type) const { return (instancesLimit == 0 || instancesSpawned < instancesLimit) && type > minType; }
    bool canSpawnMoreStructures() const { return instancesLimit == 0 || instancesSpawned < instancesLimit; }

    Creator pieceClass;
    const int pieceWeight;
    int instancesSpawned = 0;
    int instancesLimit;
    int minType;
};

class Stronghold : public StructureComponent {
public:
    enum class Door { OPENING, WOOD_DOOR, GRATES, IRON_DOOR };

    Stronghold() = default;
    explicit Stronghold(int type)
        : StructureComponent(type)
    {
    }

protected:
    void writeStructureToNBT(NBTTagCompound& tagCompound) const override;
    void readStructureFromNBT(const NBTTagCompound& tagCompound) override;
    void placeDoor(World* worldIn, Random& rand, const StructureBoundingBox& structurebb, Door door, int x, int y, int z);
    Door getRandomDoor(Random& rand);
    std::shared_ptr<StructureComponent> getNextComponentNormal(Stairs2* start, StructureComponentList& list, Random& rand, int x, int y);
    std::shared_ptr<StructureComponent> getNextComponentX(Stairs2* start, StructureComponentList& list, Random& rand, int y, int z);
    std::shared_ptr<StructureComponent> getNextComponentZ(Stairs2* start, StructureComponentList& list, Random& rand, int y, int z);

public:
    static bool canStrongholdGoDeeper(const StructureBoundingBox& structurebb) { return structurebb.minY > 10; }

protected:
    Door field_143013_d = Door::OPENING;
};

#define MC_STRONGHOLD_FINDER(NAME, FINDER, ID)                                                                                                      \
public:                                                                                                                                             \
    NAME() = default;                                                                                                                               \
    NAME(int type, Random& rand, const StructureBoundingBox& structurebb, EnumFacing facing);                                                       \
    static std::shared_ptr<Stronghold> FINDER(StructureComponentList& list, Random& rand, int x, int y, int z, EnumFacing facing, int type);       \
    bool addComponentParts(World* worldIn, Random& randomIn, const StructureBoundingBox& structureBoundingBoxIn) override;                          \
    std::string_view getStructureName() const override { return ID; }

class ChestCorridor : public Stronghold {
    MC_STRONGHOLD_FINDER(ChestCorridor, func_175868_a, "SHCC")
    void buildComponent(StructureComponent* componentIn, StructureComponentList& listIn, Random& rand) override;

protected:
    void writeStructureToNBT(NBTTagCompound& tagCompound) const override;
    void readStructureFromNBT(const NBTTagCompound& tagCompound) override;

private:
    bool hasMadeChest = false;
};

class Corridor : public Stronghold {
public:
    Corridor() = default;
    Corridor(int type, Random& rand, const StructureBoundingBox& structurebb, EnumFacing facing);
    static std::optional<StructureBoundingBox> func_175869_a(StructureComponentList& list, Random& rand, int x, int y, int z, EnumFacing facing);
    bool addComponentParts(World* worldIn, Random& randomIn, const StructureBoundingBox& structureBoundingBoxIn) override;
    std::string_view getStructureName() const override { return "SHFC"; }

protected:
    void writeStructureToNBT(NBTTagCompound& tagCompound) const override;
    void readStructureFromNBT(const NBTTagCompound& tagCompound) override;

private:
    int field_74993_a = 0;
};

class Crossing : public Stronghold {
    MC_STRONGHOLD_FINDER(Crossing, func_175866_a, "SH5C")
    void buildComponent(StructureComponent* componentIn, StructureComponentList& listIn, Random& rand) override;

protected:
    void writeStructureToNBT(NBTTagCompound& tagCompound) const override;
    void readStructureFromNBT(const NBTTagCompound& tagCompound) override;

private:
    bool field_74996_b = false;
    bool field_74997_c = false;
    bool field_74995_d = false;
    bool field_74999_h = false;
};

class LeftTurn : public Stronghold {
    MC_STRONGHOLD_FINDER(LeftTurn, func_175867_a, "SHLT")
    void buildComponent(StructureComponent* componentIn, StructureComponentList& listIn, Random& rand) override;
};

// Java declares no finder of its own: RightTurn.func_175867_a is LeftTurn's and builds a LeftTurn, so right turns
// only come back from saves.
class RightTurn : public LeftTurn {
public:
    RightTurn() = default;
    void buildComponent(StructureComponent* componentIn, StructureComponentList& listIn, Random& rand) override;
    bool addComponentParts(World* worldIn, Random& randomIn, const StructureBoundingBox& structureBoundingBoxIn) override;
    std::string_view getStructureName() const override { return "SHRT"; }
};

class Library : public Stronghold {
    MC_STRONGHOLD_FINDER(Library, func_175864_a, "SHLi")

protected:
    void writeStructureToNBT(NBTTagCompound& tagCompound) const override;
    void readStructureFromNBT(const NBTTagCompound& tagCompound) override;

private:
    bool isLargeRoom = false;
};

class PortalRoom : public Stronghold {
    MC_STRONGHOLD_FINDER(PortalRoom, func_175865_a, "SHPR")
    void buildComponent(StructureComponent* componentIn, StructureComponentList& listIn, Random& rand) override;

protected:
    void writeStructureToNBT(NBTTagCompound& tagCompound) const override;
    void readStructureFromNBT(const NBTTagCompound& tagCompound) override;

private:
    bool hasSpawner = false;
};

class Prison : public Stronghold {
    MC_STRONGHOLD_FINDER(Prison, func_175860_a, "SHPH")
    void buildComponent(StructureComponent* componentIn, StructureComponentList& listIn, Random& rand) override;
};

class RoomCrossing : public Stronghold {
    MC_STRONGHOLD_FINDER(RoomCrossing, func_175859_a, "SHRC")
    void buildComponent(StructureComponent* componentIn, StructureComponentList& listIn, Random& rand) override;

protected:
    void writeStructureToNBT(NBTTagCompound& tagCompound) const override;
    void readStructureFromNBT(const NBTTagCompound& tagCompound) override;

    int roomType = 0;
};

class Stairs : public Stronghold {
    MC_STRONGHOLD_FINDER(Stairs, func_175863_a, "SHSD")
    // The first piece of a stronghold.
    Stairs(int type, Random& rand, int x, int z);
    void buildComponent(StructureComponent* componentIn, StructureComponentList& listIn, Random& rand) override;

protected:
    void writeStructureToNBT(NBTTagCompound& tagCompound) const override;
    void readStructureFromNBT(const NBTTagCompound& tagCompound) override;

private:
    bool field_75024_a = false;
};

class Stairs2 : public Stairs {
public:
    Stairs2() = default;
    Stairs2(int type, Random& rand, int x, int z);
    BlockPos getBoundingBoxCenter() const override;
    std::string_view getStructureName() const override { return "SHStart"; }

    PieceWeight* strongholdPieceWeight = nullptr;
    // Owned by the start's component list.
    PortalRoom* strongholdPortalRoom = nullptr;
    StructureComponentList field_75026_c;
};

class StairsStraight : public Stronghold {
    MC_STRONGHOLD_FINDER(StairsStraight, func_175861_a, "SHSSD")
    void buildComponent(StructureComponent* componentIn, StructureComponentList& listIn, Random& rand) override;
};

class Straight : public Stronghold {
    MC_STRONGHOLD_FINDER(Straight, func_175862_a, "SHS")
    void buildComponent(StructureComponent* componentIn, StructureComponentList& listIn, Random& rand) override;

protected:
    void writeStructureToNBT(NBTTagCompound& tagCompound) const override;
    void readStructureFromNBT(const NBTTagCompound& tagCompound) override;

private:
    bool expandsX = false;
    bool expandsZ = false;
};

#undef MC_STRONGHOLD_FINDER

} // namespace StructureStrongholdPieces

} // namespace mc

#pragma once

// Original: net/minecraft/world/gen/structure/StructureOceanMonumentPieces.java -- the ocean monument: one
// MonumentBuilding piece that lays out a 5x5x3 grid of room definitions and keeps the rooms built from them as its
// own sub-pieces (they are not in the start's component list and are not saved; StartMonument rebuilds them).
// The elder guardians are entities (stage 10).

#include "world/gen/structure/StructureComponent.h"

#include <array>
#include <memory>
#include <vector>

namespace mc {

namespace StructureOceanMonumentPieces {

void registerOceanMonumentPieces();

// Java: RoomDefinition -- a cell of the grid, linked to its neighbours by facing index. Owned by the
// MonumentBuilding that made it.
struct RoomDefinition {
    explicit RoomDefinition(int index)
        : field_175967_a(index)
    {
    }
    void func_175957_a(EnumFacing facing, RoomDefinition* other);
    void func_175958_a();
    bool func_175959_a(int mark);
    bool func_175961_b() const { return field_175967_a >= 75; }
    int func_175960_c() const;

    int field_175967_a;
    std::array<RoomDefinition*, 6> field_175965_b {};
    std::array<bool, 6> field_175966_c {};
    bool field_175963_d = false;
    bool field_175964_e = false;
    int field_175962_f = 0;
};

class Piece : public StructureComponent {
public:
    Piece()
        : StructureComponent(0)
    {
    }
    explicit Piece(int type)
        : StructureComponent(type)
    {
    }
    Piece(EnumFacing facing, const StructureBoundingBox& structurebb);

protected:
    Piece(int type, EnumFacing facing, RoomDefinition* room, int sizeX, int sizeY, int sizeZ);

    // The grid index of a cell.
    static constexpr int func_175820_a(int x, int y, int z) { return y * 25 + z * 5 + x; }
    static constexpr int field_175823_g = 0 * 25 + 0 * 5 + 2; // func_175820_a(2, 0, 0)
    static constexpr int field_175831_h = 2 * 25 + 0 * 5 + 2; // func_175820_a(2, 2, 0)
    static constexpr int field_175832_i = 1 * 25 + 0 * 5 + 0; // func_175820_a(0, 1, 0)
    static constexpr int field_175829_j = 1 * 25 + 0 * 5 + 4; // func_175820_a(4, 1, 0)
    // Java's static block states, looked up when used (the blocks don't exist at static initialisation here).
    static const IBlockState* field_175828_a();
    static const IBlockState* field_175826_b();
    static const IBlockState* field_175827_c();
    static const IBlockState* field_175824_d() { return field_175826_b(); }
    static const IBlockState* field_175825_e();
    static const IBlockState* field_175822_f();

    void writeStructureToNBT(NBTTagCompound&) const override { }
    void readStructureFromNBT(const NBTTagCompound&) override { }
    void func_181655_a(World* worldIn, const StructureBoundingBox& structurebb, int minX, int minY, int minZ, int maxX, int maxY, int maxZ, bool skipAir);
    void func_175821_a(World* worldIn, const StructureBoundingBox& structurebb, int x, int z, bool hasOpening);
    void func_175819_a(World* worldIn, const StructureBoundingBox& structurebb, int minX, int minY, int minZ, int maxX, int maxY, int maxZ,
        const IBlockState* state);
    bool func_175818_a(const StructureBoundingBox& structurebb, int minX, int minZ, int maxX, int maxZ);
    bool func_175817_a(World* worldIn, const StructureBoundingBox& structurebb, int x, int y, int z);

    RoomDefinition* field_175830_k = nullptr;
};

#define MC_MONUMENT_ROOM(NAME, ID)                                                                                                                  \
public:                                                                                                                                             \
    NAME() = default;                                                                                                                               \
    NAME(EnumFacing facing, RoomDefinition* room, Random& rand);                                                                                   \
    bool addComponentParts(World* worldIn, Random& randomIn, const StructureBoundingBox& structureBoundingBoxIn) override;                          \
    std::string_view getStructureName() const override { return ID; }

class DoubleXRoom : public Piece {
    MC_MONUMENT_ROOM(DoubleXRoom, "OMDXR")
};

class DoubleXYRoom : public Piece {
    MC_MONUMENT_ROOM(DoubleXYRoom, "OMDXYR")
};

class DoubleYRoom : public Piece {
    MC_MONUMENT_ROOM(DoubleYRoom, "OMDYR")
};

class DoubleYZRoom : public Piece {
    MC_MONUMENT_ROOM(DoubleYZRoom, "OMDYZR")
};

class DoubleZRoom : public Piece {
    MC_MONUMENT_ROOM(DoubleZRoom, "OMDZR")
};

class MonumentCoreRoom : public Piece {
    MC_MONUMENT_ROOM(MonumentCoreRoom, "OMCR")
};

class SimpleRoom : public Piece {
    MC_MONUMENT_ROOM(SimpleRoom, "OMSimple")

private:
    int field_175833_o = 0;
};

class SimpleTopRoom : public Piece {
    MC_MONUMENT_ROOM(SimpleTopRoom, "OMSimpleT")
};

#undef MC_MONUMENT_ROOM

class EntryRoom : public Piece {
public:
    EntryRoom() = default;
    EntryRoom(EnumFacing facing, RoomDefinition* room);
    bool addComponentParts(World* worldIn, Random& randomIn, const StructureBoundingBox& structureBoundingBoxIn) override;
    std::string_view getStructureName() const override { return "OMEntry"; }
};

class Penthouse : public Piece {
public:
    Penthouse() = default;
    Penthouse(EnumFacing facing, const StructureBoundingBox& structurebb);
    bool addComponentParts(World* worldIn, Random& randomIn, const StructureBoundingBox& structureBoundingBoxIn) override;
    std::string_view getStructureName() const override { return "OMPenthouse"; }
};

// Not registered in Java, like this.
class WingRoom : public Piece {
public:
    WingRoom() = default;
    WingRoom(EnumFacing facing, const StructureBoundingBox& structurebb, int index);
    bool addComponentParts(World* worldIn, Random& randomIn, const StructureBoundingBox& structureBoundingBoxIn) override;
    std::string_view getStructureName() const override { return {}; }

private:
    int field_175834_o = 0;
};

class MonumentBuilding : public Piece {
public:
    MonumentBuilding() = default;
    MonumentBuilding(Random& rand, int x, int z, EnumFacing facing);
    bool addComponentParts(World* worldIn, Random& randomIn, const StructureBoundingBox& structureBoundingBoxIn) override;
    std::string_view getStructureName() const override { return "OMB"; }

private:
    std::vector<RoomDefinition*> func_175836_a(Random& rand);
    void func_175840_a(bool east, int x, World* worldIn, Random& rand, const StructureBoundingBox& structurebb);
    void func_175839_b(World* worldIn, Random& rand, const StructureBoundingBox& structurebb);
    void func_175837_c(World* worldIn, Random& rand, const StructureBoundingBox& structurebb);
    void func_175841_d(World* worldIn, Random& rand, const StructureBoundingBox& structurebb);
    void func_175835_e(World* worldIn, Random& rand, const StructureBoundingBox& structurebb);
    void func_175842_f(World* worldIn, Random& rand, const StructureBoundingBox& structurebb);
    void func_175838_g(World* worldIn, Random& rand, const StructureBoundingBox& structurebb);

    std::vector<std::unique_ptr<RoomDefinition>> roomStorage;
    RoomDefinition* field_175845_o = nullptr;
    RoomDefinition* field_175844_p = nullptr;
    std::vector<std::shared_ptr<Piece>> field_175843_q;
};

} // namespace StructureOceanMonumentPieces

} // namespace mc

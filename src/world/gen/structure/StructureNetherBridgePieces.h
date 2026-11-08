#pragma once

// Original: net/minecraft/world/gen/structure/StructureNetherBridgePieces.java -- the nether fortress: bridges
// (primary pieces) and the corridors inside (secondary pieces), grown from a bridge crossing (Start).
// Java keeps the piece weights in statics that each Start resets and lists; so does this.

#include "world/gen/structure/StructureComponent.h"

#include <memory>
#include <vector>

namespace mc {

namespace StructureNetherBridgePieces {

class Piece;
class Start;

void registerNetherFortressPieces();

struct PieceWeight {
    // Java: the piece class, dispatched by func_175887_b to the class's static finder.
    using Creator = std::shared_ptr<Piece> (*)(StructureComponentList& list, Random& rand, int x, int y, int z, EnumFacing facing, int type);

    PieceWeight(Creator creator, int weight, int limit, bool repeatable = false)
        : weightClass(creator)
        , field_78826_b(weight)
        , field_78824_d(limit)
        , field_78825_e(repeatable)
    {
    }
    bool func_78822_a(int) const { return field_78824_d == 0 || field_78827_c < field_78824_d; }
    bool func_78823_a() const { return field_78824_d == 0 || field_78827_c < field_78824_d; }

    Creator weightClass;
    const int field_78826_b;
    int field_78827_c = 0;
    int field_78824_d;
    bool field_78825_e;
};

class Piece : public StructureComponent {
public:
    Piece() = default;

    static bool isAboveGround(const StructureBoundingBox& structurebb) { return structurebb.minY > 10; }

protected:
    explicit Piece(int type)
        : StructureComponent(type)
    {
    }
    void readStructureFromNBT(const NBTTagCompound&) override { }
    void writeStructureToNBT(NBTTagCompound&) const override { }
    std::shared_ptr<StructureComponent> getNextComponentNormal(Start* start, StructureComponentList& list, Random& rand, int x, int y, bool secondary);
    std::shared_ptr<StructureComponent> getNextComponentX(Start* start, StructureComponentList& list, Random& rand, int y, int z, bool secondary);
    std::shared_ptr<StructureComponent> getNextComponentZ(Start* start, StructureComponentList& list, Random& rand, int y, int z, bool secondary);

private:
    int getTotalWeight(const std::vector<PieceWeight*>& weights) const;
    std::shared_ptr<Piece> func_175871_a(Start* start, std::vector<PieceWeight*>& weights, StructureComponentList& list, Random& rand, int x, int y, int z,
        EnumFacing facing, int type);
    std::shared_ptr<StructureComponent> func_175870_a(Start* start, StructureComponentList& list, Random& rand, int x, int y, int z, EnumFacing facing,
        int type, bool secondary);
};

#define MC_FORTRESS_PIECE(NAME, FINDER, ID)                                                                                                         \
public:                                                                                                                                             \
    NAME() = default;                                                                                                                               \
    NAME(int type, Random& rand, const StructureBoundingBox& structurebb, EnumFacing facing);                                                       \
    static std::shared_ptr<Piece> FINDER(StructureComponentList& list, Random& rand, int x, int y, int z, EnumFacing facing, int type);            \
    bool addComponentParts(World* worldIn, Random& randomIn, const StructureBoundingBox& structureBoundingBoxIn) override;                          \
    std::string_view getStructureName() const override { return ID; }

class Corridor : public Piece {
    MC_FORTRESS_PIECE(Corridor, func_175879_a, "NeSCLT")
    void buildComponent(StructureComponent* componentIn, StructureComponentList& listIn, Random& rand) override;

protected:
    void readStructureFromNBT(const NBTTagCompound& tagCompound) override;
    void writeStructureToNBT(NBTTagCompound& tagCompound) const override;

private:
    bool field_111021_b = false;
};

class Corridor2 : public Piece {
    MC_FORTRESS_PIECE(Corridor2, func_175876_a, "NeSCRT")
    void buildComponent(StructureComponent* componentIn, StructureComponentList& listIn, Random& rand) override;

protected:
    void readStructureFromNBT(const NBTTagCompound& tagCompound) override;
    void writeStructureToNBT(NBTTagCompound& tagCompound) const override;

private:
    bool field_111020_b = false;
};

class Corridor3 : public Piece {
    MC_FORTRESS_PIECE(Corridor3, func_175883_a, "NeCCS")
    void buildComponent(StructureComponent* componentIn, StructureComponentList& listIn, Random& rand) override;
};

class Corridor4 : public Piece {
    MC_FORTRESS_PIECE(Corridor4, func_175880_a, "NeCTB")
    void buildComponent(StructureComponent* componentIn, StructureComponentList& listIn, Random& rand) override;
};

class Corridor5 : public Piece {
    MC_FORTRESS_PIECE(Corridor5, func_175877_a, "NeSC")
    void buildComponent(StructureComponent* componentIn, StructureComponentList& listIn, Random& rand) override;
};

class Crossing : public Piece {
    MC_FORTRESS_PIECE(Crossing, func_175873_a, "NeRC")
    void buildComponent(StructureComponent* componentIn, StructureComponentList& listIn, Random& rand) override;
};

class Crossing2 : public Piece {
    MC_FORTRESS_PIECE(Crossing2, func_175878_a, "NeSCSC")
    void buildComponent(StructureComponent* componentIn, StructureComponentList& listIn, Random& rand) override;
};

class Crossing3 : public Piece {
    MC_FORTRESS_PIECE(Crossing3, func_175885_a, "NeBCr")
    // The first piece of a fortress.
    Crossing3(Random& rand, int x, int z);
    void buildComponent(StructureComponent* componentIn, StructureComponentList& listIn, Random& rand) override;
};

class End : public Piece {
    MC_FORTRESS_PIECE(End, func_175884_a, "NeBEF")

protected:
    void readStructureFromNBT(const NBTTagCompound& tagCompound) override;
    void writeStructureToNBT(NBTTagCompound& tagCompound) const override;

private:
    int fillSeed = 0;
};

class Entrance : public Piece {
    MC_FORTRESS_PIECE(Entrance, func_175881_a, "NeCE")
    void buildComponent(StructureComponent* componentIn, StructureComponentList& listIn, Random& rand) override;
};

class NetherStalkRoom : public Piece {
    MC_FORTRESS_PIECE(NetherStalkRoom, func_175875_a, "NeCSR")
    void buildComponent(StructureComponent* componentIn, StructureComponentList& listIn, Random& rand) override;
};

// Java's finder takes the type before the facing; the PieceWeight table adapts it.
class Stairs : public Piece {
public:
    Stairs() = default;
    Stairs(int type, Random& rand, const StructureBoundingBox& structurebb, EnumFacing facing);
    static std::shared_ptr<Piece> func_175872_a(StructureComponentList& list, Random& rand, int x, int y, int z, int type, EnumFacing facing);
    void buildComponent(StructureComponent* componentIn, StructureComponentList& listIn, Random& rand) override;
    bool addComponentParts(World* worldIn, Random& randomIn, const StructureBoundingBox& structureBoundingBoxIn) override;
    std::string_view getStructureName() const override { return "NeSR"; }
};

class Straight : public Piece {
    MC_FORTRESS_PIECE(Straight, func_175882_a, "NeBS")
    void buildComponent(StructureComponent* componentIn, StructureComponentList& listIn, Random& rand) override;
};

class Throne : public Piece {
public:
    Throne() = default;
    Throne(int type, Random& rand, const StructureBoundingBox& structurebb, EnumFacing facing);
    static std::shared_ptr<Piece> func_175874_a(StructureComponentList& list, Random& rand, int x, int y, int z, int type, EnumFacing facing);
    bool addComponentParts(World* worldIn, Random& randomIn, const StructureBoundingBox& structureBoundingBoxIn) override;
    std::string_view getStructureName() const override { return "NeMT"; }

protected:
    void readStructureFromNBT(const NBTTagCompound& tagCompound) override;
    void writeStructureToNBT(NBTTagCompound& tagCompound) const override;

private:
    bool hasSpawner = false;
};

#undef MC_FORTRESS_PIECE

class Start : public Crossing3 {
public:
    Start() = default;
    Start(Random& rand, int x, int z);
    std::string_view getStructureName() const override { return "NeStart"; }

    PieceWeight* theNetherBridgePieceWeight = nullptr;
    std::vector<PieceWeight*> primaryWeights;
    std::vector<PieceWeight*> secondaryWeights;
    StructureComponentList field_74967_d;
};

} // namespace StructureNetherBridgePieces

} // namespace mc

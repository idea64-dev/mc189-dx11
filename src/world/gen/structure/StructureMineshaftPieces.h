#pragma once

// Original: net/minecraft/world/gen/structure/StructureMineshaftPieces.java -- corridors, crossings, rooms and
// stairs. The chest minecarts are entities (stage 10); their loot is still drawn, so the generator stays in step.

#include "world/gen/structure/StructureComponent.h"

#include <list>

namespace mc {

namespace StructureMineshaftPieces {

void registerStructurePieces();

class Corridor : public StructureComponent {
public:
    Corridor() = default;
    Corridor(int type, Random& rand, const StructureBoundingBox& structurebb, EnumFacing facing);

    // Null where nothing fits.
    static std::optional<StructureBoundingBox> func_175814_a(const StructureComponentList& listIn, Random& rand, int x, int y, int z, EnumFacing facing);
    void buildComponent(StructureComponent* componentIn, StructureComponentList& listIn, Random& rand) override;
    bool addComponentParts(World* worldIn, Random& randomIn, const StructureBoundingBox& structureBoundingBoxIn) override;
    std::string_view getStructureName() const override { return "MSCorridor"; }

protected:
    void writeStructureToNBT(NBTTagCompound& tagCompound) const override;
    void readStructureFromNBT(const NBTTagCompound& tagCompound) override;

private:
    bool generateMinecartChest(World* worldIn, const StructureBoundingBox& boundingBoxIn, Random& rand, int x, int y, int z,
        const std::vector<WeightedRandomChestContent>& listIn, int max);

    bool hasRails = false;
    bool hasSpiders = false;
    bool spawnerPlaced = false;
    int sectionCount = 0;
};

class Cross : public StructureComponent {
public:
    Cross() = default;
    Cross(int type, Random& rand, const StructureBoundingBox& structurebb, EnumFacing facing);

    static std::optional<StructureBoundingBox> func_175813_a(const StructureComponentList& listIn, Random& rand, int x, int y, int z, EnumFacing facing);
    void buildComponent(StructureComponent* componentIn, StructureComponentList& listIn, Random& rand) override;
    bool addComponentParts(World* worldIn, Random& randomIn, const StructureBoundingBox& structureBoundingBoxIn) override;
    std::string_view getStructureName() const override { return "MSCrossing"; }

protected:
    void writeStructureToNBT(NBTTagCompound& tagCompound) const override;
    void readStructureFromNBT(const NBTTagCompound& tagCompound) override;

private:
    EnumFacing corridorDirection = EnumFacing::NORTH;
    bool isMultipleFloors = false;
};

class Room : public StructureComponent {
public:
    Room() = default;
    Room(int type, Random& rand, int x, int z);

    void buildComponent(StructureComponent* componentIn, StructureComponentList& listIn, Random& rand) override;
    bool addComponentParts(World* worldIn, Random& randomIn, const StructureBoundingBox& structureBoundingBoxIn) override;
    void func_181138_a(int x, int y, int z) override;
    std::string_view getStructureName() const override { return "MSRoom"; }

protected:
    void writeStructureToNBT(NBTTagCompound& tagCompound) const override;
    void readStructureFromNBT(const NBTTagCompound& tagCompound) override;

private:
    std::vector<StructureBoundingBox> roomsLinkedToTheRoom;
};

class Stairs : public StructureComponent {
public:
    Stairs() = default;
    Stairs(int type, Random& rand, const StructureBoundingBox& structurebb, EnumFacing facing);

    static std::optional<StructureBoundingBox> func_175812_a(const StructureComponentList& listIn, Random& rand, int x, int y, int z, EnumFacing facing);
    void buildComponent(StructureComponent* componentIn, StructureComponentList& listIn, Random& rand) override;
    bool addComponentParts(World* worldIn, Random& randomIn, const StructureBoundingBox& structureBoundingBoxIn) override;
    std::string_view getStructureName() const override { return "MSStairs"; }

protected:
    void writeStructureToNBT(NBTTagCompound&) const override { }
    void readStructureFromNBT(const NBTTagCompound&) override { }
};

} // namespace StructureMineshaftPieces

} // namespace mc

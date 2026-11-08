#pragma once

// Original: net/minecraft/world/gen/structure/ComponentScatteredFeaturePieces.java -- the single-piece temples:
// desert pyramid, jungle temple and witch hut. The witch is an entity (stage 10).

#include "world/gen/structure/StructureComponent.h"

namespace mc {

namespace ComponentScatteredFeaturePieces {

void registerScatteredFeaturePieces();

class Feature : public StructureComponent {
public:
    Feature() = default;
    Feature(Random& rand, int x, int y, int z, int sizeX, int sizeY, int sizeZ);

protected:
    void writeStructureToNBT(NBTTagCompound& tagCompound) const override;
    void readStructureFromNBT(const NBTTagCompound& tagCompound) override;
    // Moves the piece onto the average ground level the first time; false while none of it is loaded.
    bool func_74935_a(World* worldIn, const StructureBoundingBox& structurebb, int yOffset);

    int scatteredFeatureSizeX = 0;
    int scatteredFeatureSizeY = 0;
    int scatteredFeatureSizeZ = 0;
    int field_74936_d = -1;
};

class DesertPyramid : public Feature {
public:
    DesertPyramid() = default;
    DesertPyramid(Random& rand, int x, int z);
    bool addComponentParts(World* worldIn, Random& randomIn, const StructureBoundingBox& structureBoundingBoxIn) override;
    std::string_view getStructureName() const override { return "TeDP"; }

protected:
    void writeStructureToNBT(NBTTagCompound& tagCompound) const override;
    void readStructureFromNBT(const NBTTagCompound& tagCompound) override;

private:
    bool hasPlacedChest[4] {};
};

class JunglePyramid : public Feature {
public:
    JunglePyramid() = default;
    JunglePyramid(Random& rand, int x, int z);
    bool addComponentParts(World* worldIn, Random& randomIn, const StructureBoundingBox& structureBoundingBoxIn) override;
    std::string_view getStructureName() const override { return "TeJP"; }

protected:
    void writeStructureToNBT(NBTTagCompound& tagCompound) const override;
    void readStructureFromNBT(const NBTTagCompound& tagCompound) override;

private:
    bool placedMainChest = false;
    bool placedHiddenChest = false;
    bool placedTrap1 = false;
    bool placedTrap2 = false;
};

class SwampHut : public Feature {
public:
    SwampHut() = default;
    SwampHut(Random& rand, int x, int z);
    bool addComponentParts(World* worldIn, Random& randomIn, const StructureBoundingBox& structureBoundingBoxIn) override;
    std::string_view getStructureName() const override { return "TeSH"; }

protected:
    void writeStructureToNBT(NBTTagCompound& tagCompound) const override;
    void readStructureFromNBT(const NBTTagCompound& tagCompound) override;

private:
    bool hasWitch = false;
};

} // namespace ComponentScatteredFeaturePieces

} // namespace mc

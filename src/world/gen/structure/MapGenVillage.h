#pragma once

// Original: net/minecraft/world/gen/structure/MapGenVillage.java -- one village per 32x32-chunk region (at a
// random chunk of its first 24x24), in plains, desert and savanna. The pieces are in StructureVillagePieces.

#include "world/gen/FlatGeneratorInfo.h"
#include "world/gen/structure/MapGenStructure.h"

#include <vector>

namespace mc {

class BiomeGenBase;

class MapGenVillage : public MapGenStructure {
public:
    MapGenVillage() = default;
    explicit MapGenVillage(const FlatGeneratorInfo::Options& options);

    // Java: villageSpawnBiomes (built on first use, after the biomes are).
    static const std::vector<BiomeGenBase*>& villageSpawnBiomes();

    std::string getStructureName() const override { return "Village"; }

    class Start : public StructureStart {
    public:
        Start() = default;
        Start(World* worldIn, Random& rand, int x, int z, int size);

        bool isSizeableStructure() const override { return hasMoreThanTwoComponents; }
        void writeToNBT(NBTTagCompound& tagCompound) const override;
        void readFromNBT(const NBTTagCompound& tagCompound) override;
        std::string_view getStructureName() const override { return "Village"; }

    private:
        bool hasMoreThanTwoComponents = false;
    };

protected:
    bool canSpawnStructureAtCoords(int chunkX, int chunkZ) override;
    std::shared_ptr<StructureStart> getStructureStart(int chunkX, int chunkZ) override;

private:
    int terrainType = 0;
    int field_82665_g = 32;
    int field_82666_h = 8;
};

} // namespace mc

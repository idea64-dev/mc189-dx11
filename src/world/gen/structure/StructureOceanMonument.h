#pragma once

// Original: net/minecraft/world/gen/structure/StructureOceanMonument.java -- one monument per 32x32-chunk region
// (spread toward the middle of it), in deep ocean. Guardians spawn in it (getScatteredFeatureSpawnList).

#include "core/JavaHashMap.h"
#include "world/ChunkCoordIntPair.h"
#include "world/biome/BiomeGenBase.h"
#include "world/gen/FlatGeneratorInfo.h"
#include "world/gen/structure/MapGenStructure.h"

#include <vector>

namespace mc {

class StructureOceanMonument : public MapGenStructure {
public:
    StructureOceanMonument() = default;
    explicit StructureOceanMonument(const FlatGeneratorInfo::Options& options);

    std::string getStructureName() const override { return "Monument"; }
    // Java: field_175802_d
    static const std::vector<BiomeGenBase*>& field_175802_d();
    static const std::vector<BiomeGenBase::SpawnListEntry>& getScatteredFeatureSpawnList();

    class StartMonument : public StructureStart {
    public:
        StartMonument() = default;
        StartMonument(World* worldIn, Random& rand, int chunkX, int chunkZ);

        void generateStructure(World* worldIn, Random& rand, const StructureBoundingBox& structurebb) override;
        bool func_175788_a(const ChunkCoordIntPair& pair) override;
        void func_175787_b(const ChunkCoordIntPair& pair) override;
        void writeToNBT(NBTTagCompound& tagCompound) const override;
        void readFromNBT(const NBTTagCompound& tagCompound) override;
        std::string_view getStructureName() const override { return "Monument"; }

    private:
        struct PairHash {
            int32_t operator()(const ChunkCoordIntPair& pair) const { return pair.hashCode(); }
        };

        void func_175789_b(World* worldIn, Random& rand, int chunkX, int chunkZ);

        // Java: a HashSet<ChunkCoordIntPair> of the chunks already generated.
        JavaHashMap<ChunkCoordIntPair, bool, PairHash> field_175791_c;
        bool field_175790_d = false;
    };

protected:
    bool canSpawnStructureAtCoords(int chunkX, int chunkZ) override;
    std::shared_ptr<StructureStart> getStructureStart(int chunkX, int chunkZ) override;

private:
    int field_175800_f = 32;
    int field_175801_g = 5;
};

} // namespace mc

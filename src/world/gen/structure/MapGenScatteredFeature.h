#pragma once

// Original: net/minecraft/world/gen/structure/MapGenScatteredFeature.java -- one temple per 32x32-chunk region:
// a desert pyramid, jungle temple or witch hut by biome. Witch huts spawn witches (getScatteredFeatureSpawnList).

#include "world/biome/BiomeGenBase.h"
#include "world/gen/FlatGeneratorInfo.h"
#include "world/gen/structure/MapGenStructure.h"

#include <vector>

namespace mc {

class MapGenScatteredFeature : public MapGenStructure {
public:
    MapGenScatteredFeature();
    explicit MapGenScatteredFeature(const FlatGeneratorInfo::Options& options);

    std::string getStructureName() const override { return "Temple"; }
    // Whether pos is inside a witch hut.
    bool func_175798_a(const BlockPos& pos);
    const std::vector<BiomeGenBase::SpawnListEntry>& getScatteredFeatureSpawnList() const { return scatteredFeatureSpawnList; }

    class Start : public StructureStart {
    public:
        Start() = default;
        Start(World* worldIn, Random& rand, int chunkX, int chunkZ);
        std::string_view getStructureName() const override { return "Temple"; }
    };

protected:
    bool canSpawnStructureAtCoords(int chunkX, int chunkZ) override;
    std::shared_ptr<StructureStart> getStructureStart(int chunkX, int chunkZ) override;

private:
    std::vector<BiomeGenBase::SpawnListEntry> scatteredFeatureSpawnList;
    int maxDistanceBetweenScatteredFeatures = 32;
    int minDistanceBetweenScatteredFeatures = 8;
};

} // namespace mc

#pragma once

// Original: net/minecraft/world/gen/structure/MapGenNetherBridge.java -- nether fortresses, one in three 16x16-chunk
// regions. Blazes, zombie pigmen, skeletons and magma cubes spawn in them (getSpawnList).

#include "world/biome/BiomeGenBase.h"
#include "world/gen/structure/MapGenStructure.h"

#include <vector>

namespace mc {

class MapGenNetherBridge : public MapGenStructure {
public:
    MapGenNetherBridge();

    std::string getStructureName() const override { return "Fortress"; }
    const std::vector<BiomeGenBase::SpawnListEntry>& getSpawnList() const { return spawnList; }

    class Start : public StructureStart {
    public:
        Start() = default;
        Start(World* worldIn, Random& rand, int chunkX, int chunkZ);
        std::string_view getStructureName() const override { return "Fortress"; }
    };

protected:
    bool canSpawnStructureAtCoords(int chunkX, int chunkZ) override;
    std::shared_ptr<StructureStart> getStructureStart(int chunkX, int chunkZ) override;

private:
    std::vector<BiomeGenBase::SpawnListEntry> spawnList;
};

} // namespace mc

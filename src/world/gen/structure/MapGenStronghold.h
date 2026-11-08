#pragma once

// Original: net/minecraft/world/gen/structure/MapGenStronghold.java -- three strongholds on a ring around the
// origin, placed from the world seed the first time a chunk asks. The pieces are in StructureStrongholdPieces.

#include "world/ChunkCoordIntPair.h"
#include "world/gen/FlatGeneratorInfo.h"
#include "world/gen/structure/MapGenStructure.h"

#include <optional>
#include <vector>

namespace mc {

class BiomeGenBase;

class MapGenStronghold : public MapGenStructure {
public:
    MapGenStronghold();
    explicit MapGenStronghold(const FlatGeneratorInfo::Options& options);

    std::string getStructureName() const override { return "Stronghold"; }

    class Start : public StructureStart {
    public:
        Start() = default;
        Start(World* worldIn, Random& rand, int chunkX, int chunkZ);
        std::string_view getStructureName() const override { return "Stronghold"; }
    };

protected:
    bool canSpawnStructureAtCoords(int chunkX, int chunkZ) override;
    std::optional<std::vector<BlockPos>> getCoordList() override;
    std::shared_ptr<StructureStart> getStructureStart(int chunkX, int chunkZ) override;

private:
    std::vector<BiomeGenBase*> field_151546_e;
    bool ranBiomeCheck = false;
    // Java: ChunkCoordIntPair[], null until placed.
    std::vector<std::optional<ChunkCoordIntPair>> structureCoords;
    double field_82671_h = 32.0;
    int field_82672_i = 3;
};

} // namespace mc
